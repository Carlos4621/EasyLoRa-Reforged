#ifndef SERIAL_TRANSPORT_HEADER
#define SERIAL_TRANSPORT_HEADER

#include "boost/asio.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <functional>
#include <vector>
#include <deque>
#include <memory>
#include <utility>
#include "Frame.hpp"

/// @brief Configuraciones para el puerto serial abierto con BasicSerialTransport
struct SerialConfig {
    uint32_t baudRate;
    boost::asio::serial_port_base::flow_control::type flowControl;
    boost::asio::serial_port_base::parity::type parityByte;
    boost::asio::serial_port_base::stop_bits::type stopBits;
    uint32_t characterSize;
};

/// @brief Configuración default del puerto serial (9600 8N1)
static constexpr SerialConfig Default_Serial_Config{ 
    .baudRate = 9600, 
    .flowControl = boost::asio::serial_port_base::flow_control::type::none,
    .parityByte = boost::asio::serial_port_base::parity::type::none,
    .stopBits = boost::asio::serial_port::stop_bits::type::one,
    .characterSize = 8
};

/// @brief Clase encargada de recibir asíncronamente bytes y notificar al usuario para su posterior procesamiento
template<class Stream>
class BasicSerialTransport {
public:
    using FrameHandler = std::function<void(std::vector<uint8_t>)>;
    using ErrorHandler = std::function<void(boost::system::error_code)>;

    /// @brief Constructor base
    /// @param context Contexto de I/O
    explicit BasicSerialTransport(boost::asio::io_context& context);

    BasicSerialTransport(const BasicSerialTransport&) = delete;
    BasicSerialTransport& operator=(const BasicSerialTransport&) = delete;
    BasicSerialTransport(BasicSerialTransport&&) noexcept = default;
    BasicSerialTransport& operator=(BasicSerialTransport&&) noexcept = default;

    ~BasicSerialTransport() noexcept;

    /// @brief Abre el puerto seleccionas con las opciones dadas y empieza a escuchar
    /// @param portPath Dirección del puerto a abrir
    /// @param config Configuración a usar. Por defecto 9600 8N1
    void open(std::string portPath, SerialConfig config = Default_Serial_Config);

    /// @brief Cancela todas las operaciones pendientes y limpia los buffers
    void close();

    /// @brief Establece la función a la que se llamará con los datos cuando se reciba un frame completo
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(std::vector<uint8_t>)
    void setFrameHandler(FrameHandler handler) noexcept;

    /// @brief Establece la función a la que se llamará con los datos cuando se produzca un error
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(boost::system::error_code)
    void setErrorHandler(ErrorHandler handler) noexcept;

    /// @brief Escribe asincrónicamente los bytes daos
    /// @param toWrite Bytes a escribir
    /// @note Se usa un sistema de colas para situaciones donde el último mensaje no se ha enviado aún y se llama de nuevo a esta función
    void asyncWrite(std::vector<uint8_t> toWrite);

private:
    struct State;

    std::shared_ptr<State> state_m;
};

template <class Stream>
struct BasicSerialTransport<Stream>::State : std::enable_shared_from_this<State> {
    explicit State(boost::asio::io_context& context);

    boost::asio::strand<boost::asio::io_context::executor_type> strand;

    Stream serial;

    std::vector<uint8_t> rxBuffer;
    std::deque<std::vector<uint8_t>> txDeque;

    FrameHandler frameHandler{ nullptr };
    ErrorHandler errorHandler{ nullptr };
    
    bool closing{ false };

    void open(std::string portPath, SerialConfig config = Default_Serial_Config);
    void close();

    void setFrameHandler(FrameHandler handler) noexcept;
    void setErrorHandler(ErrorHandler handler) noexcept;

    void asyncWrite(std::vector<uint8_t> toWrite);

    void startRead();
    void handleRead(boost::system::error_code ec, size_t bytesWritten);

    void doWrite();
    void handleWrite(boost::system::error_code ec, size_t bytesWritten);

    void requestClose() noexcept;
    void closeOnStrand() noexcept;
};

template <class Stream>
inline BasicSerialTransport<Stream>::BasicSerialTransport(boost::asio::io_context &context) 
    : state_m{ std::make_shared<State>(context) }
{
}

template <class Stream>
inline BasicSerialTransport<Stream>::~BasicSerialTransport() noexcept {
    if (state_m != nullptr) {
        state_m->requestClose();
    }
}

template <class Stream>
inline void BasicSerialTransport<Stream>::open(std::string portPath, SerialConfig config) {
    state_m->open(std::move(portPath), std::move(config));
}

template <class Stream>
inline void BasicSerialTransport<Stream>::close() {
    state_m->close();
}

template<class Stream>
inline void BasicSerialTransport<Stream>::setFrameHandler(FrameHandler handler) noexcept {
    state_m->setFrameHandler(std::move(handler));
}

template <class Stream>
inline void BasicSerialTransport<Stream>::setErrorHandler(ErrorHandler handler) noexcept {
    state_m->setErrorHandler(std::move(handler));
}

template <class Stream>
inline void BasicSerialTransport<Stream>::asyncWrite(std::vector<uint8_t> toWrite) {
    state_m->asyncWrite(std::move(toWrite));
}

template <class Stream>
inline BasicSerialTransport<Stream>::State::State(boost::asio::io_context &context)
    : strand{ context.get_executor() }
    , serial{ context }
{
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::open(std::string portPath, SerialConfig config) {
    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() }; 

    boost::asio::post(
        strand,
        [self, config = std::move(config), portPath = std::move(portPath)] {
            if (self->closing) {
                return;
            }

            self->serial.open(portPath.data());

            self->serial.set_option(boost::asio::serial_port_base::baud_rate(config.baudRate));
            self->serial.set_option(boost::asio::serial_port_base::character_size(config.characterSize));
            self->serial.set_option(boost::asio::serial_port_base::parity(config.parityByte));
            self->serial.set_option(boost::asio::serial_port_base::stop_bits(config.stopBits));
            self->serial.set_option(boost::asio::serial_port_base::flow_control(config.flowControl));

            self->startRead();
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::close() {
    requestClose();
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::setFrameHandler(FrameHandler handler) noexcept {
    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::post(
        strand,
        [self, handler = std::move(handler)] mutable {
            if (self->closing) {
                return;
            }
            
            self->frameHandler = std::move(handler);
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::setErrorHandler(ErrorHandler handler) noexcept {
    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::post(
        strand,
        [self, handler = std::move(handler)] mutable {
            if (self->closing) {
                return;
            }
            
            self->errorHandler = std::move(handler);
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::asyncWrite(std::vector<uint8_t> toWrite) {
    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::post(
        strand,
        [self, toWrite = std::move(toWrite)] mutable {
            if (self->closing) { 
                return;
            }

            const bool idle{ self->txDeque.empty() };

            self->txDeque.emplace_back(std::move(toWrite));
    
            if (idle) {
                self->doWrite();
            }
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::startRead() {
    if (closing) {
        return;
    }

    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::async_read_until(
        serial,
        boost::asio::dynamic_buffer(rxBuffer),
        Frame::Frame_Delimiter,
        boost::asio::bind_executor(
            strand,
            [self](auto ec, auto bytesRead) {
                self->handleRead(ec, bytesRead);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::handleRead(boost::system::error_code ec, size_t bytesWritten) {
    if (closing) {
        rxBuffer.clear();
        return;
    }

    if (ec.failed()) {
        if (errorHandler != nullptr) {
            errorHandler(ec);
        }
        return;
    }
    
    std::vector<uint8_t> package(rxBuffer.cbegin(), rxBuffer.cbegin() + bytesWritten);

    rxBuffer.erase(rxBuffer.cbegin(), rxBuffer.cbegin() + bytesWritten);
    
    if (frameHandler != nullptr) {
        frameHandler(std::move(package));
    }

    startRead();
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::doWrite() {
    if(closing || txDeque.empty()) {
        return;
    }

    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::async_write(
        serial,
        boost::asio::dynamic_buffer(txDeque.front()),
        boost::asio::bind_executor(
            strand,
            [self](auto ec, auto bytesWritten) {
                self->handleWrite(ec, bytesWritten);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::handleWrite(boost::system::error_code ec, size_t bytesWritten) {
    if (closing) {
        txDeque.clear();
        return;
    }
    
    if (ec) {
        if (errorHandler != nullptr) {
            errorHandler(ec);
        }
        return;
    }

    txDeque.pop_front();

    if (!txDeque.empty()) {
        doWrite();
    }
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::requestClose() noexcept {
    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::post(
        strand,
        [self] {
            self->closeOnStrand();
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::closeOnStrand() noexcept {
    if (closing) {
        return;
    }

    closing = true;

    boost::system::error_code ignored;

    serial.cancel(ignored);
    serial.close(ignored);
}

/// @brief Clase encargada de recibir y enviar datos asíncronamente mediante el puerto serial
using SerialTransport = BasicSerialTransport<boost::asio::serial_port>;

#endif // !SERIAL_TRANSPORT_HEADER
