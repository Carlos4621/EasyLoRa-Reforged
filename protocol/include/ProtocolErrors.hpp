#ifndef PROTOCOL_ERRORS_HEADER
#define PROTOCOL_ERRORS_HEADER

#include <cstdint>

/// @brief Tipos de error del protocolo
enum class ProtocolErrors : uint8_t {
    OutputBufferTooSmall = 0,
    CRCMismatch,
    FramePayloadTooSmall,
    FramePayloadTooLong,
    InvalidPackageKind,
    InvalidMessageType,
    COBSREncodeError,
    COBSRDecodeError,
    SameBufferError,
    EmptyInputBuffer,
    IncoherentFrame,
    CodificationError,
    HandlerWithIncorrectType,
    FrameVersionMismatch,
    InputBufferTooLong,
    InputBufferTooSmall,
};

#endif // !PROTOCOL_ERRORS_HEADER
