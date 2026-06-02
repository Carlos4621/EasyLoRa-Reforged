#ifndef GET_DEVICE_INFO_HANDLE_HEADER
#define GET_DEVICE_INFO_HANDLE_HEADER

#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include <expected>
#include <cstdint>
#include <string>
#include "pb.h"
#include "DeviceInfo.pb.h"
#include "NanopbCallbacks.hpp"

class GetDeviceInfoHandle {
public:

    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> handle(const Frame& frame, std::span<uint8_t> framePayloadBuffer, size_t& bytesWritten) noexcept;

private:
    static constexpr std::string_view Firmware_Version{ "0.1" };
    static constexpr uint8_t Protocol_Version{ 1 };
    static constexpr std::string_view Device_Name{ "Easy LoRa" };
    static constexpr std::string_view Hardware_Revision{ "2" };

    [[nodiscard]]
    static DeviceInfo_DeviceInfo getDeviceInfo() noexcept;
};

#endif // !GET_DEVICE_INFO_HANDLE_HEADER