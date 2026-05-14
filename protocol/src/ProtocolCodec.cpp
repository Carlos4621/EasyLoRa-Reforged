#include "ProtocolCodec.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> ProtocolCodec::encodeFromRaw(std::span<const uint8_t> rawFrame, std::span<uint8_t> outputBuffer) {
    if (!outputBufferHaveEnoughSize(outputBuffer.size(), rawFrame.size())) {
        return std::unexpected(ProtocolErrors::BufferTooSmall);
    }

    if (spansOverlap(rawFrame, outputBuffer)) {
        const auto requiredOffset{ static_cast<size_t>(COBSR_ENCODE_SRC_OFFSET(rawFrame.size())) };
        const auto dstBegin{ reinterpret_cast<std::uintptr_t>(outputBuffer.data()) };
        const auto srcBegin{ reinterpret_cast<std::uintptr_t>(rawFrame.data()) };

        if (srcBegin < dstBegin + requiredOffset) {
            return std::unexpected(ProtocolErrors::SameBufferError);
        }
    }

    const uint8_t* rawPtr{ rawFrame.empty() ? outputBuffer.data() : rawFrame.data() };
    const auto cobsrStatus{ cobsr_encode(outputBuffer.data(), outputBuffer.size(), rawPtr, rawFrame.size()) };

    if (cobsrStatus.status != COBSR_ENCODE_OK) {
        return std::unexpected(ProtocolErrors::COBSRError);
    }
    
    return outputBuffer.first(cobsrStatus.out_len);
}

bool ProtocolCodec::outputBufferHaveEnoughSize(size_t outputBufferSize, size_t rawFrameSize) {
    return outputBufferSize >= COBSR_ENCODE_DST_BUF_LEN_MAX(rawFrameSize);
}
