#ifndef FRAME_DECODER_HEADER
#define FRAME_DECODER_HEADER

#include <cstddef>
#include <cstdint>
#include <expected>
#include "CRC.h"
#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include "SpanUtilities.hpp"
#include "FrameCodec.hpp"

/// @brief Clase que comprueba CRC y decodifica un Frame previamente codificado a bytes
class FrameDecoder {
public:

    /// @brief Tamaño mínimo que debería tener el buffer en donde se encuentra el Frame en bytes
    static constexpr size_t Min_Raw_Buffer_Size{ Frame::Header_Size + Frame::CRC_Size };

    /// @brief Tamaño máximo que puede tener el buffer de entrada
    static constexpr size_t Max_Input_Buffer_Size{ Min_Raw_Buffer_Size + FrameCodec::Max_Frame_Payload_Size };

    /// @brief Decodifica el buffer con el Frame en bytes y coloca el resultado en un Frame
    /// @param inputRawBuffer Buffer en donde se encuentra el Frame en bytes
    /// @param payloadInFrame Buffer donde se localizará el payload del frame
    /// @param frame Frame en el que se colocará la data decodificada
    /// @return std::expected con el frame en caso de éxito, ProtocolErrors en caso de error
    /// @warning Se debe entregar el buffer SIN el 0x00
    /// @warning El buffer del payload no puede ser usado como salida
    /// @warning Debido a que Frame contiene un span, el buffer de payloadInFrame debe vivir cuando se usa el frame
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> decode(std::span<const uint8_t> inputRawBuffer, std::span<uint8_t> payloadInFrame) noexcept;

    /// @brief Devuelve el tamaño mínimo que debe tener el buffer que guarda el payload del frame
    /// @param inputRawBufferSize Tamaño del buffer a decodificar
    /// @return Tamaño mínimo de payloadInFrame
    [[nodiscard]]
    static constexpr std::expected<size_t, ProtocolErrors> minimumPayloadBufferSize(size_t inputRawBufferSize) noexcept;

private:

    [[nodiscard]]
    static bool isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC) noexcept;

    [[nodiscard]]
    static bool isEnoughPayloadSize(size_t inputBufferSize, size_t framePayloadSize) noexcept;

    [[nodiscard]]
    static bool putPackageKind(std::span<const uint8_t> buffer, PackageKind& kind, size_t& currentByte) noexcept;

    [[nodiscard]]
    static bool putMessageType(std::span<const uint8_t> buffer, MessageType& type, size_t& currentByte) noexcept;

    /// @brief Transfiere dos bytes a un uint16_t en formato Big Endian
    static void putTwoBytes(std::span<const uint8_t> buffer, uint16_t& value, size_t& currentByte) noexcept;
};

constexpr std::expected<size_t, ProtocolErrors> FrameDecoder::minimumPayloadBufferSize(size_t inputRawBufferSize) noexcept {
    if (inputRawBufferSize < Min_Raw_Buffer_Size) {
        return std::unexpected{ ProtocolErrors::InputBufferTooSmall };
    }
    
    return inputRawBufferSize - Min_Raw_Buffer_Size;
}

#endif // !FRAME_DECODER_HEADER