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

    /// @brief Codifica COBS/R al buffer dado y coloca el resultado en otro buffer. Decodificación in-place solo soportada con offset.
    /// @param inputBuffer Buffer con el que calculará COBS/R
    /// @param outputBuffer Buffer en el que se colocará el inputBuffer con COBS/R aplicado
    /// @return std::expected con std::span apuntando al buffer que contiene el resultado, en caso de error devuelve un ProtocolErrors
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> addCOBSR(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer) noexcept;

    /// @brief Retorna el tamaño mínimo que debe tener el buffer de salida
    /// @param bufferToEncodeSize Tamaño del buffer a codificar
    /// @return Tamaño mínimo del buffer de salida
    static size_t minumunOutputBufferSize(size_t bufferToEncodeSize) noexcept;

    /// @brief Retorna el offset necesario para realizar decodificación in-place
    /// @param bufferToEncodeSize Tamaño del buffer a codificar
    /// @return Offset necesario para decodificación in-place
    static size_t offsetRequiredForInPlace(size_t bufferToEncodeSize) noexcept;
};

#endif // !COBSR_CODEC_HEADER