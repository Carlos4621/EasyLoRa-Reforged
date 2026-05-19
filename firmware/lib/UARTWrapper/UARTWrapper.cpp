#include "UARTWrapper.hpp"

UARTWrapper::UARTWrapper(uart_inst_t* const uart)
: uart_m{ uart }
{
    assert(uart_m != nullptr && "La dirección del UART no puede ser nullptr");
}

std::expected<uint, UARTErrors> UARTWrapper::init(uint baudrate) {
    const auto settedBaudrate(uart_init(uart_m, baudrate));

    if (settedBaudrate == 0) {
        return std::unexpected(UARTErrors::BaudRateDontSupported);
    }

    if (!isInitializated()) {
        return std::unexpected(UARTErrors::UARTNotInitializated);
    }
    
    return {};
}

uint8_t UARTWrapper::read() {
    return static_cast<uint8_t>(uart_getc(uart_m));
}

void UARTWrapper::write(uint8_t byte) {
    uart_putc_raw(uart_m, byte);
}

bool UARTWrapper::isInitializated() {
    return uart_is_enabled(uart_m);
}

uart_inst_t* UARTWrapper::getUARTInstance() {
    return uart_m;
}

bool UARTWrapper::isReadable() {
    return uart_is_readable(uart_m);
}

std::expected<void, UARTErrors> UARTWrapper::enableRxIRQ(bool enable) {
    return enableIRQ(RxIRQEnabled, enable);
}

std::expected<void, UARTErrors> UARTWrapper::enableTxIRQ(bool enable) {
    return enableIRQ(TxIRQEnabled, enable);
}

void UARTWrapper::setHandlerForIRQ(void (*function)()) {
    irq_set_exclusive_handler(UART_IRQ_NUM(uart_m), function);
}

bool UARTWrapper::isIRQHandlerSetted() {
    return irq_get_exclusive_handler(UART_IRQ_NUM(uart_m)) != nullptr;
}

std::expected<void, UARTErrors> UARTWrapper::enableIRQ(bool &IRQToEnable, bool enable) {
    if (enable && !isIRQHandlerSetted())  {
        return std::unexpected(UARTErrors::IRQHandlerNotSetted);
    }

    IRQToEnable = enable;
    irq_set_enabled(UART_IRQ_NUM(uart_m), RxIRQEnabled || TxIRQEnabled);
    uart_set_irqs_enabled(uart_m, RxIRQEnabled, TxIRQEnabled);

    return {};
}
