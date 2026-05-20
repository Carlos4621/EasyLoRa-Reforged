#ifndef WRAPPER_UART_HEADER
#define WRAPPER_UART_HEADER

#include "ByteTransport.hpp"
#include "hardware/uart.h"
#include <cstdint>
#include <expected>
#include <cstddef>
#include <optional>
#include <array>
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "ValidPins.hpp"

enum class UARTErrors : uint8_t {
    uartNotInitializated = 0,
    baudRateDontSupported,
    irqHandlerNotSetted,
    gpioNotSupportUartRx,
    gpioNotSupportUartTx,
};

/// @brief Wrapper de utilidad para UART de hardware
class UARTWrapper : public ByteTransport {
public:

    /// @brief Constructor base
    /// @param uart UART que será usado
    explicit UARTWrapper(uart_inst_t* const uart);

    /// @brief Llama a uart_init con el UART dado en el constructor
    /// @param baudRate Baudrate a colocar
    /// @return std::expected con uint indicando el baudrate colocado.
    ///         UARTErrors en caso de error.
    ///         Si hay error NO SE GARANTIZA QUE EL OBJETO FUNCIONA
    std::expected<uint, UARTErrors> init(uint baudrate, uint8_t rxPin, uint8_t txPin) const;

    /// @brief Lee un byte del FIFO Rx. Bloque hasta que se lee el byte
    /// @return Byte leído
    uint8_t read() override;

    /// @brief Escribe un byte en el FIFO Tx. Bloque hasta enviar el byte
    /// @param byte Byte leído
    void write(uint8_t byte) override;

    /// @brief Comprueba si el puerto UART está inicializado
    /// @return true en caso afirmativo, false sino
    [[nodiscard]]
    bool isInitializated() const;

    /// @brief Devuelve el puntero al uart usuado al inicializar el objeto.
    ///        Cabe mencionar que los cambios realizados directamente con este puntero pueden ser invisibles para el objeto.
    /// @return Puntero al uart usado
    [[nodiscard]]
    uart_inst_t* getUARTInstance();

    /// @brief Comprueba si hay bytes esperando en Rx
    /// @return true en caso afirmativo, false sino
    [[nodiscard]]
    bool isReadable() const;

    /// @brief Activa o desactiva si se manda una señal IRQ al recibir bytes-
    /// Se debe colocar un handler a la IRQ antes de activarse
    /// @param enable Estado a colocar
    /// @return std::expected con void en caso de exito.
    ///         UARTErrors::irqHandlerNotSetted en caso de no haber handler en el IRQ
    [[nodiscard]]
    std::expected<void, UARTErrors> enableRxIRQ(bool enable);

    /// @brief Activa o desactiva si se manda una señal IRQ al quedarse vacío el FIFO de Tx
    /// Se debe colocar un handler a la IRQ antes de activarse
    /// @param enable Estado a colocar
    /// @return std::expected con void en caso de exito.
    ///         UARTErrors::irqHandlerNotSetted en caso de no haber handler en el IRQ
    [[nodiscard]]
    std::expected<void, UARTErrors> enableTxIRQ(bool enable);

    /// @brief Coloca un handler al IRQ. Debe ser colocado antes de habilitar las IRQ
    /// @param function Función handler a colocar
    void setHandlerForIRQ(void(*function)()) const;

private:
    uart_inst_t* const uart_m;

    bool RxIRQEnabled{ false };
    bool TxIRQEnabled{ false };

    [[nodiscard]]
    bool isIRQHandlerSetted();

    [[nodiscard]]
    std::expected<void, UARTErrors> enableIRQ(bool& IRQToEnable, bool enable);
};

#endif // !WRAPPER_UART_HEADER