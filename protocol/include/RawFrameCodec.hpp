#ifndef RAW_FRAME_CODEC_HEADER
#define RAW_FRAME_CODEC_HEADER

#include <optional>
#include "Frame.hpp"
#include "BitsUtilities.hpp"
#include "CRC.h"

/// @brief Clase encargada de codificar un Frame y agregar CRC. Para agregar COBS vease ProtocolBufferCodec
class RawFrameCodec {
public:

    /// @brief Codifica en bytes y añade CRC a los datos del frame  
    /// @param frame Frame a codificar
    /// @param outputBuffer Buffer en el que se escribirá el contenido codificado y CRC
    /// @return std::optional<size_t> con los bytes escritos, en caso de error un std::nullopt
    [[nodiscard]]
    static std::optional<size_t> encodeFrameToRaw(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept;

private:

    [[nodiscard]]
    static constexpr bool isEnoughBufferSize(size_t bufferSize, size_t framePayloadSize) noexcept;
};

#endif // !RAW_FRAME_CODEC_HEADER