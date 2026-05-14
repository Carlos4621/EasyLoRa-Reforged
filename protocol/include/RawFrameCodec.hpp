#ifndef RAW_FRAME_CODEC_HEADER
#define RAW_FRAME_CODEC_HEADER

#include <expected>
#include "Frame.hpp"
#include "BitsUtilities.hpp"
#include "CRC.h"
#include "ProtocolErrors.hpp"

/// @brief Clase encargada de codificar un Frame y agregar CRC. Para agregar COBS vease ProtocolBufferCodec
class RawFrameCodec {
public:

    /// @brief Codifica en bytes y añade CRC a los datos del frame  
    /// @param frame Frame a codificar
    /// @param outputBuffer Buffer en el que se escribirá el contenido codificado y CRC
    /// @return std::expected con un span el buffer que contiene los datos escritos
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> encodeFrameToRaw(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept;

private:

    [[nodiscard]]
    static constexpr bool isEnoughBufferSize(size_t bufferSize, size_t framePayloadSize) noexcept;

    static void insertCRC(std::span<uint8_t> buffer, size_t& bytesWritten) noexcept;

    static void insertPayload(std::span<uint8_t> buffer, const Frame& frame, size_t& bytesWritten) noexcept;

    [[nodiscard]]
    static bool insertMessageType(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten);

    [[nodiscard]]
    static bool insertPackageKind(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten);

    /// @brief Inserta dos bytes en Big Endian
    static void insertTwoBytes(std::span<uint8_t> buffer, uint16_t value, size_t& bytesWritten);
};

#endif // !RAW_FRAME_CODEC_HEADER