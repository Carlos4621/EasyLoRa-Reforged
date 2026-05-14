#ifndef PROTOCOL_CODEC_HEADER
#define PROTOCOL_CODEC_HEADER

#include <span>
#include <expected>
#include <algorithm>
#include "Frame.hpp"
#include "cobs/cobsr.h"
#include "BitsUtilities.hpp"
#include "ProtocolErrors.hpp"
#include "SpanUtilities.hpp"

/// @brief Clase encargada de codificar un raw frame previamente codificado con RawFrameCodec
class ProtocolCodec {
public:

    /// @brief Codifica un raw frame y lo coloca en el buffer dado
    /// @param rawFrame Raw frame previamente codificado
    /// @param outputBuffer Buffer en el que se colocará el resultado.
    /// @return std::span apuntando al buffer que contiene el resultado
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> encodeFromRaw(std::span<const uint8_t> rawFrame, std::span<uint8_t> outputBuffer);

private:

    [[nodiscard]]
    static bool outputBufferHaveEnoughSize(size_t outputBufferSize, size_t rawFrameSize);
};

#endif // !PROTOCOL_CODEC_HEADER