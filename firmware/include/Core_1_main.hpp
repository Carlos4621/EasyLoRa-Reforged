#ifndef CORE_1_MAIN_HEADER
#define CORE_1_MAIN_HEADER

#include "Instances.hpp"
#include "hardware/sync.h"
#include <array>

static constexpr size_t Core_1_InternalBufferSize{ 512 };

static void core_1_main() {
    std::array<uint8_t, Core_1_InternalBufferSize> buffer;
    size_t bytesWritten{ 0 };

    while (true) {
        while (Rx_FrameWaiter.tryReadFrame(&buffer[0], buffer.size(), bytesWritten) == ReadFrameStatus::OK) {
            // Procesar paquete
        }

        __wfe();
    }
}

#endif // !CORE_1_MAIN_HEADER