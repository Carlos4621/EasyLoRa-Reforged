#include "CobsrDecoder.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> CobsrDecoder::decode(std::span<const uint8_t> inputBuffer, std::span<uint8_t> outputBuffer) {
    if (inputBuffer.empty()) {
        return std::unexpected(ProtocolErrors::EmptyInputBuffer);
    }

    if (outputBuffer.size() < minimunOutputBufferSize(inputBuffer.size())) {
        return std::unexpected(ProtocolErrors::BufferTooSmall);
    }

    if (spansOverlap(inputBuffer, outputBuffer) && (inputBuffer.data() != outputBuffer.data())) {
        return std::unexpected(ProtocolErrors::SameBufferError);
    }
   
    const uint8_t* encodedPtr = inputBuffer.empty() ? outputBuffer.data() : inputBuffer.data();
    const auto cobsrStatus{ cobsr_decode(outputBuffer.data(), outputBuffer.size(), encodedPtr, inputBuffer.size()) };

    if (cobsrStatus.status != COBSR_DECODE_OK) {
        return std::unexpected(ProtocolErrors::COBSRError);
    }

    return outputBuffer.first(cobsrStatus.out_len);
}

size_t CobsrDecoder::minimunOutputBufferSize(size_t bufferToDecodeSize) noexcept {
    return COBSR_DECODE_DST_BUF_LEN_MAX(bufferToDecodeSize);
}
