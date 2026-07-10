#ifndef SERIAL_TRANSPORT_TYPES_HEADER
#define SERIAL_TRANSPORT_TYPES_HEADER

#include <cstddef>
#include <cstdint>
#include <boost/asio.hpp>

/// @brief Posibles estados del envío de mensajes
enum class WriteStatus : uint8_t {
    Scheduled,
    Closed,
    QueueFull,
    MessageTooLong,
};

/// @brief Posibles resultados de escritura
enum class WriteResult : uint8_t {
    Written,
    Failed,
    Cancelled,
};

/// @brief Errores fatales que provocan el cierre del flujo de datos
enum class FatalErrors : uint8_t {
    UnpluggedDevice,
    PermissionDenied,
    ExternallyClosedPort,
};

/// @brief Errores que pueden ser manejados internamente
enum class RecoverableErrors : uint8_t {
    RxBufferFull,
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

#endif // !SERIAL_TRANSPORT_TYPES_HEADER
