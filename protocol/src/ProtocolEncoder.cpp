#include "ProtocolEncoder/ProtocolEncoder.hpp"

namespace protocol {

    std::expected<std::span<std::byte>, ProtocolErrors> ProtocolEncoder::encode(std::span<const std::byte> inputBuffer, std::span<std::byte> outputBuffer) noexcept {
        
    }
}