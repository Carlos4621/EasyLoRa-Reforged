#ifndef FRAME_HEADER
#define FRAME_HEADER

#include <cstddef>
#include <cstdint>
#include <array>

namespace protocol {

    /**
     * @brief Header con el que inciará cada paquete
     * 
    */
    class Header {
    public:
        std::byte version;
        std::byte flags;
        uint16_t payloadLenght;

        static constexpr uint8_t Header_Size {
            sizeof(version) +
            sizeof(flags) +
            sizeof(payloadLenght)
        };

        [[nodiscard]]
        std::array<std::byte, Header_Size> toBytes() const noexcept;

    private:
        enum class HeaderIndex : uint8_t{
            Version = 0,
            Flags,
            PayloadLenght_High,
            PayloadLenght_Low,
        };
    };

} // namespace protocol

#endif // FRAME_HEADER