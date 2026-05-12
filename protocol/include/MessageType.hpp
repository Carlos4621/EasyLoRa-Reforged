#ifndef MESSAGE_TYPE_HEADER
#define MESSAGE_TYPE_HEADER

#include <cstdint>

enum class MessageType : uint8_t {
    GetDeviceInfo = 1,
    GetConfiguration,
    SetConfiguration,
    SendRadioPacket,
    RadioPacketReceived,
    GenericError,
};

[[nodiscard]]
static constexpr bool isValidMessageType(uint8_t value) {
    const auto castedValue{ static_cast<MessageType>(value) };

    switch (castedValue) {
    using enum MessageType;
    case GetDeviceInfo:
    case GetConfiguration:
    case SetConfiguration:
    case SendRadioPacket:
    case RadioPacketReceived:
    case GenericError:
        return true;
    
    default:
        return false;
    }
}

#endif // !MESSAGE_TYPE_HEADER