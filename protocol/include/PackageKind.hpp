#ifndef PACKAGE_KIND_HEADER
#define PACKAGE_KIND_HEADER

#include <cstdint>

/// @brief Motivo por el que se envía o recibe el Frame
enum class PackageKind : uint8_t {
    Request = 0,
    Response,
    Event,
    Error
};

/// @brief Determina si un valor corresponde a un miembro de PackageKind
/// @param value Valor a evaluar
/// @return True en caso de ser un miembro de PackageKind
[[nodiscard]]
inline constexpr bool isValidPackageKind(uint8_t value) noexcept {
    const auto castedValue{ static_cast<PackageKind>(value) };

    switch (castedValue) {
    using enum PackageKind;
    case Request:
    case Response:
    case Event:
    case Error:
        return true;
    
    default:
        return false;
    }
}

#endif // !PACKAGE_KIND_HEADER