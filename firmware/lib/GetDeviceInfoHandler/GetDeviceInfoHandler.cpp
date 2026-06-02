#include "GetDeviceInfoHandler.hpp"

std::expected<Frame,ProtocolErrors> GetDeviceInfoHandle::handle(const Frame & frame, std::span<uint8_t> framePayloadBuffer, size_t & bytesWritten) noexcept {
    switch (frame.kind) {
    using enum PackageKind;
    case Request:
        
        break;
    
    default:
        std::unexpected{ ProtocolErrors::IncoherentFrame };
        break;
    }
    


    return std::expected<Frame,ProtocolErrors>();
}

DeviceInfo_DeviceInfo GetDeviceInfoHandle::getDeviceInfo() noexcept {
    DeviceInfo_DeviceInfo deviceInfo = DeviceInfo_DeviceInfo_init_zero;

    deviceInfo.firmware_version.funcs.encode = &writeStringCallback;
    deviceInfo.device_name.funcs.encode = &writeStringCallback;
    deviceInfo.hardware_revision.funcs.encode = &writeStringCallback;

    deviceInfo.firmware_version.arg = const_cast<void*>(static_cast<const void*>(Firmware_Version.data()));
    deviceInfo.device_name.arg = const_cast<void*>(static_cast<const void*>(Device_Name.data()));
    deviceInfo.hardware_revision.arg = const_cast<void*>(static_cast<const void*>(Hardware_Revision.data()));

    deviceInfo.protocol_version = Protocol_Version;

    return deviceInfo;
}
