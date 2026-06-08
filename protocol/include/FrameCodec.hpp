#ifndef FRAME_CODEC_HEADER
#define FRAME_CODEC_HEADER

#include <expected>
#include "Frame.hpp"
#include "BitsUtilities.hpp"
#include "CRC.h"
#include "ProtocolErrors.hpp"
#include <cstring>
#include "SpanUtilities.hpp"

/// @brief Clase encargada de codificar un Frame a bytes y agregar CRC.
class FrameCodec {
public:

    /// @brief Convierte a bytes un Frame y coloca su correspondiente CRC 
    /// @param frame Frame a codificar
    /// @param outputBuffer Buffer en el que se escribirá el contenido codificado y CRC
    /// @return std::expected con std::span el buffer que contiene los datos escritos, en caso de error devuelve un ProtocolError
    /// @warning Esta función no coloca el 0x00 delimitador, eso debe hacerse por parte del usuario
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> encode(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept;

private:

    [[nodiscard]]
    static constexpr bool isEnoughBufferSize(size_t bufferSize, size_t framePayloadSize) noexcept;

    static void insertCRC(std::span<uint8_t> buffer, size_t& bytesWritten) noexcept;

    static void insertPayload(std::span<uint8_t> buffer, const Frame& frame, size_t& bytesWritten) noexcept;

    [[nodiscard]]
    static bool insertMessageType(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) noexcept;

    [[nodiscard]]
    static bool insertPackageKind(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) noexcept;

    /// @brief Inserta dos bytes en Big Endian
    static void insertTwoBytes(std::span<uint8_t> buffer, uint16_t value, size_t& bytesWritten) noexcept;
};

#endif // !FRAME_CODEC_HEADER