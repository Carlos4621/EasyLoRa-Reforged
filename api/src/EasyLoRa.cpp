#include "EasyLoRa.hpp"

EasyLoRa::EasyLoRa(boost::asio::io_context &context, std::string_view serialPath)
    : context_m{ context }
    , port_m{ context_m, serialPath.data() }
{
}

bool EasyLoRa::isConnected() const noexcept {
    return port_m.is_open();
}

std::expected<DeviceInfo_, ProtocolErrors> EasyLoRa::getConfiguration(std::chrono::milliseconds timeout) {
    Frame request {
        .version = Frame::Actual_Frame_Version,
        .kind = PackageKind::Request,
        .flags = 0,
        .reserved = 0,
        .seq = nextSeq(),
        .type = MessageType::GetDeviceInfo,
    };

    const auto startTime{ std::chrono::high_resolution_clock::now() };

    while ((std::chrono::high_resolution_clock::now() - startTime) < timeout) {
        boost::asio::async_read_until(port_m, )
    }

    return DeviceInfo_();
}

uint16_t EasyLoRa::nextSeq() noexcept {
    ++currentSeq_m;

    if (currentSeq_m == 0) {
        currentSeq_m = 1;
    }

    return currentSeq_m;
}
