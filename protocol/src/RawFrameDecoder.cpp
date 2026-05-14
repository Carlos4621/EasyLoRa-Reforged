#include <cstring>

#include "RawFrameDecoder.hpp"

std::expected<Frame, ProtocolErrors> RawFrameDecoder::decodeFrameFromRaw(std::span<const uint8_t> inputRawBuffer, std::span<uint8_t> outputBufferInFrame) noexcept {
    if (inputRawBuffer.size() < Minimum_Raw_Buffer_Size) {
        return std::unexpected(ProtocolErrors::BufferTooSmall);
    }
    
    const auto bufferWithoutCRC{ inputRawBuffer.first(inputRawBuffer.size() - Frame::CRC_Size) };

    if (!isCRCValid(inputRawBuffer, bufferWithoutCRC)) {
        return std::unexpected(ProtocolErrors::CRCMissMatch);
    } else if (!isEnoughPayloadSize(inputRawBuffer.size(), outputBufferInFrame.size())) {
        return std::unexpected(ProtocolErrors::FramePayloadTooSmall);
    }

    size_t currentByte{ 0 };
    Frame outputFrame;

    outputFrame.version = bufferWithoutCRC[currentByte++];
    
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
    auto outputPayload{ outputBufferInFrame.first(payloadSize) };

    if (payloadSize > 0) {
        std::memmove(outputPayload.data(), bufferWithoutCRC.data() + currentByte, payloadSize);
    }

    outputFrame.payload = outputPayload;

    return outputFrame;
}

bool RawFrameDecoder::isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC) {
    const auto expectedCRC{ CRC::Calculate(&bufferWithoutCRC[0], bufferWithoutCRC.size(), CRC::CRC_16_CCITTFALSE()) };

    const auto receivedCRCSpan{ buffer.last(Frame::CRC_Size) };
    const auto receivedCRC{ bindTwoBytes(receivedCRCSpan[0], receivedCRCSpan[1]) };

    return (expectedCRC == receivedCRC);
}

bool RawFrameDecoder::isEnoughPayloadSize(size_t inputBufferSize, size_t framePayloadSize) {
    return framePayloadSize >= (inputBufferSize - Frame::Header_Size - Frame::CRC_Size);
}

bool RawFrameDecoder::inputBufferHaveEnoughSize(size_t inputBufferSize) {
    return inputBufferSize >= (Frame::Header_Size + Frame::CRC_Size);
}

bool RawFrameDecoder::putPackageKind(std::span<const uint8_t> buffer, PackageKind& kind, size_t &currentByte) {
    if (!isValidPackageKind(buffer[currentByte])) {
        return false;
    }
    kind = static_cast<PackageKind>(buffer[currentByte++]);

    return true;
}

bool RawFrameDecoder::putMessageType(std::span<const uint8_t> buffer, MessageType &type, size_t &currentByte) {
    if (!isValidMessageType(buffer[currentByte])) {
        return false;
    }
    type = static_cast<MessageType>(buffer[currentByte++]);

    return true;
}

void RawFrameDecoder::putTwoBytes(std::span<const uint8_t> buffer, uint16_t& value, size_t &currentByte) {
    const auto highByte{ buffer[currentByte++] };
    const auto lowByte{ buffer[currentByte++] };

    value = bindTwoBytes(highByte, lowByte);
}
