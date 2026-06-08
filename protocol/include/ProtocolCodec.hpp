#ifndef PROTOCOL_CODEC_HEADER
#define PROTOCOL_CODEC_HEADER

#include "CobsrCodec.hpp"
#include "FrameCodec.hpp"

/// @brief Clase que prepara el frame para ser enviado mediante serial
class ProtocolCodec {
public:
    
    /// @brief Prepara el frame para ser enviado mediante serial. Convierte el frame a bytes, aplica COBS/R y CRC
    /// @param frame Frame a codificar
    /// @param frameBytesBuffer Buffer en donde se colocará el frame en bytes
    /// @param outputBuffer Buffer en el que se colocará el resultado
    /// @return std::expected con std::span que apunta al buffer con el resultado, ProtocolErrors en caso de error
    /// @warning frameByteBuffer y outputBuffer DEBEN ser de distintos buffers
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> encode(const Frame& frame, std::span<uint8_t> frameBytesBuffer,
        std::span<uint8_t> outputBuffer) noexcept;
};

#endif // !PROTOCOL_CODEC_HEADER