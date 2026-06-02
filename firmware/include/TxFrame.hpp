#ifndef TX_FRAME_HEADER
#define TX_FRAME_HEADER

#include "BufferSizes.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

struct TxFrame {
    std::array<uint8_t, Rx_Buffer_Size> data;
    size_t size;
};

#endif // !TX_FRAME_HEADER