#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "Core_0.hpp"
#include "Core_1.hpp"
#include "FrameWaiter.hpp"
#include "pico/multicore.h"

static constexpr size_t Rx_Buffer_Size{ 512 };
static constexpr size_t Tx_Buffer_Size{ 512 };

static constexpr uint UART_0_Rx_Pin{ 0 };
static constexpr uint UART_0_Tx_Pin{ 1 };

static constexpr uint UART_1_Rx_Pin{ 2 };
static constexpr uint UART_1_Tx_Pin{ 3 };

Core_0<Rx_Buffer_Size, Tx_Buffer_Size> core0;
Core_1 core1;

void core_1_entryPoint();

int main() {
    core0.init(UART_0_Rx_Pin, UART_0_Tx_Pin, UART_1_Rx_Pin, UART_1_Tx_Pin);

    multicore_launch_core1(core_1_entryPoint);

    core0.main();
}

void core_1_entryPoint() {
    core1.main();
}
