#ifndef CORE_1_MAIN_HEADER
#define CORE_1_MAIN_HEADER

#include "Instances.hpp"
#include "hardware/sync.h"
#include <array>
#include "ProtocolDecoder.hpp"

static constexpr size_t Core_1_InternalBufferSize{ 512 };

static void core_1_main() {
    std::array<uint8_t, Core_1_InternalBufferSize> UART_0_Buffer;
    size_t bytesWritten{ 0 };

    while (true) {
        while (UART_0_FameWaiter.tryReadFrame(&UART_0_Buffer[0], UART_0_Buffer.size(), bytesWritten) == ReadFrameStatus::OK) {
            // Procesar Paquete
        }

        __wfe();
    }
}

#endif // !CORE_1_MAIN_HEADER