#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "BitsUtilities.hpp"
#include "CRC.h"
#include "RawFrameCodec.hpp"
#include "RawFrameDecoder.hpp"

static Frame makeFrame(std::vector<uint8_t>& payload) {
    Frame frame{};
    frame.version = 2;
    frame.kind = PackageKind::Request;
    frame.flags = 0xA5;
    frame.reserved = 0;
    frame.seq = 0x1234;
    frame.type = MessageType::SendRadioPacket;
    frame.payload = std::span<uint8_t>(payload.data(), payload.size());
    return frame;
}

static std::vector<uint8_t> encodeFrameOrFail(const Frame& frame) {
    const size_t bufferSize = Frame::Header_Size + frame.payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = RawFrameCodec::encodeFrameToRaw(frame, output);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, bufferSize);

    return output;
}

TEST(RawFrameCodecTests, EncodeReturnsNulloptWhenBufferTooSmall) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size - 1;
    std::vector<uint8_t> output(bufferSize);

    const auto result = RawFrameCodec::encodeFrameToRaw(frame, output);
    EXPECT_FALSE(result.has_value());
}

TEST(RawFrameCodecTests, EncodeWritesHeaderPayloadAndCrc) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = RawFrameCodec::encodeFrameToRaw(frame, output);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, bufferSize);

    EXPECT_EQ(output[0], frame.version);
    EXPECT_EQ(output[1], static_cast<uint8_t>(frame.kind));
    EXPECT_EQ(output[2], frame.flags);
    EXPECT_EQ(output[3], frame.reserved);
    EXPECT_EQ(output[4], 0x12);
    EXPECT_EQ(output[5], 0x34);
    EXPECT_EQ(output[6], static_cast<uint8_t>(frame.type));

    EXPECT_EQ(output[7], payload[0]);
    EXPECT_EQ(output[8], payload[1]);
    EXPECT_EQ(output[9], payload[2]);

    const auto expectedCrc = CRC::Calculate(output.data(), bufferSize - Frame::CRC_Size, CRC::CRC_16_CCITTFALSE());
    EXPECT_EQ(output[bufferSize - 2], getHighByte(expectedCrc));
    EXPECT_EQ(output[bufferSize - 1], getLowByte(expectedCrc));
}

TEST(RawFrameDecoderTests, DecodeReturnsNulloptOnInvalidCrc) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = RawFrameCodec::encodeFrameToRaw(frame, output);
    ASSERT_TRUE(result.has_value());

    output[bufferSize - 1] ^= 0xFF;

    std::vector<uint8_t> decodedPayload(payload.size());
    Frame decodedFrame{};
    decodedFrame.payload = std::span<uint8_t>(decodedPayload.data(), decodedPayload.size());

    const auto decoded = RawFrameDecoder::decodeFrameFromRaw(output, decodedFrame);
    EXPECT_FALSE(decoded);
}

TEST(RawFrameDecoderTests, DecodePopulatesFieldsAndPayload) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = RawFrameCodec::encodeFrameToRaw(frame, output);
    ASSERT_TRUE(result.has_value());

    std::vector<uint8_t> decodedPayload(payload.size());
    Frame decodedFrame{};
    decodedFrame.version = 9;
    decodedFrame.kind = PackageKind::Error;
    decodedFrame.flags = 0xFF;
    decodedFrame.reserved = 0xFF;
    decodedFrame.seq = 0xFFFF;
    decodedFrame.type = MessageType::GenericError;
    decodedFrame.payload = std::span<uint8_t>(decodedPayload.data(), decodedPayload.size());

    const auto decoded = RawFrameDecoder::decodeFrameFromRaw(output, decodedFrame);
    EXPECT_FALSE(decoded);
}

TEST(RawFrameDecoderTests, DecodeReturnsFalseWhenPayloadSpanTooSmall) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30, 0x40 };
    const auto frame = makeFrame(payload);

    auto output = encodeFrameOrFail(frame);

    std::vector<uint8_t> decodedPayload(2, 0xEE);
    Frame decodedFrame{};
    decodedFrame.payload = std::span<uint8_t>(decodedPayload.data(), decodedPayload.size());

    const auto decoded = RawFrameDecoder::decodeFrameFromRaw(output, decodedFrame);
    EXPECT_FALSE(decoded);

    EXPECT_EQ(decodedPayload[0], 0xEE);
    EXPECT_EQ(decodedPayload[1], 0xEE);
}

TEST(RawFrameDecoderTests, DecodeReturnsFalseWhenBufferHasOnlyCrc) {
    std::vector<uint8_t> output{ 0x00, 0x01 };

    std::vector<uint8_t> decodedPayload(1, 0xAB);
    Frame decodedFrame{};
    decodedFrame.payload = std::span<uint8_t>(decodedPayload.data(), decodedPayload.size());

    const auto decoded = RawFrameDecoder::decodeFrameFromRaw(output, decodedFrame);
    EXPECT_FALSE(decoded);
}

TEST(RawFrameDecoderTests, DecodeReturnsFalseWhenBufferIsTruncated) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const auto frame = makeFrame(payload);

    auto output = encodeFrameOrFail(frame);
    output.pop_back();

    std::vector<uint8_t> decodedPayload(payload.size(), 0xAB);
    Frame decodedFrame{};
    decodedFrame.payload = std::span<uint8_t>(decodedPayload.data(), decodedPayload.size());

    const auto decoded = RawFrameDecoder::decodeFrameFromRaw(output, decodedFrame);
    EXPECT_FALSE(decoded);
}
