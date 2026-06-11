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

std::vector<uint8_t> buildPatternPayload(size_t size, uint32_t seed, bool forceManyZeros) {
    std::vector<uint8_t> payload(size, 0x00);
    uint32_t state{ seed };

    for (size_t i{ 0 }; i < size; ++i) {
        state = (state * 1103515245U) + 12345U;
        auto value{ static_cast<uint8_t>((state >> 16U) & 0xFFU) };
        if (forceManyZeros && ((i % 3U) == 0U)) {
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
    EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMismatch);
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
        EXPECT_EQ(decoded.error(), ProtocolErrors::FrameVersionMismatch);
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
        EXPECT_EQ(decoded.error(), ProtocolErrors::CRCMismatch) << "mutatedIndex=" << mutatedIndex;
    }
}

TEST(CobsrCodecDecoderTests, DeterministicFuzzRoundTripsShortLongAndZeroHeavyInputs) {
    const std::array<size_t, 12> inputSizes{
        0U,
        1U,
        2U,
        3U,
        16U,
        31U,
        32U,
        63U,
        64U,
        127U,
        255U,
        FrameCodec::Max_Frame_Payload_Size
    };

    for (const auto inputSize : inputSizes) {
        for (const bool zeroHeavy : std::array<bool, 2>{ false, true }) {
            const auto raw = buildPatternPayload(inputSize, static_cast<uint32_t>(0xC0FFEEU + inputSize), zeroHeavy);
            std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

            const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
            ASSERT_TRUE(encoded.has_value()) << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;
            if (inputSize == 0U) {
                EXPECT_EQ(encoded.value().size(), 0U) << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;
                continue;
            }

            EXPECT_TRUE(std::none_of(encoded.value().begin(), encoded.value().end(), [](uint8_t value) {
                return value == 0x00;
            })) << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;

            std::array<uint8_t, 2> prefix{ 0xA5, 0x5A };
            std::array<uint8_t, 2> suffix{ 0xC3, 0x3C };
            std::vector<uint8_t> guardedDecodedBuffer(encoded.value().size() + prefix.size() + suffix.size(), 0xEE);
            std::copy(prefix.begin(), prefix.end(), guardedDecodedBuffer.begin());
            std::copy(suffix.begin(), suffix.end(), guardedDecodedBuffer.end() - static_cast<std::ptrdiff_t>(suffix.size()));

            auto outputSpan = std::span<uint8_t>(guardedDecodedBuffer.data() + prefix.size(), encoded.value().size());
            const auto decoded = CobsrDecoder::decode(encoded.value(), outputSpan);
            ASSERT_TRUE(decoded.has_value()) << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;

            EXPECT_EQ(decoded.value().size(), raw.size()) << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;
            EXPECT_TRUE(std::equal(raw.begin(), raw.end(), decoded.value().begin()))
                << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;
            EXPECT_TRUE(std::equal(prefix.begin(), prefix.end(), guardedDecodedBuffer.begin()))
                << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;
            EXPECT_TRUE(std::equal(suffix.begin(), suffix.end(), guardedDecodedBuffer.end() - static_cast<std::ptrdiff_t>(suffix.size())))
                << "inputSize=" << inputSize << " zeroHeavy=" << zeroHeavy;
        }
    }
}

TEST(ProtocolCodecDecoderTests, DeterministicFuzzRoundTripsFramePayloadsAndKeepsGuardBytes) {
    const std::array<size_t, 10> payloadSizes{
        0U,
        1U,
        2U,
        7U,
        32U,
        64U,
        128U,
        255U,
        256U,
        FrameCodec::Max_Frame_Payload_Size
    };

    for (const auto payloadSize : payloadSizes) {
        auto payload = buildPatternPayload(payloadSize, static_cast<uint32_t>(0xFACEU + payloadSize), true);
        const auto frame = makeFrame(payload);
        const size_t rawSize{ ProtocolCodec::minimumFrameBytesBufferSize(frame) };
        const size_t encodedSize{ ProtocolCodec::minimumOutputBufferSize(frame) };

        std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
        std::vector<uint8_t> encodedBuffer(encodedSize, 0xEE);
        const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
        ASSERT_TRUE(encoded.has_value()) << "payloadSize=" << payloadSize;

        std::array<uint8_t, 2> rawPrefix{ 0x11, 0x22 };
        std::array<uint8_t, 2> rawSuffix{ 0x33, 0x44 };
        std::array<uint8_t, 2> payloadPrefix{ 0x55, 0x66 };
        std::array<uint8_t, 2> payloadSuffix{ 0x77, 0x88 };
        std::vector<uint8_t> guardedRawBuffer(rawSize + rawPrefix.size() + rawSuffix.size(), 0xEE);
        std::vector<uint8_t> guardedPayloadBuffer(payload.size() + payloadPrefix.size() + payloadSuffix.size(), 0xEE);

        std::copy(rawPrefix.begin(), rawPrefix.end(), guardedRawBuffer.begin());
        std::copy(rawSuffix.begin(), rawSuffix.end(), guardedRawBuffer.end() - static_cast<std::ptrdiff_t>(rawSuffix.size()));
        std::copy(payloadPrefix.begin(), payloadPrefix.end(), guardedPayloadBuffer.begin());
        std::copy(payloadSuffix.begin(), payloadSuffix.end(), guardedPayloadBuffer.end() - static_cast<std::ptrdiff_t>(payloadSuffix.size()));

        auto decodedRawSpan = std::span<uint8_t>(guardedRawBuffer.data() + rawPrefix.size(), rawSize);
        auto decodedPayloadSpan = std::span<uint8_t>(guardedPayloadBuffer.data() + payloadPrefix.size(), payload.size());
        const auto decoded = ProtocolDecoder::decode(encoded.value(), decodedRawSpan, decodedPayloadSpan);
        ASSERT_TRUE(decoded.has_value()) << "payloadSize=" << payloadSize;

        expectFrameMatches(decoded.value(), payload);
        EXPECT_TRUE(std::equal(rawPrefix.begin(), rawPrefix.end(), guardedRawBuffer.begin())) << "payloadSize=" << payloadSize;
        EXPECT_TRUE(std::equal(rawSuffix.begin(), rawSuffix.end(), guardedRawBuffer.end() - static_cast<std::ptrdiff_t>(rawSuffix.size())))
            << "payloadSize=" << payloadSize;
        EXPECT_TRUE(std::equal(payloadPrefix.begin(), payloadPrefix.end(), guardedPayloadBuffer.begin())) << "payloadSize=" << payloadSize;
        EXPECT_TRUE(std::equal(payloadSuffix.begin(), payloadSuffix.end(), guardedPayloadBuffer.end() - static_cast<std::ptrdiff_t>(payloadSuffix.size())))
            << "payloadSize=" << payloadSize;
    }
}

TEST(ProtocolDecoderTests, DecodeRejectsTruncatedEncodedFrames) {
    auto payload = buildPatternPayload(64, 0x1234U, true);
    const auto frame = makeFrame(payload);
    const size_t rawSize{ ProtocolCodec::minimumFrameBytesBufferSize(frame) };
    std::vector<uint8_t> rawBuffer(rawSize, 0xEE);
    std::vector<uint8_t> encodedBuffer(ProtocolCodec::minimumOutputBufferSize(frame), 0xEE);
    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    for (size_t truncatedSize{ 1 }; truncatedSize < encoded.value().size(); ++truncatedSize) {
        const auto truncatedInput = encoded.value().first(truncatedSize);
        std::vector<uint8_t> decodedRawBuffer(rawSize, 0xEE);
        std::vector<uint8_t> decodedPayloadBuffer(payload.size(), 0xEE);

        const auto decoded = ProtocolDecoder::decode(truncatedInput, decodedRawBuffer, decodedPayloadBuffer);
        EXPECT_FALSE(decoded.has_value()) << "truncatedSize=" << truncatedSize;
    }
}

TEST(ProtocolDecoderTests, DecodeRejectsSemanticMutationsWithRecomputedCrc) {
    auto payload = buildPayload(8);

    struct MutationCase {
        uint8_t version;
        uint8_t kind;
        uint8_t type;
        size_t payloadSize;
        ProtocolErrors expectedError;
    };

    const std::array<MutationCase, 4> mutationCases{ {
        { 0x02, kKindValue, kTypeValue, payload.size(), ProtocolErrors::FrameVersionMismatch },
        { kVersion, 0xFF, kTypeValue, payload.size(), ProtocolErrors::InvalidPackageKind },
        { kVersion, kKindValue, 0x99, payload.size(), ProtocolErrors::InvalidMessageType },
        { kVersion, kKindValue, kTypeValue, FrameCodec::Max_Frame_Payload_Size + 1U, ProtocolErrors::InputBufferTooLong },
    } };

    for (const auto& mutationCase : mutationCases) {
        auto mutatedPayload = buildPayload(mutationCase.payloadSize);
        if (mutationCase.payloadSize == payload.size()) {
            mutatedPayload = payload;
        }

        auto raw = buildRawBuffer(mutationCase.version,
                                  mutationCase.kind,
                                  kFlags,
                                  kReserved,
                                  kSeq,
                                  mutationCase.type,
                                  mutatedPayload);
        std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
        const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
        ASSERT_TRUE(encoded.has_value());

        std::vector<uint8_t> rawBuffer(CobsrDecoder::minimumOutputBufferSize(encoded.value().size()), 0xEE);
        std::vector<uint8_t> payloadBuffer(mutatedPayload.size(), 0xEE);
        const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);

        ASSERT_FALSE(decoded.has_value()) << "version=" << static_cast<int>(mutationCase.version)
                                          << " kind=" << static_cast<int>(mutationCase.kind)
                                          << " type=" << static_cast<int>(mutationCase.type)
                                          << " payloadSize=" << mutationCase.payloadSize;
        EXPECT_EQ(decoded.error(), mutationCase.expectedError);
    }
}

TEST(ProtocolDecoderTests, DecodeAcceptsEveryValidKindTypePairWithRecomputedCrc) {
    const std::array<uint8_t, 4> validKinds{
        static_cast<uint8_t>(PackageKind::Request),
        static_cast<uint8_t>(PackageKind::Response),
        static_cast<uint8_t>(PackageKind::Event),
        static_cast<uint8_t>(PackageKind::Error)
    };
    const std::array<uint8_t, 6> validTypes{
        static_cast<uint8_t>(MessageType::GetDeviceInfo),
        static_cast<uint8_t>(MessageType::GetConfiguration),
        static_cast<uint8_t>(MessageType::SetConfiguration),
        static_cast<uint8_t>(MessageType::SendRadioPacket),
        static_cast<uint8_t>(MessageType::RadioPacketReceived),
        static_cast<uint8_t>(MessageType::GenericError)
    };
    auto payload = buildPayload(3);

    for (const auto kind : validKinds) {
        for (const auto type : validTypes) {
            auto raw = buildRawBuffer(kVersion, kind, kFlags, kReserved, kSeq, type, payload);
            std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);
            const auto encoded = encodeRawWithCobsr(raw, encodedBuffer);
            ASSERT_TRUE(encoded.has_value()) << "kind=" << static_cast<int>(kind) << " type=" << static_cast<int>(type);

            std::vector<uint8_t> rawBuffer(CobsrDecoder::minimumOutputBufferSize(encoded.value().size()), 0xEE);
            std::vector<uint8_t> payloadBuffer(payload.size(), 0xEE);
            const auto decoded = ProtocolDecoder::decode(encoded.value(), rawBuffer, payloadBuffer);
            ASSERT_TRUE(decoded.has_value()) << "kind=" << static_cast<int>(kind) << " type=" << static_cast<int>(type);
            EXPECT_EQ(decoded.value().kind, static_cast<PackageKind>(kind));
            EXPECT_EQ(decoded.value().type, static_cast<MessageType>(type));
            EXPECT_TRUE(std::equal(payload.begin(), payload.end(), decoded.value().payload.begin()));
        }
    }
}
