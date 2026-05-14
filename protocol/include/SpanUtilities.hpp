#ifndef SPAN_UTILITIES_HEADER
#define SPAN_UTILITIES_HEADER

#include <cstdint>
#include <span>

[[nodiscard]]
static inline bool spansOverlap(std::span<const uint8_t> left, std::span<const uint8_t> right) {
    if (left.empty() || right.empty()) {
        return false;
    }

    const auto leftBegin = reinterpret_cast<std::uintptr_t>(left.data());
    const auto leftEnd = leftBegin + left.size();
    const auto rightBegin = reinterpret_cast<std::uintptr_t>(right.data());
    const auto rightEnd = rightBegin + right.size();

    return (leftBegin < rightEnd) && (rightBegin < leftEnd);
}

#endif // SPAN_UTILITIES_HEADER
