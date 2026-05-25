#ifndef INSTANCES_HEADER
#define INSTANCES_HEADER

#include "UARTWrapper.hpp"
#include "hardware/uart.h"
#include "ringbuffer.hpp"
#include "hardware/sync.h"
#include "FrameWaiter.hpp"

/*
    Este archivo contiene las intancias globales estáticas de los wrappers y buffers
*/

static constexpr uint32_t Default_Baudrate{ 9600 };

static UARTWrapper UART_0_Wrapper{ uart0 };
static UARTWrapper UART_1_Wrapper{ uart1 };

static constexpr size_t Tx_Buffer_Size{ 512 };
static constexpr size_t Rx_Buffer_Size{ 512 };

static jnk0le::Ringbuffer<uint8_t, Tx_Buffer_Size> Tx_Buffer;
static FrameWaiter<Rx_Buffer_Size> Rx_FrameWaiter{ __sev };

#endif // !INSTANCES_HEADER