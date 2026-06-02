#ifndef PROTOCOL_ERRORS_HEADER
#define PROTOCOL_ERRORS_HEADER

#include <cstdint>

/// @brief Tipos de error del protocolo
enum class ProtocolErrors : uint8_t {
    BufferTooSmall = 0,
    CRCMissMatch,
    FramePayloadTooSmall,
    InvalidPackageKind,
    InvalidMessageType,
    COBSRError,
    SameBufferError,
    EmptyInputBuffer,
    IncoherentFrame,
};

#endif // !PROTOCOL_ERRORS_HEADER
