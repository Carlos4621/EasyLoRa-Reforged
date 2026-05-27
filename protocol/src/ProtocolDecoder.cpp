#include "ProtocolDecoder.hpp"

std::expected<Frame, ProtocolErrors> ProtocolDecoder::decode(std::span<const uint8_t> inputBuffer, std::span<uint8_t> frameBytes, std::span<uint8_t> payloadInFrame) noexcept {
    const auto status{ CobsrDecoder::decode(inputBuffer, frameBytes) };

    if (!status.has_value()) {
        return std::unexpected{ status.error() };
    }

    return FrameDecoder::decode(status.value(), payloadInFrame);
}