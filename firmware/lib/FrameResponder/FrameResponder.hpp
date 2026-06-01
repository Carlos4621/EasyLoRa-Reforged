#ifndef FRAME_RESPONDER_HEADER
#define FRAME_RESPONDER_HEADER

#include "Instances.hpp"
#include "Frame.hpp"
#include "ProtocolErrors.hpp"

class FrameResponder {
public:

    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> dispatch(const Frame& frame, std::span<uint8_t> framePayloadBuffer, size_t& bytesWritten) noexcept;

};

#endif // !FRAME_RESPONDER_HEADER