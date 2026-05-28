#include "Core_1.hpp"

void Core_1::main() noexcept {
    while (true) {
        while (UART_0_FameWaiter.tryReadFrame(&codifiedBuffer_m[0], codifiedBuffer_m.size(), bytesWritten_m) == ReadFrameStatus::OK) {
            const auto receivedStatus{ ProtocolDecoder::decode(std::span{codifiedBuffer_m, bytesWritten_m}, frameBytes_m, framePayload_m) };

            if (!receivedStatus.has_value()) {
                continue; // TODO: Añadir log que indique error de decodificación
            }
            

        }

        __wfe();
    }
}