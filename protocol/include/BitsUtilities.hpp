#ifndef BITS_UTILITIES_HEADER
#define BITS_UTILITIES_HEADER

#include <cstdint>

static constexpr uint8_t Byte_Shift{ 8 };

/// @brief Obtiene el byte más significativo en Big Endian
/// @param twoBytes Dos bytes a separar
/// @return El byte más significativo
[[nodiscard]]
static constexpr uint8_t getHighByte(uint16_t twoBytes) noexcept {
    return twoBytes >> Byte_Shift;
}

/// @brief Obtiene el byte menos significativo en Big Endian
/// @param twoBytes Dos bytes a separar
/// @return El byte menos significativo
[[nodiscard]]
static constexpr uint8_t getLowByte(uint16_t twoBytes) noexcept {
    return static_cast<uint8_t>(twoBytes); // Truncar el primer byte
}

/// @brief Junta dos bytes en Big Endian
/// @param highByte Byte más significativo
/// @param lowByte Byte menos significativo
/// @return Dos bytes unidos
[[nodiscard]]
static constexpr uint16_t bindTwoBytes(uint8_t highByte, uint8_t lowByte) noexcept {
    return (highByte << Byte_Shift) | lowByte;
}

#endif // !BITS_UTILITIES_HEADER