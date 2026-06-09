#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "BitsUtilities.hpp"
#include "CRC.h"
#include "CobsrCodec.hpp"
#include "CobsrDecoder.hpp"
#include "Frame.hpp"
#include "ProtocolCodec.hpp"
#include "ProtocolDecoder.hpp"
#include "ProtocolErrors.hpp"
#include "cobs/cobsr.h"

namespace {
constexpr uint8_t kVersion{ Frame::Actual_Frame_Version };
constexpr PackageKind kKind{ PackageKind::Request };
constexpr uint8_t kFlags{ 0xA5 };
constexpr uint8_t kReserved{ 0x00 };
constexpr uint16_t kSeq{ 0x1234 };
constexpr MessageType kType{ MessageType::SendRadioPacket };
constexpr uint8_t kKindValue{ static_cast<uint8_t>(kKind) };
constexpr uint8_t kTypeValue{ static_cast<uint8_t>(kType) };

std::vector<uint8_t> buildPayload(size_t size) {
    std::vector<uint8_t> payload(size, 0x00);

    for (size_t i = 0; i < size; ++i) {
        uint8_t value{ static_cast<uint8_t>((i * 31U) & 0xFFU) };
        if ((i % 7U) == 0U) {
            value = 0x00;
        }
        payload[i] = value;
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
    frame.payload = std::span<const uint8_t>(payload.data(), payload.size());
    return frame;
}

std::vector<uint8_t> buildRawBuffer(uint8_t version,
                                    uint8_t kindValue,
                                    uint8_t flags,
                                    uint8_t reserved,
                                    uint16_t seq,
                                    uint8_t typeValue,
                                    std::span<const uint8_t> payload) {
    const size_t bufferSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> output(bufferSize);

    size_t bytesWritten{ 0 };
    output[bytesWritten++] = version;
    output[bytesWritten++] = kindValue;
    output[bytesWritten++] = flags;
    output[bytesWritten++] = reserved;
    output[bytesWritten++] = getHighByte(seq);
    output[bytesWritten++] = getLowByte(seq);
    output[bytesWritten++] = typeValue;

    std::copy(payload.begin(), payload.end(), output.begin() + bytesWritten);
    bytesWritten += payload.size();

    const auto crcValue{ CRC::Calculate(output.data(), bytesWritten, CRC::CRC_16_CCITTFALSE()) };
    output[bytesWritten++] = getHighByte(crcValue);
    output[bytesWritten++] = getLowByte(crcValue);

    return output;
}

std::expected<std::span<uint8_t>, ProtocolErrors> encodeRawWithCobsr(std::vector<uint8_t>& raw,
                                                                     std::vector<uint8_t>& encodedBuffer) {
    return CobsrCodec::addCOBSR(raw, encodedBuffer);
}

void expectFrameMatches(const Frame& decodedFrame, std::span<const uint8_t> expectedPayload) {
    EXPECT_EQ(decodedFrame.version, kVersion);
    EXPECT_EQ(decodedFrame.kind, kKind);
    EXPECT_EQ(decodedFrame.flags, kFlags);
    EXPECT_EQ(decodedFrame.reserved, kReserved);
    EXPECT_EQ(decodedFrame.seq, kSeq);
    EXPECT_EQ(decodedFrame.type, kType);
    EXPECT_EQ(decodedFrame.payload.size(), expectedPayload.size());
    EXPECT_TRUE(std::equal(expectedPayload.begin(), expectedPayload.end(), decodedFrame.payload.begin()));
}
} // namespace

TEST(CobsrCodecTests, EncodeReturnsErrorWhenBufferTooSmall) {
    const auto raw = buildPayload(32);
    const size_t requiredSize = COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size());
    std::vector<uint8_t> output(requiredSize - 1, 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, output);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::OutputBufferTooSmall);
}

TEST(CobsrCodecTests, EncodeSupportsEmptyInput) {
    const std::vector<uint8_t> raw{};
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());
    EXPECT_EQ(encoded.value().size(), 0U);
}

TEST(CobsrCodecTests, EncodeSupportsInPlaceWithOffset) {
    const auto raw = buildPayload(128);
    const size_t requiredSize = COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size());
    const size_t offset = COBSR_ENCODE_SRC_OFFSET(raw.size());

    std::vector<uint8_t> buffer(requiredSize + offset, 0xCD);
    std::copy(raw.begin(), raw.end(), buffer.begin() + offset);

    const auto rawSpan = std::span<const uint8_t>(buffer.data() + offset, raw.size());
    auto outputSpan = std::span<uint8_t>(buffer.data(), requiredSize);
    const auto encodedInPlace = CobsrCodec::addCOBSR(rawSpan, outputSpan);
    ASSERT_TRUE(encodedInPlace.has_value());

    std::vector<uint8_t> baseline(requiredSize, 0xEE);
    const auto encodedBaseline = CobsrCodec::addCOBSR(raw, baseline);
    ASSERT_TRUE(encodedBaseline.has_value());

    EXPECT_EQ(encodedInPlace.value().size(), encodedBaseline.value().size());
    EXPECT_TRUE(std::equal(encodedBaseline.value().begin(),
                           encodedBaseline.value().end(),
                           encodedInPlace.value().begin()));
}

TEST(CobsrCodecTests, EncodeRejectsOverlappingBufferWithoutOffset) {
    const auto raw = buildPayload(64);
    const size_t requiredSize = COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size());
    std::vector<uint8_t> buffer(requiredSize, 0xCD);

    std::copy(raw.begin(), raw.end(), buffer.begin());

    const auto rawSpan = std::span<const uint8_t>(buffer.data(), raw.size());
    auto outputSpan = std::span<uint8_t>(buffer.data(), buffer.size());
    const auto encoded = CobsrCodec::addCOBSR(rawSpan, outputSpan);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::SameBufferError);
}

TEST(CobsrCodecDecoderTests, EncodeDecodeRoundTrip) {
    const auto raw = buildPayload(300);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());
    EXPECT_TRUE(std::none_of(encoded.value().begin(), encoded.value().end(), [](uint8_t value) {
        return value == 0x00;
    }));

    std::vector<uint8_t> decodedBuffer(encoded.value().size(), 0xEE);
    const auto decoded = CobsrDecoder::decode(encoded.value(), decodedBuffer);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value().size(), raw.size());
    EXPECT_TRUE(std::equal(raw.begin(), raw.end(), decodedBuffer.begin()));
}

TEST(CobsrDecoderTests, DecodeReturnsErrorWhenOutputBufferTooSmall) {
    const auto raw = buildPayload(48);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> decodedBuffer(encoded.value().size() - 1, 0xEE);
    const auto decoded = CobsrDecoder::decode(encoded.value(), decodedBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::OutputBufferTooSmall);
}

TEST(CobsrDecoderTests, DecodeReturnsErrorWhenInputBufferEmpty) {
    std::vector<uint8_t> encodedBuffer{};
    std::vector<uint8_t> decodedBuffer(1, 0xEE);

    const auto decoded = CobsrDecoder::decode(encodedBuffer, decodedBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::EmptyInputBuffer);
}

TEST(CobsrDecoderTests, DecodeRejectsOverlappingBufferWithDifferentStart) {
    const auto raw = buildPayload(32);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> shared(encoded.value().size() + 1, 0xEE);
    std::copy(encoded.value().begin(), encoded.value().end(), shared.begin() + 1);

    const auto inputSpan = std::span<const uint8_t>(shared.data() + 1, encoded.value().size());
    auto outputSpan = std::span<uint8_t>(shared.data(), encoded.value().size());
    const auto decoded = CobsrDecoder::decode(inputSpan, outputSpan);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::SameBufferError);
}

TEST(CobsrDecoderTests, DecodeRejectsZeroByteInInput) {
    std::vector<uint8_t> encodedBuffer{ 0x11, 0x00, 0x22 };
    std::vector<uint8_t> decodedBuffer(encodedBuffer.size(), 0xEE);

    const auto decoded = CobsrDecoder::decode(encodedBuffer, decodedBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::COBSRDecodeError);
}

TEST(CobsrDecoderTests, DecodeSupportsInPlaceBuffer) {
    const auto raw = buildPayload(64);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> buffer(encoded.value().begin(), encoded.value().end());
    const auto inputSpan = std::span<const uint8_t>(buffer.data(), buffer.size());
    auto outputSpan = std::span<uint8_t>(buffer.data(), buffer.size());

    const auto decoded = CobsrDecoder::decode(inputSpan, outputSpan);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value().data(), buffer.data());
    EXPECT_EQ(decoded.value().size(), raw.size());
    EXPECT_TRUE(std::equal(raw.begin(), raw.end(), buffer.begin()));
}

TEST(ProtocolCodecDecoderTests, EncodeDecodeRoundTripWithCompleteFrame) {
    auto payload = buildPayload(64);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());
    EXPECT_TRUE(std::none_of(encoded.value().begin(), encoded.value().end(), [](uint8_t value) {
        return value == 0x00;
    }));

    std::vector<uint8_t> decodedRawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), decodedRawBuffer, decodedPayload);
    ASSERT_TRUE(decoded.has_value());
    expectFrameMatches(decoded.value(), payload);
}

TEST(ProtocolCodecTests, EncodePropagatesRawBufferTooSmall) {
    auto payload = buildPayload(4);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize - 1, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::OutputBufferTooSmall);
}

TEST(ProtocolCodecTests, EncodePropagatesCobsrBufferTooSmall) {
    auto payload = buildPayload(4);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize) - 1, 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::OutputBufferTooSmall);
}

TEST(ProtocolCodecTests, EncodePropagatesInvalidPackageKind) {
    auto payload = buildPayload(4);
    auto frame = makeFrame(payload);
    frame.kind = static_cast<PackageKind>(0xFF);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::InvalidPackageKind);
}

TEST(ProtocolCodecTests, EncodePropagatesInvalidMessageType) {
    auto payload = buildPayload(4);
    auto frame = makeFrame(payload);
    frame.type = static_cast<MessageType>(0x99);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::InvalidMessageType);
}

TEST(ProtocolCodecTests, EncodePropagatesInvalidOverlap) {
    auto payload = buildPayload(4);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> sharedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame,
                                               std::span<uint8_t>(sharedBuffer.data(), rawSize),
                                               sharedBuffer);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::SameBufferError);
}

TEST(ProtocolDecoderTests, DecodePropagatesInvalidCobsr) {
    std::vector<uint8_t> encodedBuffer{ 0x11, 0x00, 0x22 };
    std::vector<uint8_t> rawBuffer(encodedBuffer.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(1, 0xEE);

    const auto decoded = ProtocolDecoder::decode(encodedBuffer, rawBuffer, payloadBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::COBSRDecodeError);
}

TEST(ProtocolDecoderTests, DecodePropagatesCrcMismatchAfterValidCobsr) {
    auto payload = buildPayload(4);
    auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);
    raw.back() ^= 0xFF;
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMisMatch);
}

TEST(ProtocolDecoderTests, DecodePropagatesPayloadOutputTooSmall) {
    auto payload = buildPayload(4);
    auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size() - 1, 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::FramePayloadTooSmall);
}

TEST(ProtocolDecoderTests, DecodePropagatesInvalidPackageKindAfterValidCobsr) {
    auto payload = buildPayload(4);
    auto raw = buildRawBuffer(kVersion, 0xFF, kFlags, kReserved, kSeq, kTypeValue, payload);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InvalidPackageKind);
}

TEST(ProtocolDecoderTests, DecodePropagatesInvalidMessageTypeAfterValidCobsr) {
    auto payload = buildPayload(4);
    auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, 0x99, payload);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InvalidMessageType);
}

TEST(ProtocolDecoderTests, DecodeAcceptsCurrentVersionAndRejectsOtherVersions) {
    auto payload = buildPayload(1);
    for (const auto version : std::array<uint8_t, 3>{ 0x00, 0x02, 0xFF }) {
        auto raw = buildRawBuffer(version, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);
        std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
        const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
        ASSERT_TRUE(encoded.has_value());

        std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
        std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
        const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
        EXPECT_FALSE(decoded.has_value());
        EXPECT_EQ(decoded.error(), ProtocolErrors::FrameVersionMissmatch);
    }

    auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    ASSERT_TRUE(decoded.has_value());
    expectFrameMatches(decoded.value(), payload);
}

TEST(ProtocolDecoderTests, DecodePreservesFlagsAndReservedBits) {
    auto payload = buildPayload(1);
    constexpr uint8_t flags{ 0xFF };
    constexpr uint8_t reserved{ 0x7E };
    auto raw = buildRawBuffer(kVersion, kKindValue, flags, reserved, kSeq, kTypeValue, payload);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value().flags, flags);
    EXPECT_EQ(decoded.value().reserved, reserved);
}

TEST(ProtocolCodecDecoderTests, RoundTripSupportsBoundaryPayloadSizes) {
    for (const size_t payloadSize : std::array<size_t, 4>{ 0U, 1U, 255U, 256U }) {
        auto payload = buildPayload(payloadSize);
        const auto frame = makeFrame(payload);
        const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
        std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
        std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

        const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
        ASSERT_TRUE(encoded.has_value()) << "payloadSize=" << payloadSize;

        std::vector<uint8_t> decodedRawBuffer(rawSize, 0xEE);
        std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
        const auto decoded = ProtocolDecoder::decode(encoded.value(), decodedRawBuffer, decodedPayload);
        ASSERT_TRUE(decoded.has_value()) << "payloadSize=" << payloadSize;
        expectFrameMatches(decoded.value(), payload);
    }
}

TEST(ProtocolCodecDecoderTests, RoundTripSupportsMaximumPayloadSize) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> decodedRawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), decodedRawBuffer, decodedPayload);
    ASSERT_TRUE(decoded.has_value());
    expectFrameMatches(decoded.value(), payload);
}

TEST(ProtocolCodecTests, EncodeRejectsPayloadLargerThanMaximum) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size + 1U);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::FramePayloadTooLong);
}

TEST(ProtocolDecoderTests, DecodeRejectsPayloadLargerThanMaximum) {
    auto payload = buildPayload(FrameCodec::Max_Frame_Payload_Size + 1U);
    auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    ASSERT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::InputBufferTooLong);
}

TEST(ProtocolCodecTests, EncodeAcceptsExactlyRequiredRawBufferAndRejectsOneByteLess) {
    auto payload = buildPayload(1);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ Frame::Header_Size + payload.size() + Frame::CRC_Size };
    std::vector<uint8_t> exactRawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(rawSize), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, exactRawBuffer, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> smallRawBuffer(rawSize - 1, 0xEE);
    const auto tooSmall = ProtocolCodec::encode(frame, smallRawBuffer, encodedBuffer);
    EXPECT_FALSE(tooSmall.has_value());
    EXPECT_EQ(tooSmall.error(), ProtocolErrors::OutputBufferTooSmall);
}

TEST(ProtocolDecoderTests, DecodeSupportsMinimumFrameWithEmptyPayload) {
    std::vector<uint8_t> payload{};
    auto raw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);
    ASSERT_EQ(raw.size(), Frame::Header_Size + Frame::CRC_Size);

    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
    const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
    std::vector<uint8_t> payloadBuffer{};
    const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value().payload.size(), 0U);
}

TEST(ProtocolDecoderTests, DecodeRejectsEverySingleByteRawMutationWithCrcMismatch) {
    auto payload = buildPayload(8);
    const auto cleanRaw = buildRawBuffer(kVersion, kKindValue, kFlags, kReserved, kSeq, kTypeValue, payload);

    for (size_t mutatedIndex{ 0 }; mutatedIndex < cleanRaw.size(); ++mutatedIndex) {
        auto raw = cleanRaw;
        raw[mutatedIndex] ^= 0x01;

        std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
        const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
        ASSERT_TRUE(encoded.has_value()) << "mutatedIndex=" << mutatedIndex;

        std::vector<uint8_t> rawBuffer(raw.size(), 0xEE);
        std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
        const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
        EXPECT_FALSE(decoded.has_value()) << "mutatedIndex=" << mutatedIndex;
        EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMisMatch) << "mutatedIndex=" << mutatedIndex;
    }
}
