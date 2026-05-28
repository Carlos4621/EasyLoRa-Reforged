#ifndef CORE_0_HEADER
#define CORE_0_HEADER

#include "UARTWrapper.hpp"
#include "hardware/uart.h"
#include "ringbuffer.hpp"
#include "hardware/sync.h"
#include "FrameWaiter.hpp"

/// @brief Clase que representa al core 0, este es el encargado del manejo total de los UART
template<size_t RxBufferSize, size_t TxBufferSize>
class Core_0 final {
public:

    static void init(uint UART_0_RxPin, uint UART_0_TxPin, uint UART_1_TxPin, uint UART_1_RxPin) noexcept;

    /// @brief Función del bucle principal del core
    [[noreturn]]
    static void main() noexcept;

private:
    static UARTWrapper UART_0_Wrapper;
    static UARTWrapper UART_1_Wrapper;

    static FrameWaiter<RxBufferSize> UART_0_FrameWaiter;

    static void initialize_UART_0_IRQ();
    
    static void UART_0_IRQ_Handler();
};

template <size_t RxBufferSize, size_t TxBufferSize>
UARTWrapper Core_0<RxBufferSize, TxBufferSize>::UART_0_Wrapper{ uart0 };

template <size_t RxBufferSize, size_t TxBufferSize>
UARTWrapper Core_0<RxBufferSize, TxBufferSize>::UART_1_Wrapper{ uart1 };

template <size_t RxBufferSize, size_t TxBufferSize>
FrameWaiter<RxBufferSize> Core_0<RxBufferSize, TxBufferSize>::UART_0_FrameWaiter{ __sev };

template <size_t RxBufferSize, size_t TxBufferSize>
inline void Core_0<RxBufferSize, TxBufferSize>::init(uint UART_0_RxPin, uint UART_0_TxPin, uint UART_1_TxPin, uint UART_1_RxPin) noexcept {
    UART_0_Wrapper.init(PICO_DEFAULT_UART_BAUD_RATE, UART_0_RxPin, UART_0_TxPin);
    UART_1_Wrapper.init(PICO_DEFAULT_UART_BAUD_RATE, UART_1_RxPin, UART_1_TxPin);

    initialize_UART_0_IRQ();
}

template <size_t RxBufferSize, size_t TxBufferSize>
inline void Core_0<RxBufferSize, TxBufferSize>::main() noexcept {
    
}

template <size_t RxBufferSize, size_t TxBufferSize>
inline void Core_0<RxBufferSize, TxBufferSize>::initialize_UART_0_IRQ() {
    UART_0_Wrapper.setHandlerForIRQ(UART_0_IRQ_Handler);

    assert(UART_0_Wrapper.enableRxIRQ(true) && "Error while UART 0 Rx IRQ enable");
}

template <size_t RxBufferSize, size_t TxBufferSize>
inline void Core_0<RxBufferSize, TxBufferSize>::UART_0_IRQ_Handler() {
    while (UART_0_Wrapper.isReadable()) {
        UART_0_FameWaiter.feed(UART_0_Wrapper.read());
    }
}

#endif // !CORE_0_HEADER
