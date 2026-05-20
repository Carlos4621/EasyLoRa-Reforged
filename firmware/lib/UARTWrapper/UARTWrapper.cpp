#include "UARTWrapper.hpp"

UARTWrapper::UARTWrapper(uart_inst_t* const uart)
: uart_m{ uart }
{
    assert(uart_m != nullptr && "La dirección del UART no puede ser nullptr");
}

std::expected<uint, UARTErrors> UARTWrapper::init(uint baudrate, uint8_t rxPin, uint8_t txPin) const {
    const auto settedBaudrate(uart_init(uart_m, baudrate));

    if (settedBaudrate == 0) {
        return std::unexpected(UARTErrors::baudRateDontSupported);
    }

    if (!isInitializated()) {
        return std::unexpected(UARTErrors::uartNotInitializated);
    }

    if (!isValidUART_Rx(UART_NUM(uart_m), rxPin)) {
        return std::unexpected(UARTErrors::gpioNotSupportUartRx);
    }

    if (!isValidUART_Tx(UART_NUM(uart_m), txPin)) {
        return std::unexpected(UARTErrors::gpioNotSupportUartTx);
    }

    gpio_set_function(rxPin, gpio_function_t::GPIO_FUNC_UART);
    gpio_set_function(txPin, gpio_function_t::GPIO_FUNC_UART);

    return {};
}

uint8_t UARTWrapper::read() {
    return static_cast<uint8_t>(uart_getc(uart_m));
}

void UARTWrapper::write(uint8_t byte) {
    uart_putc_raw(uart_m, byte);
}

bool UARTWrapper::isInitializated() const {
    return uart_is_enabled(uart_m);
}

uart_inst_t* UARTWrapper::getUARTInstance() {
    return uart_m;
}

bool UARTWrapper::isReadable() const {
    return uart_is_readable(uart_m);
}

std::expected<void, UARTErrors> UARTWrapper::enableRxIRQ(bool enable) {
    return enableIRQ(RxIRQEnabled, enable);
}

std::expected<void, UARTErrors> UARTWrapper::enableTxIRQ(bool enable) {
    return enableIRQ(TxIRQEnabled, enable);
}

void UARTWrapper::setHandlerForIRQ(void (*function)()) const {
    irq_set_exclusive_handler(UART_IRQ_NUM(uart_m), function);
}

bool UARTWrapper::isIRQHandlerSetted() {
    return irq_get_exclusive_handler(UART_IRQ_NUM(uart_m)) != nullptr;
}

std::expected<void, UARTErrors> UARTWrapper::enableIRQ(bool &IRQToEnable, bool enable) {
    if (enable && !isIRQHandlerSetted())  {
        return std::unexpected(UARTErrors::irqHandlerNotSetted);
    }

    IRQToEnable = enable;
    irq_set_enabled(UART_IRQ_NUM(uart_m), RxIRQEnabled || TxIRQEnabled);
    uart_set_irqs_enabled(uart_m, RxIRQEnabled, TxIRQEnabled);

    return {};
}