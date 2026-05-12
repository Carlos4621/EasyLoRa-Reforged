#include "RawFrameCodec.hpp"

std::expected<size_t, ProtocolErrors> RawFrameCodec::encodeFrameToRaw(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept {
    if (!isEnoughBufferSize(outputBuffer.size(), frame.payload.size())) {
        return std::unexpected(ProtocolErrors::BufferTooSmall);
    }
    
    size_t bytesWritten{ 0 };
    outputBuffer[bytesWritten++] = frame.version;

    if (!insertPackageKind(outputBuffer, std::to_underlying(frame.kind), bytesWritten)) {
        return std::unexpected(ProtocolErrors::InvalidPackageKind);
    }
    
    outputBuffer[bytesWritten++] = frame.flags;
    outputBuffer[bytesWritten++] = frame.reserved;
    insertTwoBytes(outputBuffer, frame.seq, bytesWritten);

    if (!insertMessageType(outputBuffer, std::to_underlying(frame.type), bytesWritten)) {
        return std::unexpected(ProtocolErrors::InvalidMessageType);
    }

    insertPayload(outputBuffer, frame, bytesWritten);
    insertCRC(outputBuffer, bytesWritten);

    return bytesWritten;
}

constexpr bool RawFrameCodec::isEnoughBufferSize(size_t bufferSize, size_t framePayloadSize) noexcept {
    return bufferSize >= (framePayloadSize + Frame::CRC_Size + Frame::Header_Size);
}

void RawFrameCodec::insertCRC(std::span<uint8_t> buffer, size_t& bytesWritten) noexcept {
    const auto CRC_Value{ CRC::Calculate(&buffer[0], bytesWritten, CRC::CRC_16_CCITTFALSE()) };

    insertTwoBytes(buffer, CRC_Value, bytesWritten);
}

void RawFrameCodec::insertPayload(std::span<uint8_t> buffer, const Frame& frame, size_t& bytesWritten) noexcept {
    std::copy(frame.payload.begin(), frame.payload.end(), buffer.begin() + bytesWritten);
    bytesWritten += frame.payload.size();
}

bool RawFrameCodec::insertMessageType(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) {
    if (!isValidMessageType(value)) {
        return false;
    }
    buffer[bytesWritten++] = value;

    return true;
}

bool RawFrameCodec::insertPackageKind(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) {
    if (!isValidPackageKind(value)) {
        return false;
    }
    buffer[bytesWritten++] = value;

    return true;
}

void RawFrameCodec::insertTwoBytes(std::span<uint8_t> buffer, uint16_t value, size_t &bytesWritten) {
    buffer[bytesWritten++] = getHighByte(value);
    buffer[bytesWritten++] = getLowByte(value);
}
