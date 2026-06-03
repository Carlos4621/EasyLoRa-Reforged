#include "GetDeviceInfoHandler.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include "pb_decode.h"

namespace {
Frame MakeFrame(PackageKind kind, MessageType type, uint16_t seq = 0x1234) {
    return Frame{
        .version = Frame::Actual_Frame_Version,
        .kind = kind,
        .flags = 0,
        .reserved = 0,
        .seq = seq,
        .type = type,
        .payload = std::span<const uint8_t>{}
    };
}
}

TEST(GetDeviceInfoHandlerTests, RejectsWrongMessageType) {
    std::array<uint8_t, DeviceInfo_size> buffer{};
    size_t bytes_written = 77;

    const Frame frame = MakeFrame(PackageKind::Request, MessageType::GetConfiguration);
    auto result = GetDeviceInfoHandle::handle(frame, buffer, bytes_written);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::HandlerWithIncorrectType);
    EXPECT_EQ(bytes_written, 77u);
}

TEST(GetDeviceInfoHandlerTests, RejectsNonRequestKind) {
    std::array<uint8_t, DeviceInfo_size> buffer{};

    for (PackageKind kind : {PackageKind::Response, PackageKind::Event, PackageKind::Error}) {
        size_t bytes_written = 33;
        const Frame frame = MakeFrame(kind, MessageType::GetDeviceInfo);
        auto result = GetDeviceInfoHandle::handle(frame, buffer, bytes_written);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), ProtocolErrors::IncoherentFrame);
        EXPECT_EQ(bytes_written, 33u);
    }
}

TEST(GetDeviceInfoHandlerTests, RejectsTooSmallBuffer) {
    std::array<uint8_t, DeviceInfo_size - 1> buffer{};
    size_t bytes_written = 19;

    const Frame frame = MakeFrame(PackageKind::Request, MessageType::GetDeviceInfo);
    auto result = GetDeviceInfoHandle::handle(frame, buffer, bytes_written);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::BufferTooSmall);
    EXPECT_EQ(bytes_written, 19u);
}

TEST(GetDeviceInfoHandlerTests, BuildsResponseWithEncodedPayload) {
    std::array<uint8_t, DeviceInfo_size> buffer{};
    size_t bytes_written = 0;

    const uint16_t seq = 0xBEEF;
    const Frame frame = MakeFrame(PackageKind::Request, MessageType::GetDeviceInfo, seq);
    auto result = GetDeviceInfoHandle::handle(frame, buffer, bytes_written);

    ASSERT_TRUE(result.has_value());
    const Frame response = result.value();

    EXPECT_EQ(response.version, Frame::Actual_Frame_Version);
    EXPECT_EQ(response.kind, PackageKind::Response);
    EXPECT_EQ(response.flags, 0u);
    EXPECT_EQ(response.reserved, 0u);
    EXPECT_EQ(response.seq, seq);
    EXPECT_EQ(response.type, MessageType::GetDeviceInfo);

    ASSERT_GT(bytes_written, 0u);
    ASSERT_LE(bytes_written, buffer.size());
    EXPECT_EQ(response.payload.size(), bytes_written);
    EXPECT_EQ(response.payload.data(), buffer.data());

    DeviceInfo decoded = DeviceInfo_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(response.payload.data(), response.payload.size());
    ASSERT_TRUE(pb_decode(&stream, &DeviceInfo_msg, &decoded));

    EXPECT_STREQ(decoded.firmware_version, "0.1");
    EXPECT_EQ(decoded.protocol_version, 1u);
    EXPECT_STREQ(decoded.device_name, "Easy LoRa");
    EXPECT_STREQ(decoded.hardware_revision, "2");
}
