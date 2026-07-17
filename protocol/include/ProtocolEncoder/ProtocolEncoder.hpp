#ifndef PROTOCOL_ENCODER_HEADER
#define PROTOCOL_ENCODER_HEADER

#include <expected>
#include "ProtocolErrors/ProtocolErrors.hpp"
#include <span>

namespace protocol {
    class ProtocolEncoder {
    public:
        
        /// @brief Codifica un buffer para 
        /// @param inputBuffer Buffer donde se localiza la data a codificar
        /// @param outputBuffer Buffer donde se guardará la data codificada y lista para ser enviada
        /// @return std::expected<std::span<std::byte>, ProtocolErrors> con std::span con vista a la data codificada, en caso de error ProtocolErrors
        ///         con el tipo de error
        static std::expected<std::span<std::byte>, ProtocolErrors> encode(std::span<const std::byte> inputBuffer, std::span<std::byte> outputBuffer) noexcept;
    };
} // namespace protocol

#endif // !PROTOCOL_ENCODER_HEADER