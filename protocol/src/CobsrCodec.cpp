#include "CobsrCodec.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> CobsrCodec::addCOBSR(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer) noexcept {
    if (inputBuffer.empty()) {
        return outputBuffer.first(0);
    }
    
    if (outputBuffer.size() < minumunOutputBufferSize(inputBuffer.size())) {
        return std::unexpected(ProtocolErrors::OutputBufferTooSmall);
    }

    if (spansOverlap(inputBuffer, outputBuffer)) {
        const auto requiredOffset{ static_cast<size_t>(offsetRequiredForInPlace(inputBuffer.size())) };
        const auto dstBegin{ reinterpret_cast<std::uintptr_t>(outputBuffer.data()) };
        const auto srcBegin{ reinterpret_cast<std::uintptr_t>(inputBuffer.data()) };

        if (srcBegin < dstBegin + requiredOffset) {
            return std::unexpected(ProtocolErrors::SameBufferError);
        }
    }

    const uint8_t* rawPtr{ inputBuffer.empty() ? outputBuffer.data() : inputBuffer.data() };
    const auto cobsrStatus{ cobsr_encode(outputBuffer.data(), outputBuffer.size(), rawPtr, inputBuffer.size()) };

    if (cobsrStatus.status != COBSR_ENCODE_OK) {
        return std::unexpected(ProtocolErrors::COBSREncodeError);
    }
    
    return outputBuffer.first(cobsrStatus.out_len);
}

constexpr size_t CobsrCodec::minumunOutputBufferSize(size_t bufferToEncodeSize) noexcept {
    return COBSR_ENCODE_DST_BUF_LEN_MAX(bufferToEncodeSize);
}

constexpr size_t CobsrCodec::offsetRequiredForInPlace(size_t bufferToEncodeSize) noexcept{
    return COBSR_ENCODE_SRC_OFFSET(bufferToEncodeSize);
}
