#ifndef RAW_FRAME_DECODER
#define RAW_FRAME_DECODER

#include <optional>
#include "CRC.h"
#include "Frame.hpp"
#include "BitsUtilities.hpp"

/// @brief Clase encargada de decodificar un buffer codificado con RawFrameCodec
class RawFrameDecoder {
public:

    /// @brief Decodifica el buffer y coloca el resultao en un Frame
    /// @param inputRawBuffer Buffer en donde se encuentra la data codificada
    /// @param frame Frame en el que se colocará la data decodificada
    /// @return true en caso de éxito, false sino
    [[nodiscard]]
    static bool decodeFrameFromRaw(std::span<const uint8_t> inputRawBuffer, Frame& frame) noexcept;

private:

    [[nodiscard]]
    static bool isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC);
};

#endif // !RAW_FRAME_DECODER