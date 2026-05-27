#ifndef COBSR_CODEC_HEADER
#define COBSR_CODEC_HEADER

#include <span>
#include <expected>
#include "Frame.hpp"
#include "cobs/cobsr.h"
#include "BitsUtilities.hpp"
#include "ProtocolErrors.hpp"
#include "SpanUtilities.hpp"

/// @brief Clase que codifica COBS/R a un buffer y coloca el resultado en otro buffer
class CobsrCodec {
public:

    /// @brief Codifica COBS/R al buffer dado y coloca el resultado en otro buffer
    /// @param inputBuffer Buffer con el que calculará COBS/R
    /// @param outputBuffer Buffer en el que se colocará el inputBuffer con COBS/R aplicado
    /// @return std::expected con std::span apuntando al buffer que contiene el resultado, en caso de error devuelve un ProtocolErrors
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> addCOBSR(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer);

private:

    [[nodiscard]]
    static bool outputBufferHaveEnoughSize(size_t outputBufferSize, size_t rawFrameSize);
};

#endif // !COBSR_CODEC_HEADER