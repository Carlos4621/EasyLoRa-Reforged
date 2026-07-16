#include "GetDeviceInfoHandler.hpp"

std::expected<Frame, ProtocolErrors> GetDeviceInfoHandle::handle(const Frame & frame, std::span<uint8_t> framePayloadBuffer, size_t & bytesWritten) noexcept {
    if (frame.type != MessageType::GetDeviceInfo) {
        return std::unexpected{ ProtocolErrors::HandlerWithIncorrectType };
    }
    
    switch (frame.kind) {
    using enum PackageKind;
    case Request:
        return handleRequest(frame, framePayloadBuffer, bytesWritten);
    
    default:
        return std::unexpected{ ProtocolErrors::IncoherentFrame };
    }

    std::unreachable();
}

DeviceInfo_ GetDeviceInfoHandle::getDeviceInfo() noexcept {
    static_assert(sizeof(DeviceInfo_::device_name) >= Device_Name.size(), "Tamaño de Device_Name demasiado grande");
    static_assert(sizeof(DeviceInfo_::firmware_version) >= Firmware_Version.size(), "Tamaño de Firmware_Version demasiado grande");
    static_assert(sizeof(DeviceInfo_::hardware_revision) >= Hardware_Revision.size(), "Tamaño de Hardware_Revision demasiado grande");

    DeviceInfo_ deviceInfo = DeviceInfo__init_zero;

    std::memcpy(deviceInfo.device_name, Device_Name.data(), Device_Name.size());
    std::memcpy(deviceInfo.firmware_version, Firmware_Version.data(), Firmware_Version.size());
    std::memcpy(deviceInfo.hardware_revision, Hardware_Revision.data(), Hardware_Revision.size());

    deviceInfo.protocol_version = Protocol_Version;

    return deviceInfo;
}

std::expected<Frame, ProtocolErrors> GetDeviceInfoHandle::handleRequest(const Frame &frame, std::span<uint8_t> framePayloadBuffer, size_t &bytesWritten) noexcept {
    if (framePayloadBuffer.size() < DeviceInfo__size) {
        return std::unexpected{ ProtocolErrors::OutputBufferTooSmall };
    }

    Frame response{
        .version = Frame::Actual_Frame_Version,
        .kind = PackageKind::Response,
        .flags = 0,
        .reserved = 0,
        .seq = frame.seq,
        .type = MessageType::GetDeviceInfo
    };

    pb_ostream_t stream = pb_ostream_from_buffer(&framePayloadBuffer[0], framePayloadBuffer.size());
    
    const auto info{ getDeviceInfo() };

    if (!pb_encode(&stream, &DeviceInfo__msg, &info)) {
        return std::unexpected{ ProtocolErrors::CodificationError };
    }

    // TODO: Añadir log de error mostrando stream.errmsg

    bytesWritten = stream.bytes_written;

    response.payload = framePayloadBuffer.first(bytesWritten);

    return response;
}
