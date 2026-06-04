#ifndef CORE_0_HEADER
#define CORE_0_HEADER

#include "UARTWrapper.hpp"
#include "hardware/uart.h"
#include "ringbuffer.hpp"
#include "hardware/sync.h"
#include "FrameWaiter.hpp"
#include "Instances.hpp"

/// @brief Clase que representa al core 0, este es el encargado del manejo total de los UART
class Core_0 final {
public:

    /// @brief Inicializa los componentes necesarios para Core 0, los errores ocurridos dentro de este se consideran irrecuperables y creashean el programa
    /// @param UART_0_RxPin Pin en donde se inicializará el pin Rx del UART 0
    /// @param UART_0_TxPin Pin en donde se inicializará el pin Tx del UART 0
    /// @param UART_1_TxPin Pin en donde se inicializará el pin Rx del UART 1
    /// @param UART_1_RxPin Pin en donde se inicializará el pin Tx del UART 1
    static void init(uint UART_0_RxPin, uint UART_0_TxPin, uint UART_1_TxPin, uint UART_1_RxPin) noexcept;

    /// @brief Función del bucle principal del core, no retorna
    [[noreturn]]
    static void main() noexcept;

private:
    static void initialize_UART_0_IRQ();
    
    static void UART_0_IRQ_Handler();
};

#endif // !CORE_0_HEADER
