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
   
    if (inputBuffer.back() != Frame::Frame_Delimiter) {
        return std::unexpected(ProtocolErrors::COBSRInputBufferWithoutDelimiter);
    }

    const auto encodedSize{ inputBuffer.size() - sizeof(Frame::Frame_Delimiter) };
    const auto cobsrStatus{ cobsr_decode(outputBuffer.data(), outputBuffer.size(), inputBuffer.data(), encodedSize) };

    if (cobsrStatus.status != COBSR_DECODE_OK) {
        return std::unexpected(ProtocolErrors::COBSRDecodeError);
    }

    return outputBuffer.first(cobsrStatus.out_len);
}
