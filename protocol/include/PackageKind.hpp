#ifndef PACKAGE_KIND_HEADER
#define PACKAGE_KIND_HEADER

#include <cstdint>

enum class PackageKind : uint8_t {
    Request = 1,
    Response,
    Event,
    Error
};

[[nodiscard]]
static constexpr bool isValidPackageKind(uint8_t value) {
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