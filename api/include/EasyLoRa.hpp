#ifndef EASY_LORA_HEADER
#define EASY_LORA_HEADER

#include "boost/asio.hpp"
#include "ProtocolCodec.hpp"
#include <string_view>
#include "DeviceInfo_.pb.h"
#include <chrono>

class EasyLoRa {
public:
    EasyLoRa(boost::asio::io_context& context, std::string_view serialPath);

    /// @brief Determina si el dispositivo EasyLoRa está conectado al puerto dado
    /// @return true en caso de que el dispositivo esté conectado
    [[nodiscar]]
    bool isConnected() const noexcept;

    [[nodiscard]]
    std::expected<DeviceInfo_, ProtocolErrors> getConfiguration(std::chrono::milliseconds timeout = std::chrono::milliseconds{500});

private:

    boost::asio::io_context& context_m;
    boost::asio::serial_port port_m;

    uint16_t currentSeq_m{ 1 };

    [[nodiscard]]
    uint16_t nextSeq() noexcept;
};

#endif // !EASY_LORA_HEADER