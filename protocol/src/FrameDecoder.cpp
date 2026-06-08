#include "FrameDecoder.hpp"

#include <cstring>
#include "BitsUtilities.hpp"

std::expected<Frame, ProtocolErrors> FrameDecoder::decode(std::span<const uint8_t> inputRawBuffer, std::span<uint8_t> outputPayloadInFrame) noexcept {
    if (inputRawBuffer.size() < Minimum_Raw_Buffer_Size) {
        return std::unexpected(ProtocolErrors::BufferTooSmall);
    }

    if (spansOverlap(inputRawBuffer, outputPayloadInFrame)) {
        return std::unexpected(ProtocolErrors::SameBufferError);
    }
    
    const auto bufferWithoutCRC{ inputRawBuffer.first(inputRawBuffer.size() - Frame::CRC_Size) };

    if (!isCRCValid(inputRawBuffer, bufferWithoutCRC)) {
        return std::unexpected(ProtocolErrors::CRCMissMatch);
    } else if (!isEnoughPayloadSize(inputRawBuffer.size(), outputPayloadInFrame.size())) {
        return std::unexpected(ProtocolErrors::FramePayloadTooSmall);
    }

    size_t currentByte{ 0 };
    Frame outputFrame;

    outputFrame.version = bufferWithoutCRC[currentByte++];

    if (outputFrame.version != Frame::Actual_Frame_Version) {
        return std::unexpected(ProtocolErrors::FrameVersionMissmatch);
    }
    
    if (!putPackageKind(bufferWithoutCRC, outputFrame.kind, currentByte)) {
        return std::unexpected(ProtocolErrors::InvalidPackageKind);
    }

    outputFrame.flags = bufferWithoutCRC[currentByte++];
    outputFrame.reserved = bufferWithoutCRC[currentByte++];

    putTwoBytes(bufferWithoutCRC, outputFrame.seq, currentByte);

    if (!putMessageType(bufferWithoutCRC, outputFrame.type, currentByte)) {
        return std::unexpected(ProtocolErrors::InvalidMessageType);
    }

    const size_t payloadSize{ bufferWithoutCRC.size() - currentByte };
    auto outputPayload{ outputPayloadInFrame.first(payloadSize) };

    if (payloadSize > 0) {
        std::memmove(outputPayload.data(), bufferWithoutCRC.data() + currentByte, payloadSize);
    }

    outputFrame.payload = outputPayload;

    return outputFrame;
}

bool FrameDecoder::isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC) noexcept {
    const auto expectedCRC{ CRC::Calculate(&bufferWithoutCRC[0], bufferWithoutCRC.size(), CRC::CRC_16_CCITTFALSE()) };

    const auto receivedCRCSpan{ buffer.last(Frame::CRC_Size) };
    const auto receivedCRC{ bindTwoBytes(receivedCRCSpan[0], receivedCRCSpan[1]) };

    return (expectedCRC == receivedCRC);
}

bool FrameDecoder::isEnoughPayloadSize(size_t inputBufferSize, size_t framePayloadSize) noexcept {
    return framePayloadSize >= (inputBufferSize - Frame::Header_Size - Frame::CRC_Size);
}

bool FrameDecoder::inputBufferHaveEnoughSize(size_t inputBufferSize) noexcept {
    return inputBufferSize >= (Frame::Header_Size + Frame::CRC_Size);
}

bool FrameDecoder::putPackageKind(std::span<const uint8_t> buffer, PackageKind& kind, size_t &currentByte) noexcept {
    if (!isValidPackageKind(buffer[currentByte])) {
        return false;
    }
    kind = static_cast<PackageKind>(buffer[currentByte++]);

    return true;
}

bool FrameDecoder::putMessageType(std::span<const uint8_t> buffer, MessageType &type, size_t &currentByte) noexcept {
    if (!isValidMessageType(buffer[currentByte])) {
        return false;
    }
    type = static_cast<MessageType>(buffer[currentByte++]);

    return true;
}

void FrameDecoder::putTwoBytes(std::span<const uint8_t> buffer, uint16_t& value, size_t &currentByte) noexcept {
    const auto highByte{ buffer[currentByte++] };
    const auto lowByte{ buffer[currentByte++] };

    value = bindTwoBytes(highByte, lowByte);
}
