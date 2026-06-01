#ifndef INSTANCES_HEADER
#define INSTANCES_HEADER

#include "UARTWrapper.hpp"
#include "FrameWaiter.hpp"
#include "TxFrame.hpp"

static constexpr size_t Rx_Buffer_Size{ 512 };
static constexpr size_t Tx_Buffer_Frame_Instances_Size{ 8 };

static UARTWrapper UART_0_Wrapper{ uart0 };
static UARTWrapper UART_1_Wrapper{ uart1 };

static FrameWaiter<Rx_Buffer_Size> UART_0_FrameWaiter{ __sev };
static jnk0le::Ringbuffer<TxFrame, Tx_Buffer_Frame_Instances_Size> UART_0_Tx_Buffer;

#endif // !INSTANCES_HEADER