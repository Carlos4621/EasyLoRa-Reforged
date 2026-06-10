#include "CobsrDecoder.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> CobsrDecoder::decode(std::span<const uint8_t> inputBuffer,
 std::span<uint8_t> outputBuffer) noexcept {
    if (inputBuffer.empty()) {
        return std::unexpected(ProtocolErrors::EmptyInputBuffer);
    }

    if (outputBuffer.size() < minimumOutputBufferSize(inputBuffer.size())) {
        return std::unexpected(ProtocolErrors::OutputBufferTooSmall);
    }

    if ((inputBuffer.data() != outputBuffer.data()) && spansOverlap(inputBuffer, outputBuffer)) {
        return std::unexpected(ProtocolErrors::SameBufferError);
    }
   
    const uint8_t* encodedPtr = inputBuffer.empty() ? outputBuffer.data() : inputBuffer.data();
    const auto cobsrStatus{ cobsr_decode(outputBuffer.data(), outputBuffer.size(), encodedPtr, inputBuffer.size()) };

    if (cobsrStatus.status != COBSR_DECODE_OK) {
        return std::unexpected(ProtocolErrors::COBSRDecodeError);
    }

    return outputBuffer.first(cobsrStatus.out_len);
}
