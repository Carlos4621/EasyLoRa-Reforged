#ifndef INSTANCES_HEADER
#define INSTANCES_HEADER

#include "BufferSizes.hpp"
#include "UARTWrapper.hpp"
#include "FrameWaiter.hpp"
#include "hardware/sync.h"

static UARTWrapper UART_0_Wrapper{ uart0 };
static UARTWrapper UART_1_Wrapper{ uart1 };

static FrameWaiter<Rx_Buffer_Size> UART_0_Rx_FrameWaiter{ __sev };
static FrameWaiter<Tx_Buffer_Size> UART_0_Tx_FrameWaiter{ __sev };

#endif // !INSTANCES_HEADER