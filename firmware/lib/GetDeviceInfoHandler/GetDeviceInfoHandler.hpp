#ifndef GET_DEVICE_INFO_HANDLE_HEADER
#define GET_DEVICE_INFO_HANDLE_HEADER

#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include <expected>
#include <cstdint>
#include <string>
#include <cstring>
#include <utility>
#include "pb.h"
#include "pb_encode.h"
#include "DeviceInfo_.pb.h"

/// @brief Handler que arma respuestas para Frames con MessageType::GetDeviceInfo
class GetDeviceInfoHandle {
public:

    /// @brief Arma la respuesta del Frame
    /// @param frame Frame a responder
    /// @param framePayloadBuffer Buffer donde se escribirá el payload del frame de respuesta
    /// @param bytesWritten Bytes escritos en el payload
    /// @return std::expected con Frame en caso de éxito, sino ProtocolErrors
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> handle(const Frame& frame, std::span<uint8_t> framePayloadBuffer, size_t& bytesWritten) noexcept;

private:
    static constexpr std::string_view Firmware_Version{ "0.1" };
    static constexpr uint8_t Protocol_Version{ 1 };
    static constexpr std::string_view Device_Name{ "Easy LoRa" };
    static constexpr std::string_view Hardware_Revision{ "2" };

    [[nodiscard]]
    static DeviceInfo_ getDeviceInfo() noexcept;

    static std::expected<Frame, ProtocolErrors> handleRequest(const Frame& frame, std::span<uint8_t> framePayloadBuffer, size_t& bytesWritten) noexcept;
};

#endif // !GET_DEVICE_INFO_HANDLE_HEADER