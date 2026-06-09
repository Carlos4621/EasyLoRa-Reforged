#include <algorithm>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "BitsUtilities.hpp"
#include "CRC.h"
#include "ProtocolErrors.hpp"
#include "FrameCodec.hpp"
#include "FrameDecoder.hpp"

namespace {
constexpr uint8_t kVersion = Frame::Actual_Frame_Version;
constexpr PackageKind kKind = PackageKind::Request;
constexpr uint8_t kFlags = 0xA5;
constexpr uint8_t kReserved = 0x00;
constexpr uint16_t kSeq = 0x1234;
constexpr MessageType kType = MessageType::SendRadioPacket;
constexpr uint8_t kKindValue = static_cast<uint8_t>(kKind);
constexpr uint8_t kTypeValue = static_cast<uint8_t>(kType);

Frame makeFrame(std::vector<uint8_t>& payload) {
    Frame frame{};
    frame.version = kVersion;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = std::span<uint8_t>(payload.data(), payload.size());
    return frame;
}

std::vector<uint8_t> buildRawBuffer(uint8_t version,
                                    uint8_t kindValue,
                                    uint8_t flags,
                                    uint8_t reserved,
                                    uint16_t seq,
                                    uint8_t typeValue,
                                    std::span<const uint8_t> payload) {
    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    size_t bytesWritten = 0;
    output[bytesWritten++] = version;
    output[bytesWritten++] = kindValue;
    output[bytesWritten++] = flags;
    output[bytesWritten++] = reserved;
    output[bytesWritten++] = getHighByte(seq);
    output[bytesWritten++] = getLowByte(seq);
    output[bytesWritten++] = typeValue;

    std::copy(payload.begin(), payload.end(), output.begin() + bytesWritten);
    bytesWritten += payload.size();

    const auto crcValue = CRC::Calculate(output.data(), bytesWritten, CRC::CRC_16_CCITTFALSE());
    output[bytesWritten++] = getHighByte(crcValue);
    output[bytesWritten++] = getLowByte(crcValue);

    return output;
}

std::vector<uint8_t> buildRawBufferWithDefaults(uint8_t kindValue,
                                                uint8_t typeValue,
                                                std::span<const uint8_t> payload) {
    return buildRawBuffer(kVersion, kindValue, kFlags, kReserved, kSeq, typeValue, payload);
}
} // namespace

TEST(RawFrameCodecTests, EncodeReturnsErrorWhenBufferTooSmall) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size - 1;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::OutputBufferTooSmall);
}

TEST(RawFrameCodecTests, EncodeWritesHeaderPayloadAndCrc) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), bufferSize);

    EXPECT_EQ(output[0], kVersion);
    EXPECT_EQ(output[1], kKindValue);
    EXPECT_EQ(output[2], kFlags);
    EXPECT_EQ(output[3], kReserved);
    EXPECT_EQ(output[4], getHighByte(kSeq));
    EXPECT_EQ(output[5], getLowByte(kSeq));
    EXPECT_EQ(output[6], kTypeValue);

    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), output.begin() + Frame::Header_Size));

    const auto expectedCrc = CRC::Calculate(output.data(), bufferSize - Frame::CRC_Size, CRC::CRC_16_CCITTFALSE());
    EXPECT_EQ(output[bufferSize - 2], getHighByte(expectedCrc));
    EXPECT_EQ(output[bufferSize - 1], getLowByte(expectedCrc));
}

TEST(RawFrameCodecTests, EncodeSupportsEmptyPayload) {
    std::vector<uint8_t> payload{};
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), bufferSize);

    EXPECT_EQ(output[0], kVersion);
    EXPECT_EQ(output[1], kKindValue);
    EXPECT_EQ(output[2], kFlags);
    EXPECT_EQ(output[3], kReserved);
    EXPECT_EQ(output[4], getHighByte(kSeq));
    EXPECT_EQ(output[5], getLowByte(kSeq));
    EXPECT_EQ(output[6], kTypeValue);

    const auto expectedCrc = CRC::Calculate(output.data(), bufferSize - Frame::CRC_Size, CRC::CRC_16_CCITTFALSE());
    EXPECT_EQ(output[bufferSize - 2], getHighByte(expectedCrc));
    EXPECT_EQ(output[bufferSize - 1], getLowByte(expectedCrc));
}

TEST(RawFrameCodecTests, EncodeReturnsErrorWhenPackageKindInvalid) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    Frame frame = makeFrame(payload);
    frame.kind = static_cast<PackageKind>(0xFF);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::InvalidPackageKind);
}

TEST(RawFrameCodecTests, EncodeReturnsErrorWhenMessageTypeInvalid) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    Frame frame = makeFrame(payload);
    frame.type = static_cast<MessageType>(0x99);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::InvalidMessageType);
}

TEST(RawFrameCodecTests, EncodeReturnsErrorWhenPayloadOverlapsHeader) {
    std::vector<uint8_t> output(Frame::Header_Size + 4 + Frame::CRC_Size, 0xAA);
    Frame frame{};
    frame.version = kVersion;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = std::span<const uint8_t>(output.data(), 4);

    const auto result = FrameCodec::encode(frame, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::SameBufferError);
}

TEST(RawFrameCodecTests, EncodeSupportsPayloadAlreadyAfterHeader) {
    constexpr size_t payloadSize{ 4 };
    const size_t encodedSize{ Frame::Header_Size + payloadSize + Frame::CRC_Size };
    std::vector<uint8_t> output(encodedSize, 0xAA);
    std::vector<uint8_t> expectedPayload{ 0x10, 0x20, 0x30, 0x40 };
    std::copy(expectedPayload.begin(), expectedPayload.end(), output.begin() + Frame::Header_Size);

    Frame frame{};
    frame.version = kVersion;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = std::span<const uint8_t>(output.data() + Frame::Header_Size, payloadSize);

    const auto result = FrameCodec::encode(frame, output);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), encodedSize);
    EXPECT_TRUE(std::equal(expectedPayload.begin(), expectedPayload.end(), output.begin() + Frame::Header_Size));
}

TEST(RawFrameCodecTests, EncodeSupportsPayloadInsideOutputWithAllowedOffset) {
    constexpr size_t payloadSize{ 4 };
    const size_t encodedSize{ Frame::Header_Size + payloadSize + Frame::CRC_Size };
    std::vector<uint8_t> output(encodedSize + 1, 0xAA);
    std::vector<uint8_t> expectedPayload{ 0x10, 0x20, 0x30, 0x40 };
    std::copy(expectedPayload.begin(), expectedPayload.end(), output.begin() + Frame::Header_Size + 1);

    Frame frame{};
    frame.version = kVersion;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = std::span<const uint8_t>(output.data() + Frame::Header_Size + 1, payloadSize);

    const auto result = FrameCodec::encode(frame, output);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), encodedSize);
    EXPECT_TRUE(std::equal(expectedPayload.begin(), expectedPayload.end(), output.begin() + Frame::Header_Size));
}

TEST(RawFrameCodecTests, EncodeCurrentlyAllowsPayloadOverlappingCrcArea) {
    constexpr size_t payloadSize{ 4 };
    const size_t encodedSize{ Frame::Header_Size + payloadSize + Frame::CRC_Size };
    std::vector<uint8_t> output(encodedSize + payloadSize, 0xAA);
    std::vector<uint8_t> expectedPayload{ 0x10, 0x20, 0x30, 0x40 };
    std::copy(expectedPayload.begin(), expectedPayload.end(), output.begin() + Frame::Header_Size + payloadSize - 1);

    Frame frame{};
    frame.version = kVersion;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = std::span<const uint8_t>(output.data() + Frame::Header_Size + payloadSize - 1, payloadSize);

    const auto result = FrameCodec::encode(frame, std::span<uint8_t>(output.data(), encodedSize));
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::equal(expectedPayload.begin(), expectedPayload.end(), output.begin() + Frame::Header_Size));
}

TEST(RawFrameDecoderTests, DecodeReturnsErrorWhenBufferTooSmall) {
    std::vector<uint8_t> raw(Frame::Header_Size + Frame::CRC_Size - 1, 0x00);

    std::vector<uint8_t> decodedPayload(1, 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::OutputBufferTooSmall);
    EXPECT_EQ(decodedPayload[0], 0xEE);
}

TEST(RawFrameDecoderTests, DecodeReturnsErrorOnInvalidCrc) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);
    raw.back() ^= 0xFF;

    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMisMatch);
    EXPECT_TRUE(std::all_of(decodedPayload.begin(), decodedPayload.end(), [](uint8_t value) {
        return value == 0xEE;
    }));
}

TEST(RawFrameDecoderTests, DecodeReturnsErrorWhenPayloadSpanTooSmall) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30, 0x40 };
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(2, 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::FramePayloadTooSmall);
    EXPECT_EQ(decodedPayload[0], 0xEE);
    EXPECT_EQ(decodedPayload[1], 0xEE);
}

TEST(RawFrameDecoderTests, DecodePopulatesFieldsAndPayload) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(payload.size(), 0x00);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    ASSERT_TRUE(decoded.has_value());

    const auto& decodedFrame = decoded.value();
    EXPECT_EQ(decodedFrame.version, kVersion);
    EXPECT_EQ(decodedFrame.kind, kKind);
    EXPECT_EQ(decodedFrame.flags, kFlags);
    EXPECT_EQ(decodedFrame.reserved, kReserved);
    EXPECT_EQ(decodedFrame.seq, kSeq);
    EXPECT_EQ(decodedFrame.type, kType);
    EXPECT_EQ(decodedPayload, payload);
}

TEST(RawFrameDecoderTests, DecodeAllowsLargerPayloadSpan) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(6, 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    ASSERT_TRUE(decoded.has_value());

    const auto& decodedFrame = decoded.value();
    EXPECT_EQ(decodedFrame.payload.size(), payload.size());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
    EXPECT_EQ(decodedPayload[3], 0xEE);
    EXPECT_EQ(decodedPayload[4], 0xEE);
    EXPECT_EQ(decodedPayload[5], 0xEE);
}

TEST(RawFrameDecoderTests, DecodeRejectsInvalidPackageKind) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBufferWithDefaults(0xFF, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(payload.size(), 0x00);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InvalidPackageKind);
}

TEST(RawFrameDecoderTests, DecodeRejectsInvalidMessageType) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBufferWithDefaults(kKindValue, 0x99, payload);

    std::vector<uint8_t> decodedPayload(payload.size(), 0x00);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InvalidMessageType);
}

TEST(RawFrameDecoderTests, DecodeRejectsVersionMismatch) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBuffer(0x02, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(payload.size(), 0x00);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::FrameVersionMissmatch);
}

TEST(RawFrameDecoderTests, DecodeSupportsEmptyPayload) {
    std::vector<uint8_t> payload{};
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload{};
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    ASSERT_TRUE(decoded.has_value());

    const auto& decodedFrame = decoded.value();
    EXPECT_EQ(decodedFrame.version, kVersion);
    EXPECT_EQ(decodedFrame.kind, kKind);
    EXPECT_EQ(decodedFrame.flags, kFlags);
    EXPECT_EQ(decodedFrame.reserved, kReserved);
    EXPECT_EQ(decodedFrame.seq, kSeq);
    EXPECT_EQ(decodedFrame.type, kType);
    EXPECT_EQ(decodedFrame.payload.size(), 0U);
    EXPECT_EQ(decodedPayload.size(), 0U);
}

TEST(RawFrameDecoderTests, DecodeRejectsInPlacePayloadBuffer) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30, 0x40 };
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    const auto inputSpan = std::span<const uint8_t>(raw.data(), raw.size());
    auto outputSpan = std::span<uint8_t>(raw.data(), raw.size());
    const auto decoded = FrameDecoder::decode(inputSpan, outputSpan);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::SameBufferError);
}
