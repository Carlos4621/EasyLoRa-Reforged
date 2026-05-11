#ifndef BITS_UTILITIES_HEADER
#define BITS_UTILITIES_HEADER

#include <cstdint>

static constexpr uint8_t Byte_Shift{ 8 };

[[nodiscard]]
static constexpr uint8_t getHighByte(uint16_t twoBytes) {
    return twoBytes >> Byte_Shift;
}

[[nodiscard]]
static constexpr uint8_t getLowByte(uint16_t twoBytes) {
    return static_cast<uint8_t>(twoBytes); // Truncar el primer byte
}

[[nodiscard]]
static constexpr uint16_t bindTwoBytes(uint8_t highByte, uint8_t lowByte) {
    return (highByte << Byte_Shift) | lowByte;
}

#endif // !BITS_UTILITIES_HEADER