#ifndef FRAME_RESPONDER_HEADER
#define FRAME_RESPONDER_HEADER

#include "Instances.hpp"
#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include "GetDeviceInfoHandler.hpp"

/// @brief Clase router para la contestación de Frames
class FrameResponder {
public:

    /// @brief Arma la respuesta necesaria para el Frame
    /// @param frame Frame a responder
    /// @param framePayloadBuffer Buffer en donde estará el payload del buffer de respuesta
    /// @param bytesWritten Bytes escrito en el payload
    /// @return std::expected con un Frame en caso de éxito, sino ProtocolErrors
    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> dispatch(const Frame& frame, std::span<uint8_t> framePayloadBuffer, size_t& bytesWritten) noexcept;

};

#endif // !FRAME_RESPONDER_HEADER