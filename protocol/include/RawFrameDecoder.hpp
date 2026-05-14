#ifndef RAW_FRAME_DECODER
#define RAW_FRAME_DECODER

#include <expected>
#include "CRC.h"
#include "Frame.hpp"
#include "BitsUtilities.hpp"
#include "ProtocolErrors.hpp"

/// @brief Clase encargada de decodificar un buffer codificado con RawFrameCodec
class RawFrameDecoder {
public:

    static constexpr size_t Minimum_Raw_Buffer_Size{ Frame::Header_Size + Frame::CRC_Size };

    /// @brief Decodifica el buffer y coloca el resultado en un Frame
    /// @param inputRawBuffer Buffer en donde se encuentra la data codificada
    /// @param frame Frame en el que se colocará la data decodificada
    /// @return std::expected con el frame en caso de éxito, ProtocolErrors en caso de error
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> decodeFrameFromRaw(std::span<const uint8_t> inputRawBuffer, 
        std::span<uint8_t> outputBufferInFrame) noexcept;

private:

    [[nodiscard]]
    static bool isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC);

    [[nodiscard]]
    static bool isEnoughPayloadSize(size_t inputBufferSize, size_t framePayloadSize);

    [[nodiscard]]
    static bool inputBufferHaveEnoughSize(size_t inputBufferSize);

    [[nodiscard]]
    static bool putPackageKind(std::span<const uint8_t> buffer, PackageKind& kind, size_t& currentByte);

    [[nodiscard]]
    static bool putMessageType(std::span<const uint8_t> buffer, MessageType& type, size_t& currentByte);

    /// @brief Transfiere dos bytes a un uint16_t en formato Big Endian
    static void putTwoBytes(std::span<const uint8_t> buffer, uint16_t& value, size_t& currentByte);
};

#endif // !RAW_FRAME_DECODER