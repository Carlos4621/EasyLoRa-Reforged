#ifndef PROTOCOL_DECODER_HEADER
#define PROTOCOL_DECODER_HEADER

#include <cstdint>
#include <expected>
#include <span>
#include "CobsrDecoder.hpp"
#include "FrameDecoder.hpp"

/// @brief Clase que decodifica un frame recibido y previamente codificado con ProtocolCodec
class ProtocolDecoder {
public:

    /// @brief Decodifica un frame previamente codificado con ProtocolCodec
    /// @param inputBuffer Buffer donde se localiza el frame codificado
    /// @param frameBytes Buffer intermedio donde se guardará el frame después de decodificar COBS/R
    /// @param payloadInFrame Buffer donde se guarda el payload del frame
    /// @return std::expected con Frame en caso de éxito, en caso de error un ProtocolErrors
    /// @warning frameBytes y payloadInFrame DEBEN ser de distintos buffers
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> decode(std::span<const uint8_t> inputBuffer, std::span<uint8_t> frameBytes, 
        std::span<uint8_t> payloadInFrame) noexcept;
};

#endif // !PROTOCOL_DECODER_HEADER