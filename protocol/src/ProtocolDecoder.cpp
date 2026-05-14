#include "ProtocolDecoder.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> ProtocolDecoder::decodeToRaw(std::span<const uint8_t> encodedBuffer, std::span<uint8_t> outputBuffer) {
    if (encodedBuffer.empty()) {
        return std::unexpected(ProtocolErrors::EmptyInputBuffer);
    }

    if (outputBuffer.size() < COBSR_DECODE_DST_BUF_LEN_MAX(encodedBuffer.size())) {
        return std::unexpected(ProtocolErrors::BufferTooSmall);
    }

    if (spansOverlap(encodedBuffer, outputBuffer) && (encodedBuffer.data() != outputBuffer.data())) {
        return std::unexpected(ProtocolErrors::SameBufferError);
    }
   
    const uint8_t* encodedPtr = encodedBuffer.empty() ? outputBuffer.data() : encodedBuffer.data();
    const auto cobsrStatus{ cobsr_decode(outputBuffer.data(), outputBuffer.size(), encodedPtr, encodedBuffer.size()) };

    if (cobsrStatus.status != COBSR_DECODE_OK) {
        return std::unexpected(ProtocolErrors::COBSRError);
    }

    return outputBuffer.first(cobsrStatus.out_len);
}