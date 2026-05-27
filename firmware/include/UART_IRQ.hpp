#ifndef UART_IRQ_HEADER
#define UART_IRQ_HEADER

#include "Instances.hpp"
#include <cassert>
#include "hardware/sync.h"

// TODO: Log de debug para error en assert
static void initialize_UART_0_IRQ(uint rxPin, uint txPin) {
    if (!UART_0_Wrapper.isInitializated()) {
        assert(UART_0_Wrapper.init(Default_Baudrate, rxPin, txPin) && "Error while UART 0 IRQ Rx initialization");
    }

    UART_0_Wrapper.setHandlerForIRQ(UART_0_IRQ_Handler);

    assert(UART_0_Wrapper.enableRxIRQ(true) && "Error while UART 0 Rx IRQ enable");
}

static void UART_0_IRQ_Handler() {
    while (UART_0_Wrapper.isReadable()) {
        UART_0_FameWaiter.feed(UART_0_Wrapper.read());
    }
}

#endif // !UART_IRQ_HEADER