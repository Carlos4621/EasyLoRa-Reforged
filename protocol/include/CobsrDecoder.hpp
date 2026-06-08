#ifndef COBSR_DECODER_HEADER
#define COBSR_DECODER_HEADER

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include "ProtocolErrors.hpp"
#include "SpanUtilities.hpp"
#include "cobs/cobsr.h"

/// @brief Clase que decodifica un buffer codificado con COBS/R y coloca el resultado en otro buffer
class CobsrDecoder {
public:

    /// @brief Decodifica un buffer codificado con COBS/R y coloca el resultado en otro buffer. Decodificación in-place posible solo sin offset.
    /// @param inputBuffer Buffer codificado en COBS/R
    /// @param outputBuffer Buffer en el que se colocará el inputBuffer descodificado
    /// @return std::expected con std::span que apunta al buffer con el resultado, en caso de error un ProtocolErrors
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> decode(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer) noexcept;

    /// @brief Retorna el tamaño mínimo que debe tener el buffer de salida
    /// @param bufferToEncodeSize Tamaño del buffer a decodificar
    /// @return Tamaño mínimo del buffer de salida
    static size_t minimunOutputBufferSize(size_t bufferToDecodeSize) noexcept;
};

#endif // !COBSR_DECODER_HEADER