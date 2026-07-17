#ifndef PROTOCOL_ERRORS_HEADER
#define PROTOCOL_ERRORS_HEADER

#include <cstdint>

namespace protocol {

    /**
     * @brief Posibles errores del protocolo
     * 
    */
    enum class ProtocolErrors : uint8_t {
        InputBufferTooSmall = 0,
        OutputBufferTooSmall,
        CrcMismatch,
    };
}

#endif // !PROTOCOL_ERRORS_HEADER