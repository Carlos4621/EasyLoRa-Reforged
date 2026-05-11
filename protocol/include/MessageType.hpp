#ifndef MESSAGE_TYPE_HEADER
#define MESSAGE_TYPE_HEADER

#include <cstdint>

enum class MessageType : uint8_t {
    GetDeviceInfo = 1,
    GetConfiguration,
    SetConfiguration,
    SendRadioPacket,
    RadioPacketReceived,
    GenericError
};

#endif // !MESSAGE_TYPE_HEADER