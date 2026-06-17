#ifndef PROTOCOL_DECODER_HEADER
#define PROTOCOL_DECODER_HEADER

#include <cstdint>
#include <expected>
#include <span>
#include "CobsrDecoder.hpp"
#include "FrameDecoder.hpp"

/// @brief Organiza los buffers usados para la decodificación de protocolo.
struct ProtocolDecoderBuffers {

    /// @brief Buffer con el frame codificado en COBS/R, con delimitador final 0x00.
    std::span<const uint8_t> inputBuffer;

    /// @brief Buffer intermedio donde se guardará el frame raw después de decodificar COBS/R.
    std::span<uint8_t> frameBytes;

    /// @brief Buffer donde se guardará el payload del frame decodificado.
    std::span<uint8_t> payloadInFrame;
};

/// @brief Clase que decodifica un frame recibido y previamente codificado con ProtocolCodec
class ProtocolDecoder {
public:

    /// @brief Decodifica un frame previamente codificado con ProtocolCodec
    /// @param buffers Buffers donde se trabajará la decodificación
    /// @return std::expected con Frame en caso de éxito, en caso de error un ProtocolErrors
    /// @warning ProtocolDecoderBuffers::frameBytes y ProtocolDecoderBuffers::payloadInFrame DEBEN ser de distintos buffers
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> decode(const ProtocolDecoderBuffers& buffers) noexcept;

    /// @brief Devuelve el tamaño mínimo que debe tener el buffer que almacenará el frame en forma de bytes
    /// @param inputBufferSize Tamaño del buffer a decodificar
    /// @return Tamaño mínimo de frameBytes
    [[nodiscard]]
    static constexpr size_t minimumFrameBytesBufferSize(size_t inputBufferSize) noexcept;

    /// @brief Devuelve el tamaño mínimo que debe tener el buffer que almaneca el payload del frame
    /// @param inputBufferSize Tamaño del buffer a decodificar
    /// @return Tamaño mínimo de payloadInFrame
    [[nodiscard]]
    static constexpr std::expected<size_t, ProtocolErrors> minimumPayloadBufferSize(size_t inputBufferSize) noexcept;
};

constexpr size_t ProtocolDecoder::minimumFrameBytesBufferSize(size_t inputBufferSize) noexcept {
    return CobsrDecoder::minimumOutputBufferSize(inputBufferSize);
}

constexpr std::expected<size_t, ProtocolErrors> ProtocolDecoder::minimumPayloadBufferSize(size_t inputBufferSize) noexcept {
    return FrameDecoder::minimumPayloadBufferSize(minimumFrameBytesBufferSize(inputBufferSize));
}

#endif // !PROTOCOL_DECODER_HEADER
