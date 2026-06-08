#ifndef FRAME_DECODER_HEADER
#define FRAME_DECODER_HEADER

#include <cstddef>
#include <cstdint>
#include <expected>
#include "CRC.h"
#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include "SpanUtilities.hpp"

/// @brief Clase que comprueba CRC y decodifica un Frame previamente codificado a bytes
class FrameDecoder {
public:

    /// @brief Tamaño mínimo que debería tener el buffer en donde se encuentra el Frame en bytes
    static constexpr size_t Minimum_Raw_Buffer_Size{ Frame::Header_Size + Frame::CRC_Size };

    /// @brief Decodifica el buffer con el Frame en bytes y coloca el resultado en un Frame
    /// @param inputRawBuffer Buffer en donde se encuentra el Frame en bytes
    /// @param payloadInFrame Buffer donde se localizará el payload del frame
    /// @param frame Frame en el que se colocará la data decodificada
    /// @return std::expected con el frame en caso de éxito, ProtocolErrors en caso de error
    /// @warning Se debe entregar el buffer SIN el 0x00
    /// @warning El buffer del payload no puede ser usado como salida
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> decode(std::span<const uint8_t> inputRawBuffer, std::span<uint8_t> payloadInFrame) noexcept;

private:

    [[nodiscard]]
    static bool isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC) noexcept;

    [[nodiscard]]
    static bool isEnoughPayloadSize(size_t inputBufferSize, size_t framePayloadSize) noexcept;

    [[nodiscard]]
    static bool inputBufferHaveEnoughSize(size_t inputBufferSize) noexcept;

    [[nodiscard]]
    static bool putPackageKind(std::span<const uint8_t> buffer, PackageKind& kind, size_t& currentByte) noexcept;

    [[nodiscard]]
    static bool putMessageType(std::span<const uint8_t> buffer, MessageType& type, size_t& currentByte) noexcept;

    /// @brief Transfiere dos bytes a un uint16_t en formato Big Endian
    static void putTwoBytes(std::span<const uint8_t> buffer, uint16_t& value, size_t& currentByte) noexcept;
};

#endif // !FRAME_DECODER_HEADER