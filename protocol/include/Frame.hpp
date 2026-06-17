#ifndef FRAME_HEADER
#define FRAME_HEADER

#include <cstddef>
#include <cstdint>
#include <span>
#include "PackageKind.hpp"
#include "MessageType.hpp"

/// @brief Estructura con los datos necesarios para un Frame
struct Frame {
    uint8_t version;
    PackageKind kind;
    uint8_t flags; // Placeholder
    uint8_t reserved; // Placeholder
    uint16_t seq;
    MessageType type;
    std::span<const uint8_t> payload;

    static constexpr size_t Header_Size { 
        sizeof(version) +
        sizeof(kind) +
        sizeof(flags) +
        sizeof(reserved) +
        sizeof(seq) +
        sizeof(type)
    };

    static constexpr uint8_t CRC_Size{ 2 };

    static constexpr uint8_t Actual_Frame_Version{ 1 };

    static constexpr uint8_t Frame_Delimiter{ 0x00 };
};

#endif // !FRAME_HEADER