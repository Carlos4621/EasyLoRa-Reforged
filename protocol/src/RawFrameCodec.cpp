#include "RawFrameCodec.hpp"

std::optional<size_t> RawFrameCodec::encodeFrameToRaw(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept {
    if (!isEnoughBufferSize(outputBuffer.size(), frame.payload.size())) {
        return std::nullopt;
    }
    
    size_t bytesWritten{ 0 };
    outputBuffer[bytesWritten++] = frame.version;
    outputBuffer[bytesWritten++] = std::to_underlying(frame.kind);
    outputBuffer[bytesWritten++] = frame.flags;
    outputBuffer[bytesWritten++] = frame.reserved;
    outputBuffer[bytesWritten++] = getHighByte(frame.seq);
    outputBuffer[bytesWritten++] = getLowByte(frame.seq);
    outputBuffer[bytesWritten++] = std::to_underlying(frame.type);

    std::copy(frame.payload.begin(), frame.payload.end(), outputBuffer.begin() + bytesWritten);
    bytesWritten += frame.payload.size();
    
    const auto CRC_Value{ CRC::Calculate(&outputBuffer[0], bytesWritten, CRC::CRC_16_CCITTFALSE()) };

    outputBuffer[bytesWritten++] = getHighByte(CRC_Value);
    outputBuffer[bytesWritten++] = getLowByte(CRC_Value);

    return bytesWritten;
}

constexpr bool RawFrameCodec::isEnoughBufferSize(size_t bufferSize, size_t framePayloadSize) noexcept {
    return bufferSize >= (framePayloadSize + Frame::CRC_Size + Frame::Header_Size);
}
