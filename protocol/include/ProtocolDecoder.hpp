#ifndef PROTOCOL_DECODER_HEADER
#define PROTOCOL_DECODER_HEADER

#include <span>
#include <expected>
#include "ProtocolErrors.hpp"
#include "cobs/cobsr.h"
#include "SpanUtilities.hpp"

/// @brief Clase que decodifica un buffer previamente codificado con ProtocolCodec
class ProtocolDecoder {
public:

    /// @brief Decodifica un buffer codificao con ProtocolCodec y lo coloca en el buffer de salida
    /// @param encodedBuffer Buffer codificado
    /// @param outputBuffer Buffer en el que se colocará el resultado
    /// @return Span del buffer que contiene los elementos decodificados
    [[nodiscard]]
    static std::expected<std::span<uint8_t>, ProtocolErrors> decodeToRaw(std::span<const uint8_t> encodedBuffer, std::span<uint8_t> outputBuffer);
};

#endif // !PROTOCOL_DECODER_HEADER