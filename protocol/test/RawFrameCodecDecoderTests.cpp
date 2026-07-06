#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "BitsUtilities.hpp"
#include "CRC.h"
#include "ProtocolErrors.hpp"
#include "FrameCodec.hpp"
#include "FrameDecoder.hpp"
#include "SpanUtilities.hpp"

namespace {
constexpr uint8_t kVersion = Frame::Actual_Frame_Version;
constexpr PackageKind kKind = PackageKind::Request;
constexpr uint8_t kFlags = 0xA5;
constexpr uint8_t kReserved = 0x00;
constexpr uint16_t kSeq = 0x1234;
constexpr MessageType kType = MessageType::SendRadioPacket;
constexpr uint8_t kKindValue = static_cast<uint8_t>(kKind);
constexpr uint8_t kTypeValue = static_cast<uint8_t>(kType);

std::vector<uint8_t> buildPayload(size_t size) {
    std::vector<uint8_t> payload(size, 0x00);

    for (size_t i = 0; i < size; ++i) {
        payload[i] = static_cast<uint8_t>((i * 31U) & 0xFFU);
    }

    return payload;
}

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

TEST(RawFrameCodecTests, EncodePreservesFlagsAndReservedMatrix) {
    std::vector<uint8_t> payload{ 0x10, 0x20 };
    const std::array<uint8_t, 6> values{ 0x00, 0x01, 0x7F, 0x80, 0xA5, 0xFF };

    for (const auto flags : values) {
        for (const auto reserved : values) {
            auto frame = makeFrame(payload);
            frame.flags = flags;
            frame.reserved = reserved;

            std::vector<uint8_t> output(Frame::Header_Size + payload.size() + Frame::CRC_Size, 0xEE);
            const auto encoded = FrameCodec::encode(frame, output);
            ASSERT_TRUE(encoded.has_value()) << "flags=" << static_cast<int>(flags)
                                             << " reserved=" << static_cast<int>(reserved);
            EXPECT_EQ(encoded.value()[2], flags) << "flags=" << static_cast<int>(flags)
                                                 << " reserved=" << static_cast<int>(reserved);
            EXPECT_EQ(encoded.value()[3], reserved) << "flags=" << static_cast<int>(flags)
                                                    << " reserved=" << static_cast<int>(reserved);
        }
    }
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

TEST(RawFrameCodecTests, MinimumOutputBufferSizeMatchesFrameLayout) {
    for (const auto payloadSize : std::array<size_t, 4>{
             0U,
             1U,
             static_cast<size_t>(FrameCodec::Max_Frame_Payload_Size),
             static_cast<size_t>(FrameCodec::Max_Frame_Payload_Size + 1U) }) {
        auto payload = buildPayload(payloadSize);
        const auto frame = makeFrame(payload);

        EXPECT_EQ(FrameCodec::minimumOutputBufferSize(frame), Frame::Header_Size + payloadSize + Frame::CRC_Size);
    }
}

TEST(RawFrameCodecTests, EncodeAcceptsMaximumPayloadSize) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size);
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), bufferSize);
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), output.begin() + Frame::Header_Size));
}

TEST(RawFrameCodecTests, EncodeRejectsPayloadLargerThanMaximum) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size + 1U);
    const auto frame = makeFrame(payload);

    const size_t bufferSize = Frame::Header_Size + payload.size() + Frame::CRC_Size;
    std::vector<uint8_t> output(bufferSize);

    const auto result = FrameCodec::encode(frame, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::FramePayloadTooLong);
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

TEST(RawFrameCodecTests, EncodeAcceptsAdjacentPayloadAndOutputBuffers) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    const size_t outputSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> storage(payload.size() + outputSize, 0xEE);
    std::copy(payload.begin(), payload.end(), storage.begin());

    Frame frame{};
    frame.version = kVersion;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = std::span<const uint8_t>(storage.data(), payload.size());

    auto outputSpan = std::span<uint8_t>(storage.data() + payload.size(), outputSize);
    const auto result = FrameCodec::encode(frame, outputSpan);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().data(), outputSpan.data());
    EXPECT_EQ(result.value().size(), outputSize);
}

TEST(SpanUtilitiesTests, AdjacentSpansDoNotOverlap) {
    std::array<uint8_t, 8> storage{};
    const auto left = std::span<const uint8_t>(storage.data(), 4);
    const auto right = std::span<const uint8_t>(storage.data() + 4, 4);

    EXPECT_FALSE(spansOverlap(left, right));
}

TEST(SpanUtilitiesTests, PartialOverlapAtBeginningOrEndIsDetected) {
    std::array<uint8_t, 8> storage{};
    const auto left = std::span<const uint8_t>(storage.data(), 5);
    const auto right = std::span<const uint8_t>(storage.data() + 3, 5);

    EXPECT_TRUE(spansOverlap(left, right));
    EXPECT_TRUE(spansOverlap(right, left));
}

TEST(RawFrameDecoderTests, DecodeReturnsErrorWhenBufferTooSmall) {
    std::vector<uint8_t> raw(Frame::Header_Size + Frame::CRC_Size - 1, 0x00);

    std::vector<uint8_t> decodedPayload(1, 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InputBufferTooSmall);
    EXPECT_EQ(decodedPayload[0], 0xEE);
}

TEST(RawFrameDecoderTests, MinimumPayloadBufferSizeReportsRawPayloadBytes) {
    EXPECT_EQ(FrameDecoder::minimumPayloadBufferSize(FrameDecoder::Min_Raw_Buffer_Size).value(), 0U);
    EXPECT_EQ(FrameDecoder::minimumPayloadBufferSize(FrameDecoder::Min_Raw_Buffer_Size + 1U).value(), 1U);
    EXPECT_EQ(FrameDecoder::minimumPayloadBufferSize(FrameDecoder::Max_Input_Buffer_Size).value(),
              FrameCodec::Max_Frame_Payload_Size);

    const auto tooSmall = FrameDecoder::minimumPayloadBufferSize(FrameDecoder::Min_Raw_Buffer_Size - 1U);
    ASSERT_FALSE(tooSmall.has_value());
    EXPECT_EQ(tooSmall.error(), ProtocolErrors::InputBufferTooSmall);
}

TEST(RawFrameDecoderTests, DecodeReturnsErrorOnInvalidCrc) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);
    raw.back() ^= 0xFF;

    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMismatch);
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

TEST(RawFrameDecoderTests, DecodePreservesFlagsAndReservedMatrix) {
    std::vector<uint8_t> payload{ 0x10, 0x20 };
    const std::array<uint8_t, 6> values{ 0x00, 0x01, 0x7F, 0x80, 0xA5, 0xFF };

    for (const auto flags : values) {
        for (const auto reserved : values) {
            auto raw = buildRawBuffer(kVersion, kKindValue, flags, reserved, kSeq, kTypeValue, payload);
            std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);

            const auto decoded = FrameDecoder::decode(raw, decodedPayload);
            ASSERT_TRUE(decoded.has_value()) << "flags=" << static_cast<int>(flags)
                                             << " reserved=" << static_cast<int>(reserved);
            EXPECT_EQ(decoded.value().flags, flags) << "flags=" << static_cast<int>(flags)
                                                    << " reserved=" << static_cast<int>(reserved);
            EXPECT_EQ(decoded.value().reserved, reserved) << "flags=" << static_cast<int>(flags)
                                                          << " reserved=" << static_cast<int>(reserved);
            EXPECT_TRUE(std::equal(payload.begin(), payload.end(), decoded.value().payload.begin()));
        }
    }
}

TEST(RawFrameCodecDecoderTests, SeqRoundTripsBoundaryValues) {
    auto payload = buildPayload(3);

    for (const auto seq : std::array<uint16_t, 5>{ 0x0000U, 0x0001U, 0x00FFU, 0x0100U, 0xFFFFU }) {
        auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, seq, kTypeValue, payload);
        std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);

        const auto decoded = FrameDecoder::decode(raw, decodedPayload);
        ASSERT_TRUE(decoded.has_value()) << "seq=" << seq;
        EXPECT_EQ(decoded.value().seq, seq);
    }
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
    EXPECT_EQ(decoded.error(), ProtocolErrors::FrameVersionMismatch);
}

TEST(RawFrameDecoderTests, CrcMismatchTakesPrecedenceOverInvalidHeaderFields) {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };
    auto raw = buildRawBuffer(0x02, 0xFF, kFlags, kReserved, kSeq, 0x99, payload);
    raw.back() ^= 0xFF;

    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    ASSERT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMismatch);
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

TEST(RawFrameDecoderTests, DecodeAcceptsMaximumPayloadSize) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size);
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    ASSERT_TRUE(decoded.has_value());

    EXPECT_EQ(decoded.value().payload.size(), payload.size());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), decodedPayload.begin()));
}

TEST(RawFrameDecoderTests, DecodeRejectsPayloadLargerThanMaximum) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size + 1U);
    auto raw = buildRawBufferWithDefaults(kKindValue, kTypeValue, payload);

    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = FrameDecoder::decode(raw, decodedPayload);
    ASSERT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InputBufferTooLong);
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
