#include "ProtocolDecoder.hpp"

std::expected<Frame, ProtocolErrors> ProtocolDecoder::decode(const ProtocolDecoderBuffers& buffers) noexcept {
    const auto status{ CobsrDecoder::decode(buffers.inputBuffer, buffers.frameBytes) };

    if (!status.has_value()) {
        return std::unexpected{ status.error() };
    }

    return FrameDecoder::decode(status.value(), buffers.payloadInFrame);
}
