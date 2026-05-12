#include "RawFrameDecoder.hpp"

bool RawFrameDecoder::decodeFrameFromRaw(std::span<const uint8_t> inputRawBuffer, Frame &frame) noexcept {
    if (!inputBufferHaveEnoughSize(inputRawBuffer.size())) {
        return false;
    }
    
    const auto bufferWithoutCRC{ inputRawBuffer.first(inputRawBuffer.size() - Frame::CRC_Size) };

    if (!isCRCValid(inputRawBuffer, bufferWithoutCRC) || !isEnoughPayloadSize(inputRawBuffer.size(), frame.payload.size())) {
        return false;
    }

    size_t currentByte{ 0 };
    frame.version = bufferWithoutCRC[currentByte++];
    frame.kind = static_cast<PackageKind>(bufferWithoutCRC[currentByte++]);
    frame.flags = bufferWithoutCRC[currentByte++];
    frame.reserved = bufferWithoutCRC[currentByte++];

    const auto seqHighByte{ bufferWithoutCRC[currentByte++] };
    const auto seqLowByte{ bufferWithoutCRC[currentByte++] };

    frame.seq = bindTwoBytes(seqHighByte, seqLowByte);

    frame.type = static_cast<MessageType>(bufferWithoutCRC[currentByte++]);

    std::copy(bufferWithoutCRC.begin() + currentByte, bufferWithoutCRC.end(), frame.payload.begin());

    return true;
}

bool RawFrameDecoder::isCRCValid(std::span<const uint8_t> buffer, std::span<const uint8_t> bufferWithoutCRC) {
    const auto expectedCRC{ CRC::Calculate(&bufferWithoutCRC[0], bufferWithoutCRC.size(), CRC::CRC_16_CCITTFALSE()) };

    const auto receivedCRCSpan{ buffer.last(Frame::CRC_Size) };
    const auto receivedCRC{ bindTwoBytes(receivedCRCSpan[0], receivedCRCSpan[1]) };

    return (expectedCRC == receivedCRC);
}

bool RawFrameDecoder::isEnoughPayloadSize(size_t inputBufferSize, size_t framePayloadSize) {
    return framePayloadSize >= (inputBufferSize + Frame::Header_Size);
}

bool RawFrameDecoder::inputBufferHaveEnoughSize(size_t inputBufferSize) {
    return inputBufferSize >= (Frame::Header_Size + Frame::CRC_Size);
}
