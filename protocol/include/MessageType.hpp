#ifndef MESSAGE_TYPE_HEADER
#define MESSAGE_TYPE_HEADER

#include <cstdint>

/// @brief Tipo de mensaje que comunica el Frame
enum class MessageType : uint8_t {
    GetDeviceInfo = 0,
    GetConfiguration,
    SetConfiguration,
    SendRadioPacket,
    RadioPacketReceived,
    GenericError,
};

/// @brief Valida si el valor enviado corresponde correctamente a un miembro de MessageType
/// @param value Valor a validar
/// @return Si el valor corresponde a un miembro de MessageType
[[nodiscard]]
static constexpr bool isValidMessageType(uint8_t value) noexcept {
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