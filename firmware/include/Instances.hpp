#ifndef INSTANCES_HEADER
#define INSTANCES_HEADER

#include "BufferSizes.hpp"
#include "UARTWrapper.hpp"
#include "FrameWaiter.hpp"
#include "TxFrame.hpp"
#include "hardware/sync.h"

static UARTWrapper UART_0_Wrapper{ uart0 };
static UARTWrapper UART_1_Wrapper{ uart1 };

static FrameWaiter<Rx_Buffer_Size> UART_0_FrameWaiter{ __sev };
static jnk0le::Ringbuffer<TxFrame, Tx_Buffer_Frame_Instances_Size> UART_0_Tx_Buffer;

#endif // !INSTANCES_HEADER