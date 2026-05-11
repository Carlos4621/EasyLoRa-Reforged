#ifndef PACKAGE_KIND_HEADER
#define PACKAGE_KIND_HEADER

#include <cstdint>

enum class PackageKind : uint8_t {
    Request = 1,
    Response,
    Event,
    Error
};

#endif // !PACKAGE_KIND_HEADER