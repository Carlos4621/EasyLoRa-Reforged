#include "CobsrCodec.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> CobsrCodec::addCOBSR(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer) noexcept {
    if (inputBuffer.empty()) {
        return std::unexpected{ProtocolErrors::EmptyInputBuffer};
    }
    
    if (outputBuffer.size() < minimumOutputBufferSize(inputBuffer.size())) {
        return std::unexpected{ProtocolErrors::OutputBufferTooSmall};
    }

    if (spansOverlap(inputBuffer, outputBuffer)) {
        const auto requiredOffset{ static_cast<size_t>(offsetRequiredForInPlace(inputBuffer.size())) };
        const auto dstBegin{ reinterpret_cast<std::uintptr_t>(outputBuffer.data()) };
        const auto srcBegin{ reinterpret_cast<std::uintptr_t>(inputBuffer.data()) };

        if (srcBegin < dstBegin + requiredOffset) {
            return std::unexpected{ProtocolErrors::SameBufferError};
        }
    }

    const auto cobsrStatus{ cobsr_encode(outputBuffer.data(), outputBuffer.size(), inputBuffer.data(), inputBuffer.size()) };

    if (cobsrStatus.status != COBSR_ENCODE_OK) {
        return std::unexpected{ProtocolErrors::COBSREncodeError};
    }

    outputBuffer[cobsrStatus.out_len] = Frame::Frame_Delimiter;
    
    return outputBuffer.first(cobsrStatus.out_len + sizeof(Frame::Frame_Delimiter));
}
