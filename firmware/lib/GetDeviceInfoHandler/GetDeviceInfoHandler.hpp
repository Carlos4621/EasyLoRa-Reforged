#ifndef GET_DEVICE_INFO_HANDLE_HEADER
#define GET_DEVICE_INFO_HANDLE_HEADER

#include "Frame.hpp"
#include "ProtocolErrors.hpp"
#include <expected>

class GetDeviceInfoHandle {
public:

    [[nodiscard]]
    static std::expected<Frame, ProtocolErrors> handle(const Frame& frame, ) noexcept;
};

#endif // !GET_DEVICE_INFO_HANDLE_HEADER