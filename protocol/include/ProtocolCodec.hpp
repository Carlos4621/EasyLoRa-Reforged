#ifndef PROTOCOL_CODEC_HEADER
#define PROTOCOL_CODEC_HEADER

#include <cstdint>
#include <expected>
#include <span>
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
    /// @warning El agregado del delimitador 0x00 es responsabilidad del stream
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> encode(const Frame& frame, std::span<uint8_t> frameBytesBuffer,
        std::span<uint8_t> outputBuffer) noexcept;
    
    /// @brief Retorna el tamaño mínimo del buffer en el que se guardará los bytes del frame
    /// @param frame Frame a codificar
    /// @return Tamaño mínimo del frameBytesBuffer
    [[nodiscard]]
    static constexpr size_t minimumFrameBytesBufferSize(const Frame& frame) noexcept;

    /// @brief Retorna el tamaño mínimo del buffer en que se guardará el resultado
    /// @param frame Frame a codificar
    /// @return Tamañomínimo del outputBuffer
    [[nodiscard]]
    static constexpr size_t minimumOutputBufferSize(const Frame& frame) noexcept;
};

constexpr size_t ProtocolCodec::minimumFrameBytesBufferSize(const Frame &frame) noexcept {
    return FrameCodec::minimumOutputBufferSize(frame);
}

constexpr size_t ProtocolCodec::minimumOutputBufferSize(const Frame &frame) noexcept {
    return CobsrCodec::minimumOutputBufferSize(minimumFrameBytesBufferSize(frame));
}

#endif // !PROTOCOL_CODEC_HEADER