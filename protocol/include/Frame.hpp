#ifndef FRAME_HEADER
#define FRAME_HEADER

#include <cstdint>
#include <span>
#include "PackageKind.hpp"
#include "MessageType.hpp"

struct Frame {
    uint8_t version;
    PackageKind kind;
    uint8_t flags;
    uint8_t reserved;
    uint16_t seq;
    MessageType type;
    std::span<uint8_t> payload;

    static constexpr size_t Header_Size { 
        sizeof(version) +
        sizeof(kind) +
        sizeof(flags) +
        sizeof(reserved) +
        sizeof(seq) +
        sizeof(type)
    };

    static constexpr uint8_t CRC_Size{ 2 };
};

#endif // !FRAME_HEADER