#include "ProtocolCodec.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> ProtocolCodec::encode(const Frame &frame, std::span<uint8_t> frameBytesBuffer,
    std::span<uint8_t> outputBuffer) noexcept {
        
    const auto status{ FrameCodec::encode(frame, frameBytesBuffer) };

    if (!status.has_value()) {
        return std::unexpected{ status.error() };
    }

    return CobsrCodec::addCOBSR(status.value(), outputBuffer);
}
