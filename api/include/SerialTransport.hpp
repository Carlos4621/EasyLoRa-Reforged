#ifndef SERIAL_TRANSPORT_HEADER
#define SERIAL_TRANSPORT_HEADER

#include "boost/asio.hpp"
#include <string_view>
#include <functional>
#include <vector>
#include <deque>
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

    /// @brief Abre el puerto seleccionas con las opciones dadas y empieza a escuchar
    /// @param portPath Dirección del puerto a abrir
    /// @param config Configuración a usar. Por defecto 9600 8N1
    void open(std::string portPath, SerialConfig config = Default_Serial_Config);

    /// @brief Cancela todas las operaciones pendientes y limpia los buffers
    void close();

    /// @brief Establece la función a la que se llamará con los datos cuando se reciba un frame completo
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(std::vector<uint8_t>)
    void setFrameHandler(const FrameHandler& handler) noexcept;

    /// @brief Establece la función a la que se llamará con los datos cuando se produzca un error
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(boost::system::error_code)
    void setErrorHandler(const ErrorHandler& handler) noexcept;

    /// @brief Escribe asincrónicamente los bytes daos
    /// @param toWrite Bytes a escribir
    /// @note Se usa un sistema de colas para situaciones donde el último mensaje no se ha enviado aún y se llama de nuevo a esta función
    void asyncWrite(std::vector<uint8_t> toWrite);

private:
    boost::asio::strand<boost::asio::io_context::executor_type> strand_m;
    Stream serial_m;

    FrameHandler frameHandler_m{ nullptr };
    ErrorHandler errorHandler_m{ nullptr };

    std::vector<uint8_t> rxBuffer_m;
    std::deque<std::vector<uint8_t>> txDeque_m;

    void startRead();
    void handleRead(boost::system::error_code ec, size_t bytesWritten);

    void doWrite();
    void handleWrite(boost::system::error_code ec, size_t bytesWritten);
};

template <class Stream>
inline BasicSerialTransport<Stream>::BasicSerialTransport(boost::asio::io_context &context) 
    : strand_m{ context.get_executor() }
    , serial_m{ context }
{
}

template <class Stream>
inline void BasicSerialTransport<Stream>::open(std::string portPath, SerialConfig config) {
    boost::asio::post(
        strand_m,
        [this, config = std::move(config), portPath = std::move(portPath)] {
            serial_m.open(portPath.data());

            serial_m.set_option(boost::asio::serial_port_base::baud_rate(config.baudRate));
            serial_m.set_option(boost::asio::serial_port_base::character_size(config.characterSize));
            serial_m.set_option(boost::asio::serial_port_base::parity(config.parityByte));
            serial_m.set_option(boost::asio::serial_port_base::stop_bits(config.stopBits));
            serial_m.set_option(boost::asio::serial_port_base::flow_control(config.flowControl));

            startRead();
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::close() {
    boost::asio::post(
        strand_m, 
        [this] {
            boost::system::error_code ignored;

            serial_m.cancel(ignored);
            serial_m.close(ignored);

            txDeque_m.clear();
            rxBuffer_m.clear();
    });
}

template<class Stream>
inline void BasicSerialTransport<Stream>::setFrameHandler(const FrameHandler & handler) noexcept {
    frameHandler_m = handler;
}

template <class Stream>
inline void BasicSerialTransport<Stream>::setErrorHandler(const ErrorHandler &handler) noexcept {
    errorHandler_m = handler;
}

template <class Stream>
inline void BasicSerialTransport<Stream>::asyncWrite(std::vector<uint8_t> toWrite) {
    boost::asio::post(
        strand_m,
        [this, toWrite = std::move(toWrite)] {
            const bool idle{ txDeque_m.empty() };

            txDeque_m.emplace_back(std::move(toWrite));
    
            if (idle) {
                doWrite();
            }
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::startRead() {
    boost::asio::async_read_until(
        serial_m,
        boost::asio::dynamic_buffer(rxBuffer_m),
        Frame::Frame_Delimiter,
        boost::asio::bind_executor(
            strand_m,
            [this](boost::system::error_code ec, std::size_t n) {
                handleRead(ec, n);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::handleRead(boost::system::error_code ec, size_t bytesWritten) {
    if (ec.failed()) {
        if (errorHandler_m != nullptr) {
            errorHandler_m(ec);
        }
        return;
    }
    
    std::vector<uint8_t> package(rxBuffer_m.cbegin(), rxBuffer_m.cbegin() + bytesWritten);

    rxBuffer_m.erase(rxBuffer_m.cbegin(), rxBuffer_m.cbegin() + bytesWritten);
    
    if (frameHandler_m != nullptr) {
        frameHandler_m(std::move(package));
    }

    startRead();
}

template <class Stream>
inline void BasicSerialTransport<Stream>::doWrite() {
    if(txDeque_m.empty()) {
        return;
    }

    boost::asio::async_write(
        serial_m,
        boost::asio::dynamic_buffer(txDeque_m.front()),
        boost::asio::bind_executor(
            strand_m,
            [this](boost::system::error_code ec, size_t bytesWritten) {
                handleWrite(ec, bytesWritten);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::handleWrite(boost::system::error_code ec, size_t bytesWritten) {
    if (ec) {
        if (errorHandler_m != nullptr) {
            errorHandler_m(ec);
        }
        return;
    }

    txDeque_m.pop_front();

    if (!txDeque_m.empty()) {
        doWrite();
    }
}

using SerialTransport = BasicSerialTransport<boost::asio::serial_port>;

#endif // !SERIAL_TRANSPORT_HEADER