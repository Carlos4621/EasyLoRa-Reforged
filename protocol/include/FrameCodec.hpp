#ifndef FRAME_CODEC_HEADER
#define FRAME_CODEC_HEADER

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include "CRC.h"
#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include "SpanUtilities.hpp"

/// @brief Clase encargada de codificar un Frame a bytes y agregar CRC.
class FrameCodec {
public:

    /// @brief Tamaño máximo permitido para el payload ubicado en el frame
    static constexpr uint16_t Max_Frame_Payload_Size{ 512 };

    /// @brief Convierte a bytes un Frame y coloca su correspondiente CRC.
    /// @param frame Frame a codificar
    /// @param outputBuffer Buffer en el que se escribirá el contenido codificado y CRC
    /// @return std::expected con std::span el buffer que contiene los datos escritos, en caso de error devuelve un ProtocolError
    /// @warning Esta función no coloca el 0x00 delimitador, eso debe hacerse por parte del usuario.
    /// @warning El buffer del payload del frame no puede ser usado como buffer de salida
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> encode(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept;

    /// @brief Devuelve el tamaño mínimo que debe tener el buffer de salida
    /// @param frame Frame a coificar
    /// @return Tamaño mínimo del buffer de salida
    [[nodiscard]]
    static constexpr size_t minimumOutputBufferSize(const Frame& frame) noexcept;

private:

    static void insertCRC(std::span<uint8_t> buffer, size_t& bytesWritten) noexcept;

    static void insertPayload(std::span<uint8_t> buffer, const Frame& frame, size_t& bytesWritten) noexcept;

    [[nodiscard]]
    static bool insertMessageType(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) noexcept;

    [[nodiscard]]
    static bool insertPackageKind(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) noexcept;

    /// @brief Inserta dos bytes en Big Endian
    static void insertTwoBytes(std::span<uint8_t> buffer, uint16_t value, size_t& bytesWritten) noexcept;
};

constexpr size_t FrameCodec::minimumOutputBufferSize(const Frame &frame) noexcept{
    return frame.payload.size() + Frame::CRC_Size + Frame::Header_Size;
}

#endif // !FRAME_CODEC_HEADER