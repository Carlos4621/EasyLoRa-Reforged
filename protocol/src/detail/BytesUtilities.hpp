#ifndef BYTES_UTILITIES_HEADER
#define BYTES_UTILITIES_HEADER

#include <cstddef>
#include <cstdint>

namespace protocol::detail {
    static constexpr uint8_t Byte_Shift{ 8 };

    /**
     * @brief Obtiene el byte superior de dos bytes, en Big Endian
     * 
     * @param twoBytes Par de bytes a separar
     * @return std::byte con el byte superior
    */
    [[nodiscard]]
    inline std::byte getHighByte(uint16_t twoBytes) noexcept {
        return static_cast<std::byte>(twoBytes >> Byte_Shift);
    }

    /**
     * @brief Obtiene el byte inferior de dos bytes, en Big Endian
     * 
     * @param twoBytes Par de bytes a separar
     * @return std::byte con el byte inferior
    */
    [[nodiscard]]
    inline std::byte getLowByte(uint16_t twoBytes) noexcept {
        return static_cast<std::byte>(twoBytes);
    }

    /**
     * @brief Une dos variables de un byte a una sola de dos bytes en Big Endian
     * 
     * @param highByte Byte superior
     * @param lowByte Byte inferior
     * @return uint16_t con el par de bytes unidos
    */
    [[nodiscard]]
    inline uint16_t bindTwobytes(std::byte highByte, std::byte lowByte) noexcept {
        return static_cast<uint16_t>((static_cast<uint16_t>(highByte) << Byte_Shift) | static_cast<uint16_t>(lowByte));
    }
}

#endif // !BYTES_UTILITIES_HEADER