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

/// @brief Posibles estados del envío de mensajes
enum class WriteStatus : uint8_t {
    AddedToQueue,
    MessageTooLong,
};

/// @brief Configuraciones para el puerto serial abierto con BasicSerialTransport
struct SerialConfig {
    uint32_t baudRate;
    boost::asio::serial_port_base::flow_control::type flowControl;
    boost::asio::serial_port_base::parity::type parityByte;
    boost::asio::serial_port_base::stop_bits::type stopBits;
    uint32_t characterSize;
};

/// @brief Configuraciones para los buffers usados con BasicSerialTransport
struct BufferConfig {
    size_t rxBufferMaxSize;
    size_t txMaxElementsInQueue;
    size_t txMaxSizePerElement;
};

/// @brief Configuración default del puerto serial (9600 8N1)
static constexpr SerialConfig Default_Serial_Config{ 
    .baudRate = 9600, 
    .flowControl = boost::asio::serial_port_base::flow_control::type::none,
    .parityByte = boost::asio::serial_port_base::parity::type::none,
    .stopBits = boost::asio::serial_port::stop_bits::type::one,
    .characterSize = 8
};

/// @brief Configuración default de los buffers
static constexpr BufferConfig Default_Buffer_Config {
    .rxBufferMaxSize = 1024,
    .txMaxElementsInQueue = 10,
    .txMaxSizePerElement = 512
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

    /// @brief Abre el puerto seleccionado con las opciones dadas y empieza a escuchar, llamando a FrameHandler en caso de frame recibido
    /// @param portPath Dirección del puerto a abrir
    /// @param serialConfig Configuración del puerto serial a usar. Por defecto 9600 8N1
    /// @param bufferConfig Configuración de los tamaños máximos de los buffers internos, por defecto rxBuffer = 1024, txInQueue = 10, txMaxSize = 512
    /// @note Reinicia el transporte después de un error de I/O.
    void open(std::string portPath, SerialConfig serialConfig = Default_Serial_Config, BufferConfig bufferConfig = Default_Buffer_Config);

    /// @brief Solicita un cierre y cancelación de procesos, estos serán realizados ASAP
    void close();

    /// @brief Establece la función a la que se llamará con los datos cuando se reciba un frame completo
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(std::vector<uint8_t>)
    void setFrameHandler(FrameHandler handler) noexcept;

    /// @brief Establece la función a la que se llamará con los datos cuando se produzca un error
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(boost::system::error_code)
    void setErrorHandler(ErrorHandler handler) noexcept;

    /// @brief Escribe asincrónicamente los bytes dados
    /// @param toWrite Bytes a escribir
    /// @note Se usa un sistema de colas para situaciones donde el último mensaje no se ha enviado aún y se llama de nuevo a esta función
    /// @note Las escrituras se descartan si ocurre un error de I/O; se requiere open() para reiniciar el transporte.
    [[nodiscard]]
    WriteStatus asyncWrite(std::vector<uint8_t> toWrite);

private:
    struct State;

    std::shared_ptr<State> state_m;
};

template <class Stream>
struct BasicSerialTransport<Stream>::State : std::enable_shared_from_this<State> {
    explicit State(boost::asio::io_context& context);

    boost::asio::strand<boost::asio::io_context::executor_type> strand;

    Stream serial;

    BufferConfig bufferConfig;
    std::vector<uint8_t> rxBuffer;
    std::deque<std::vector<uint8_t>> txDeque;

    FrameHandler frameHandler{ nullptr };
    ErrorHandler errorHandler{ nullptr };
    
    bool closing{ false };
    bool stopped{ false };
    size_t generation{ 0 };

    void open(std::string portPath, SerialConfig serialConfig, BufferConfig bufferConfig);
    void close();

    void setFrameHandler(FrameHandler handler) noexcept;
    void setErrorHandler(ErrorHandler handler) noexcept;

    [[nodiscard]]
    WriteStatus asyncWrite(std::vector<uint8_t> toWrite);

    void startRead();
    void handleRead(boost::system::error_code ec, size_t bytesRead, size_t operationGeneration);

    void doWrite();
    void handleWrite(boost::system::error_code ec, size_t bytesWritten, size_t operationGeneration);

    void stopAfterIoError() noexcept;

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
inline void BasicSerialTransport<Stream>::open(std::string portPath, SerialConfig serialConfig, BufferConfig bufferConfig) {
    state_m->open(std::move(portPath), std::move(serialConfig), std::move(bufferConfig));
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
inline WriteStatus BasicSerialTransport<Stream>::asyncWrite(std::vector<uint8_t> toWrite) {
    return state_m->asyncWrite(std::move(toWrite));
}

template <class Stream>
inline BasicSerialTransport<Stream>::State::State(boost::asio::io_context &context)
    : strand{ context.get_executor() }
    , serial{ context }
{
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::open(std::string portPath, SerialConfig serialConfig, BufferConfig bufferConfig) {
    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() }; 

    boost::asio::post(
        strand,
        [self, serialConfig = std::move(serialConfig), portPath = std::move(portPath), bufferConfig = std::move(bufferConfig)] {
            if (self->closing) {
                return;
            }

            ++self->generation;
            self->stopped = false;
            self->rxBuffer.clear();
            self->txDeque.clear();

            self->bufferConfig = std::move(bufferConfig);

            self->rxBuffer.reserve(bufferConfig.rxBufferMaxSize);

            self->serial.open(portPath.data());

            self->serial.set_option(boost::asio::serial_port_base::baud_rate(serialConfig.baudRate));
            self->serial.set_option(boost::asio::serial_port_base::character_size(serialConfig.characterSize));
            self->serial.set_option(boost::asio::serial_port_base::parity(serialConfig.parityByte));
            self->serial.set_option(boost::asio::serial_port_base::stop_bits(serialConfig.stopBits));
            self->serial.set_option(boost::asio::serial_port_base::flow_control(serialConfig.flowControl));

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
inline WriteStatus BasicSerialTransport<Stream>::State::asyncWrite(std::vector<uint8_t> toWrite) {
    if (toWrite.size() > bufferConfig.txMaxSizePerElement) {
        return WriteStatus::MessageTooLong;
    }

    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };

    boost::asio::post(
        strand,
        [self, toWrite = std::move(toWrite)] mutable {
            if (self->closing || self->stopped) {
                return;
            }

            if (self->txDeque.size() >= self->bufferConfig.txMaxElementsInQueue) {
                errorHandler(boost::asio::error::no_buffer_space);
                return;
            }

            const bool idle{ self->txDeque.empty() };

            self->txDeque.emplace_back(std::move(toWrite));
    
            if (idle) {
                self->doWrite();
            }
        }
    );

    return WriteStatus::AddedToQueue;
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::startRead() {
    if (closing || stopped) {
        return;
    }

    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };
    const auto operationGeneration{ generation };

    boost::asio::async_read_until(
        serial,
        boost::asio::dynamic_buffer(rxBuffer, bufferConfig.rxBufferMaxSize),
        Frame::Frame_Delimiter,
        boost::asio::bind_executor(
            strand,
            [self, operationGeneration](auto ec, auto bytesRead) {
                self->handleRead(ec, bytesRead, operationGeneration);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::handleRead(boost::system::error_code ec, size_t bytesRead, size_t operationGeneration) {
    if (closing || stopped || operationGeneration != generation) {
        return;
    }

    if (ec) {
        stopAfterIoError();

        if (errorHandler != nullptr) {
            errorHandler(ec);
        }

        return;
    }

    std::vector<uint8_t> package(rxBuffer.cbegin(), rxBuffer.cbegin() + bytesRead);

    if (frameHandler != nullptr) {
        frameHandler(std::move(package));
    }

    rxBuffer.erase(rxBuffer.cbegin(), rxBuffer.cbegin() + bytesRead);

    startRead();
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::doWrite() {
    if(closing || stopped || txDeque.empty()) {
        return;
    }

    auto self{ BasicSerialTransport<Stream>::State::shared_from_this() };
    const auto operationGeneration{ generation };

    boost::asio::async_write(
        serial,
        boost::asio::dynamic_buffer(txDeque.front()),
        boost::asio::bind_executor(
            strand,
            [self, operationGeneration](auto ec, auto bytesWritten) {
                self->handleWrite(ec, bytesWritten, operationGeneration);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::State::handleWrite(
    boost::system::error_code ec,
    size_t bytesWritten,
    size_t operationGeneration
) {
    if (closing || stopped || operationGeneration != generation) {
        return;
    }

    if (ec) {
        stopAfterIoError();

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
inline void BasicSerialTransport<Stream>::State::stopAfterIoError() noexcept {
    stopped = true;
    ++generation;
    rxBuffer.clear();
    txDeque.clear();

    boost::system::error_code ignored;
    serial.cancel(ignored);
    serial.close(ignored);
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
    stopped = true;
    ++generation;
    rxBuffer.clear();
    txDeque.clear();

    boost::system::error_code ignored;

    serial.cancel(ignored);
    serial.close(ignored);
}

/// @brief Clase encargada de recibir y enviar datos asíncronamente mediante el puerto serial
using SerialTransport = BasicSerialTransport<boost::asio::serial_port>;

#endif // !SERIAL_TRANSPORT_HEADER
