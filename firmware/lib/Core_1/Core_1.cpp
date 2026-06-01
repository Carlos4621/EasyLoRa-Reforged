#include "Core_1.hpp"

size_t Core_1::bytesWritten_m{ 0 };

void Core_1::main() noexcept {
    while (true) {
        while (UART_0_FrameWaiter.tryReadFrame(&codifiedBuffer_m[0], codifiedBuffer_m.size(), bytesWritten_m) == ReadFrameStatus::OK) {
            const auto receivedStatus{ ProtocolDecoder::decode(std::span{codifiedBuffer_m.cbegin(), bytesWritten_m}, frameBytes_m, framePayload_m) };

            if (!receivedStatus.has_value()) {
                continue; // TODO: Añadir log que indique error de decodificación
            }
            
            // TODO: Procesar paquete
        }

        __wfe();
    }
}