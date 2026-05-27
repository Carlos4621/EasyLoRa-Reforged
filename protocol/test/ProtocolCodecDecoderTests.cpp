#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "CobsrCodec.hpp"
#include "CobsrDecoder.hpp"
#include "ProtocolErrors.hpp"
#include "cobs/cobsr.h"

namespace {
std::vector<uint8_t> buildPayload(size_t size) {
    std::vector<uint8_t> payload(size, 0x00);

    for (size_t i = 0; i < size; ++i) {
        uint8_t value = static_cast<uint8_t>((i * 31U) & 0xFFU);
        if ((i % 7U) == 0U) {
            value = 0x00;
        }
        payload[i] = value;
    }

    return payload;
}
}

TEST(ProtocolCodecTests, EncodeReturnsErrorWhenBufferTooSmall) {
    const auto raw = buildPayload(32);
    const size_t requiredSize = COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size());
    std::vector<uint8_t> output(requiredSize - 1, 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, output);
    EXPECT_FALSE(encoded.has_value());
    EXPECT_EQ(encoded.error(), ProtocolErrors::BufferTooSmall);
}

TEST(ProtocolCodecTests, EncodeSupportsEmptyInput) {
    const std::vector<uint8_t> raw{};
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());
    EXPECT_EQ(encoded.value().size(), 1U);

    std::vector<uint8_t> decodedBuffer(encoded.value().size(), 0xEE);
    const auto decoded = CobsrDecoder::decode(encoded.value(), decodedBuffer);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value().size(), 0U);
}

TEST(ProtocolCodecTests, EncodeSupportsInPlaceWithOffset) {
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

TEST(ProtocolCodecTests, EncodeRejectsOverlappingBufferWithoutOffset) {
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

TEST(ProtocolCodecDecoderTests, EncodeDecodeRoundTrip) {
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

TEST(ProtocolDecoderTests, DecodeReturnsErrorWhenOutputBufferTooSmall) {
    const auto raw = buildPayload(48);
    std::vector<uint8_t> encodedBuffer(COBSR_ENCODE_DST_BUF_LEN_MAX(raw.size()), 0xEE);

    const auto encoded = CobsrCodec::addCOBSR(raw, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> decodedBuffer(encoded.value().size() - 1, 0xEE);
    const auto decoded = CobsrDecoder::decode(encoded.value(), decodedBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::BufferTooSmall);
}

TEST(ProtocolDecoderTests, DecodeReturnsErrorWhenInputBufferEmpty) {
    std::vector<uint8_t> encodedBuffer{};
    std::vector<uint8_t> decodedBuffer(1, 0xEE);

    const auto decoded = CobsrDecoder::decode(encodedBuffer, decodedBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::EmptyInputBuffer);
}

TEST(ProtocolDecoderTests, DecodeRejectsOverlappingBufferWithDifferentStart) {
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

TEST(ProtocolDecoderTests, DecodeRejectsZeroByteInInput) {
    std::vector<uint8_t> encodedBuffer{ 0x11, 0x00, 0x22 };
    std::vector<uint8_t> decodedBuffer(encodedBuffer.size(), 0xEE);

    const auto decoded = CobsrDecoder::decode(encodedBuffer, decodedBuffer);
    EXPECT_FALSE(decoded.has_value());
    EXPECT_EQ(decoded.error(), ProtocolErrors::COBSRError);
}

TEST(ProtocolDecoderTests, DecodeSupportsInPlaceBuffer) {
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
