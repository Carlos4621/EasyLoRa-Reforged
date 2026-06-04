#include "Core_1.hpp"

size_t Core_1::bytesWritten_m{ 0 };

void Core_1::main() noexcept {
    while (true) {
        while (UART_0_Rx_FrameWaiter.tryReadFrame(&receivedFrameCodifiedBuffer_m[0], receivedFrameCodifiedBuffer_m.size(), bytesWritten_m) == ReadFrameStatus::OK) {
            const auto receivedFrame{ 
                ProtocolDecoder::decode(std::span{receivedFrameCodifiedBuffer_m.cbegin(), bytesWritten_m}, receivedFrameBytes_m, receivedFramePayload_m) };

            if (!receivedFrame.has_value()) {
                continue; // TODO: Añadir log que indique error de decodificación
            }
            
            const auto responseFrame{ FrameResponder::dispatch(receivedFrame.value(), responseFramePayload_m, bytesWritten_m) };

            if (!responseFrame.has_value()) {
                continue; // TODO: También agregar log de error
            }

            const auto codifiedStatus{ ProtocolCodec::encode(responseFrame.value(), responseFrameBytes_m, responseFrameCodifiedBuffer_m) };
            
            if (!codifiedStatus.has_value()) {
                continue; // TODO: Otro log
            }
            
            UART_0_Tx_FrameWaiter.feed(codifiedStatus.value());
            UART_0_Tx_FrameWaiter.insertDelimiter();
        }

        __wfe();
    }
}