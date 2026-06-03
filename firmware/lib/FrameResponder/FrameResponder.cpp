#include "FrameResponder.hpp"

std::expected<Frame, ProtocolErrors> FrameResponder::dispatch(const Frame &frame, std::span<uint8_t> framePayloadBuffer, size_t& bytesWritten) noexcept {
    bytesWritten = 0;

    switch (frame.type) {
        using enum MessageType;
        case GetDeviceInfo:
            GetDeviceInfoHandle::handle(frame, framePayloadBuffer, bytesWritten);
            break;
        
        default:
            std::unexpected{ ProtocolErrors::InvalidMessageType };
            break;
    }
    
    
    return Frame();
}
