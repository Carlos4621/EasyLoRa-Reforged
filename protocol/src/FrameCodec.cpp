#include "FrameCodec.hpp"

#include <cstring>
#include "BitsUtilities.hpp"

std::expected<std::span<uint8_t>, ProtocolErrors> FrameCodec::encode(const Frame& frame, std::span<uint8_t> outputBuffer) noexcept {
    if (frame.payload.size() > Max_Frame_Payload_Size) {
        return std::unexpected{ ProtocolErrors::FramePayloadTooLong };
    }

    if (outputBuffer.size() < minimumOutputBufferSize(frame)) {
        return std::unexpected(ProtocolErrors::OutputBufferTooSmall);
    }

    const auto headerSpan{ std::span<const uint8_t>(outputBuffer.first(Frame::Header_Size)) };
    if (spansOverlap(frame.payload, headerSpan)) {
        return std::unexpected(ProtocolErrors::SameBufferError);
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

    return outputBuffer.first(bytesWritten);
}

void FrameCodec::insertCRC(std::span<uint8_t> buffer, size_t& bytesWritten) noexcept {
    const auto CRC_Value{ CRC::Calculate(&buffer[0], bytesWritten, CRC::CRC_16_CCITTFALSE()) };

    insertTwoBytes(buffer, CRC_Value, bytesWritten);
}

void FrameCodec::insertPayload(std::span<uint8_t> buffer, const Frame& frame, size_t& bytesWritten) noexcept {
    if (!frame.payload.empty()) {
        std::memmove(buffer.data() + bytesWritten, frame.payload.data(), frame.payload.size());
    }
    bytesWritten += frame.payload.size();
}

bool FrameCodec::insertMessageType(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) noexcept {
    if (!isValidMessageType(value)) {
        return false;
    }
    buffer[bytesWritten++] = value;

    return true;
}

bool FrameCodec::insertPackageKind(std::span<uint8_t> buffer, uint8_t value, size_t& bytesWritten) noexcept {
    if (!isValidPackageKind(value)) {
        return false;
    }
    buffer[bytesWritten++] = value;

    return true;
}

void FrameCodec::insertTwoBytes(std::span<uint8_t> buffer, uint16_t value, size_t &bytesWritten) noexcept {
    buffer[bytesWritten++] = getHighByte(value);
    buffer[bytesWritten++] = getLowByte(value);
}
