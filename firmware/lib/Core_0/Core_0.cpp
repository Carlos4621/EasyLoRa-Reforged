#include "Core_0.hpp"

void Core_0::init(uint UART_0_RxPin, uint UART_0_TxPin, uint UART_1_TxPin, uint UART_1_RxPin) noexcept {
    UART_0_Wrapper.init(PICO_DEFAULT_UART_BAUD_RATE, UART_0_RxPin, UART_0_TxPin);
    UART_1_Wrapper.init(PICO_DEFAULT_UART_BAUD_RATE, UART_1_RxPin, UART_1_TxPin);

    initialize_UART_0_IRQ();
}

void Core_0::main() noexcept {
    while (true) {
        

        __wfe();
    }
    
}

void Core_0::initialize_UART_0_IRQ() {
    UART_0_Wrapper.setHandlerForIRQ(UART_0_IRQ_Handler);

    assert(UART_0_Wrapper.enableRxIRQ(true) && "Error while UART 0 Rx IRQ enable");
}

inline void Core_0::UART_0_IRQ_Handler() {
    while (UART_0_Wrapper.isReadable()) {
        UART_0_FrameWaiter.feed(UART_0_Wrapper.read());
    }
}