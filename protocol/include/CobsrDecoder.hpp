#ifndef COBSR_DECODER_HEADER
#define COBSR_DECODER_HEADER

#include <span>
#include <expected>
#include "ProtocolErrors.hpp"
#include "cobs/cobsr.h"
#include "SpanUtilities.hpp"

/// @brief Clase que decodifica un buffer codificado con COBS/R y coloca el resultado en otro buffer
class CobsrDecoder {
public:

    /// @brief Decodifica un buffer codificado con COBS/R y coloca el resultado en otro buffer
    /// @param inputBuffer Buffer codificado en COBS/R
    /// @param outputBuffer Buffer en el que se colocará el inputBuffer descodificado
    /// @return std::expected con std::span que apunta al buffer con el resultado, en caso de error un ProtocolErrors
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> decode(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer);
};

#endif // !COBSR_DECODER_HEADER