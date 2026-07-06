#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "DeviceInfo_.pb.h"
#include "Frame.hpp"
#include "ProtocolCodec.hpp"
#include "ProtocolDecoder.hpp"
#include "pb_decode.h"
#include "pb_encode.h"

namespace {
constexpr PackageKind kKind{ PackageKind::Response };
constexpr uint8_t kFlags{ 0x00 };
constexpr uint8_t kReserved{ 0x00 };
constexpr uint16_t kSeq{ 0xBEEF };
constexpr MessageType kType{ MessageType::GetDeviceInfo };

template <size_t Size>
void setString(char (&destination)[Size], const char* value) {
    std::fill(std::begin(destination), std::end(destination), '\0');
    std::strncpy(destination, value, Size - 1U);
}

DeviceInfo_ makeDeviceInfoAtStringLimits() {
    DeviceInfo_ info = DeviceInfo__init_zero;
    setString(info.firmware_version, "fw-123456789012");
    info.protocol_version = Frame::Actual_Frame_Version;
    setString(info.device_name, "device-name-1234567890123456789");
    setString(info.hardware_revision, "hw-123456789012");
    return info;
}

DeviceInfo_ makeDeviceInfoAtEncodedLimits() {
    auto info = makeDeviceInfoAtStringLimits();
    info.protocol_version = std::numeric_limits<uint32_t>::max();
    return info;
}

std::vector<uint8_t> encodeDeviceInfo(const DeviceInfo_& info) {
    std::vector<uint8_t> payload(DeviceInfo__size, 0x00);
    auto stream = pb_ostream_from_buffer(payload.data(), payload.size());

    if (!pb_encode(&stream, DeviceInfo__fields, &info)) {
        return {};
    }

    payload.resize(stream.bytes_written);
    return payload;
}

Frame makeFrame(std::span<const uint8_t> payload);

std::vector<uint8_t> encodeFramePayload(std::span<const uint8_t> payload) {
    const auto frame = makeFrame(payload);
    std::vector<uint8_t> rawBuffer(ProtocolCodec::minimumFrameBytesBufferSize(frame), 0xEE);
    std::vector<uint8_t> encodedBuffer(ProtocolCodec::minimumOutputBufferSize(frame), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    if (!encoded.has_value()) {
        return {};
    }

    return std::vector<uint8_t>(encoded.value().begin(), encoded.value().end());
}

std::optional<std::vector<uint8_t>> decodeFramePayload(std::span<const uint8_t> encodedFrame, size_t payloadSize) {
    std::vector<uint8_t> decodedRawBuffer(ProtocolDecoder::minimumFrameBytesBufferSize(encodedFrame.size()), 0xEE);
    std::vector<uint8_t> decodedPayload(payloadSize, 0xEE);

    const auto decodedFrame = ProtocolDecoder::decode({ encodedFrame, decodedRawBuffer, decodedPayload });
    if (!decodedFrame.has_value()) {
        return std::nullopt;
    }

    return std::vector<uint8_t>(decodedFrame.value().payload.begin(), decodedFrame.value().payload.end());
}

Frame makeFrame(std::span<const uint8_t> payload) {
    Frame frame{};
    frame.version = Frame::Actual_Frame_Version;
    frame.kind = kKind;
    frame.flags = kFlags;
    frame.reserved = kReserved;
    frame.seq = kSeq;
    frame.type = kType;
    frame.payload = payload;
    return frame;
}

void expectDeviceInfoEquals(const DeviceInfo_& actual, const DeviceInfo_& expected) {
    EXPECT_STREQ(actual.firmware_version, expected.firmware_version);
    EXPECT_EQ(actual.protocol_version, expected.protocol_version);
    EXPECT_STREQ(actual.device_name, expected.device_name);
    EXPECT_STREQ(actual.hardware_revision, expected.hardware_revision);
}
} // namespace

TEST(NanopbCompatibilityTests, DeviceInfoRoundTripsAsFramePayloadAtStringLimits) {
    const auto expectedInfo = makeDeviceInfoAtStringLimits();
    auto payload = encodeDeviceInfo(expectedInfo);
    ASSERT_FALSE(payload.empty());

    const auto frame = makeFrame(payload);
    std::vector<uint8_t> rawBuffer(ProtocolCodec::minimumFrameBytesBufferSize(frame), 0xEE);
    std::vector<uint8_t> encodedBuffer(ProtocolCodec::minimumOutputBufferSize(frame), 0xEE);

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, encodedBuffer);
    ASSERT_TRUE(encoded.has_value());

    std::vector<uint8_t> decodedRawBuffer(rawBuffer.size(), 0xEE);
    std::vector<uint8_t> decodedPayload(payload.size(), 0xEE);
    const auto decodedFrame = ProtocolDecoder::decode({encoded.value(), decodedRawBuffer, decodedPayload});
    ASSERT_TRUE(decodedFrame.has_value());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), decodedFrame.value().payload.begin()));

    DeviceInfo_ actualInfo = DeviceInfo__init_zero;
    auto stream = pb_istream_from_buffer(decodedFrame.value().payload.data(), decodedFrame.value().payload.size());
    ASSERT_TRUE(pb_decode(&stream, DeviceInfo__fields, &actualInfo));
    expectDeviceInfoEquals(actualInfo, expectedInfo);
}

TEST(NanopbCompatibilityTests, DeviceInfoRoundTripsMaximumValidPayload) {
    const auto expectedInfo = makeDeviceInfoAtEncodedLimits();
    auto payload = encodeDeviceInfo(expectedInfo);
    ASSERT_FALSE(payload.empty());
    ASSERT_LE(payload.size(), DeviceInfo__size);

    const auto encodedFrame = encodeFramePayload(payload);
    ASSERT_FALSE(encodedFrame.empty());

    const auto decodedPayload = decodeFramePayload(encodedFrame, payload.size());
    ASSERT_TRUE(decodedPayload.has_value());
    ASSERT_EQ(decodedPayload.value(), payload);

    DeviceInfo_ actualInfo = DeviceInfo__init_zero;
    auto stream = pb_istream_from_buffer(decodedPayload.value().data(), decodedPayload.value().size());
    ASSERT_TRUE(pb_decode(&stream, DeviceInfo__fields, &actualInfo));
    expectDeviceInfoEquals(actualInfo, expectedInfo);
}

TEST(NanopbCompatibilityTests, DeviceInfoDefaultsRoundTripFromEmptyPayload) {
    std::vector<uint8_t> payload{};

    const auto encodedFrame = encodeFramePayload(payload);
    ASSERT_FALSE(encodedFrame.empty());

    const auto decodedPayload = decodeFramePayload(encodedFrame, payload.size());
    ASSERT_TRUE(decodedPayload.has_value());
    ASSERT_TRUE(decodedPayload.value().empty());

    DeviceInfo_ actualInfo = DeviceInfo__init_zero;
    auto stream = pb_istream_from_buffer(decodedPayload.value().data(), decodedPayload.value().size());
    ASSERT_TRUE(pb_decode(&stream, DeviceInfo__fields, &actualInfo));
    EXPECT_STREQ(actualInfo.firmware_version, "");
    EXPECT_EQ(actualInfo.protocol_version, 0U);
    EXPECT_STREQ(actualInfo.device_name, "");
    EXPECT_STREQ(actualInfo.hardware_revision, "");
}

TEST(NanopbCompatibilityTests, DeviceInfoCorruptNanopbPayloadFailsAfterFrameDecodeSucceeds) {
    const std::array<uint8_t, 2> corruptPayload{ 0x0A, 0x01 };

    const auto encodedFrame = encodeFramePayload(corruptPayload);
    ASSERT_FALSE(encodedFrame.empty());

    const auto decodedPayload = decodeFramePayload(encodedFrame, corruptPayload.size());
    ASSERT_TRUE(decodedPayload.has_value());
    ASSERT_EQ(decodedPayload.value().size(), corruptPayload.size());
    ASSERT_TRUE(std::equal(corruptPayload.begin(), corruptPayload.end(), decodedPayload.value().begin()));

    DeviceInfo_ actualInfo = DeviceInfo__init_zero;
    auto stream = pb_istream_from_buffer(decodedPayload.value().data(), decodedPayload.value().size());
    EXPECT_FALSE(pb_decode(&stream, DeviceInfo__fields, &actualInfo));
}

TEST(NanopbCompatibilityTests, DeviceInfoRejectsStringsWithoutNullTerminator) {
    DeviceInfo_ info = DeviceInfo__init_zero;
    std::fill(std::begin(info.firmware_version), std::end(info.firmware_version), 'x');

    std::array<uint8_t, DeviceInfo__size> payload{};
    auto stream = pb_ostream_from_buffer(payload.data(), payload.size());

    EXPECT_FALSE(pb_encode(&stream, DeviceInfo__fields, &info));
}
