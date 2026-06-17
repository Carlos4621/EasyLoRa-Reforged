#ifndef SERIAL_TRANSPORT_HEADER
#define SERIAL_TRANSPORT_HEADER

#include "boost/asio.hpp"
#include <string_view>
#include <functional>
#include <vector>
#include <deque>
#include "Frame.hpp"

/// @brief Configuraciones para el puerto serial abierto con SerialTransport
struct SerialConfig {
    uint32_t baudRate;
    boost::asio::serial_port_base::flow_control::type flowControl;
    boost::asio::serial_port_base::parity::type parityByte;
    boost::asio::serial_port_base::stop_bits::type stopBits;
    uint32_t characterSize;
};

/// @brief Configuración default del puerto serial
static constexpr SerialConfig Default_Serial_Config{ 
    .baudRate = 9600, 
    .flowControl = boost::asio::serial_port_base::flow_control::type::none,
    .parityByte = boost::asio::serial_port_base::parity::type::none,
    .stopBits = boost::asio::serial_port::stop_bits::type::one,
    .characterSize = 8
};

/// @brief Clase encargada de recibir asíncronamente bytes y notificar al usuario para su posterior procesamiento
class SerialTransport {
public:
    using FrameHandler = std::function<void(std::vector<uint8_t>)>;
    using ErrorHandler = std::function<void(boost::system::error_code)>;

    /// @brief Constructor base
    /// @param context Contexto de I/O
    explicit SerialTransport(boost::asio::io_context& context);

    /// @brief Abre el puerto seleccionas con las opciones dadas y empieza a escuchar
    /// @param portPath Dirección del puerto a abrir
    /// @param config Configuración a usar. Por defecto 9600 8N1
    void open(std::string_view portPath, const SerialConfig& config = Default_Serial_Config);

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
    boost::asio::serial_port serial_m;

    FrameHandler frameHandler_m{ nullptr };
    ErrorHandler errorHandler_m{ nullptr };

    std::vector<uint8_t> rxBuffer_m;
    std::deque<std::vector<uint8_t>> txDeque_m;

    void startRead();
    void handleRead(boost::system::error_code ec, size_t bytesWritten);

    void doWrite();
    void handleWrite(boost::system::error_code ec, size_t bytesWritten);
};

#endif // !SERIAL_TRANSPORT_HEADER