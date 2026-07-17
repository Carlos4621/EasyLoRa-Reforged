#include "CrcCalculator.hpp"
#include <cstring>
#include <expected>
#include "BytesUtilities.hpp"
#include "CRC.h"
#include "ProtocolErrors/ProtocolErrors.hpp"

namespace protocol::detail {
    std::expected<std::span<std::byte>, ProtocolErrors> CrcCalculator::addCRC(std::span<const std::byte> inputBuffer, std::span<std::byte> outputBuffer) noexcept {
        if (inputBuffer.empty()) {
            return std::unexpected{ ProtocolErrors::InputBufferTooSmall };
        }

        if (minimumAddingOutputBufferSize(inputBuffer.size()) > outputBuffer.size()) {
            return std::unexpected{ ProtocolErrors::OutputBufferTooSmall };
        }

        std::memcpy(outputBuffer.data(), inputBuffer.data(), inputBuffer.size());

        const auto lastByte{ inputBuffer.size() };
        const auto CRC{ CRC::Calculate(inputBuffer.data(), inputBuffer.size(), CRC::CRC_16_CCITTFALSE()) };

        outputBuffer[lastByte] = getHighByte(CRC);
        outputBuffer[lastByte + 1] = getLowByte(CRC);

        return outputBuffer.first(lastByte + sizeof(CRC));
    }

    std::expected<std::span<std::byte>, ProtocolErrors> CrcCalculator::removeCRC(std::span<const std::byte> inputBuffer, std::span<std::byte> outputBuffer) noexcept {
        const auto minimumOutputSize{ minimumRemovingOutputBufferSize(inputBuffer.size()) };

        if (!minimumOutputSize.has_value()) {
            return std::unexpected{ minimumOutputSize.error() };
        }

        if (*minimumOutputSize > outputBuffer.size()) {
            return std::unexpected{ ProtocolErrors::OutputBufferTooSmall };
        }

        const auto CrcBytes{ inputBuffer.last(CRC_Lenght) };
        const auto actualCrc{ bindTwobytes(CrcBytes[0], CrcBytes[1]) };
        const auto expectedCrc{ CRC::Calculate(inputBuffer.data(), *minimumOutputSize, CRC::CRC_16_CCITTFALSE()) };

        if (actualCrc != expectedCrc) {
            return std::unexpected{ ProtocolErrors::CrcMismatch };
        }

        std::memcpy(outputBuffer.data(), inputBuffer.data(), *minimumOutputSize);

        return outputBuffer.first(*minimumOutputSize);
    }

    size_t CrcCalculator::minimumAddingOutputBufferSize(size_t inputBufferSize) noexcept {
        return inputBufferSize + CRC_Lenght;
    }

    std::expected<size_t, ProtocolErrors> CrcCalculator::minimumRemovingOutputBufferSize(size_t inputBufferSize) noexcept {
        if (inputBufferSize < CRC_Lenght) {
            return std::unexpected{ ProtocolErrors::InputBufferTooSmall };
        }

        return inputBufferSize - CRC_Lenght;
    }
}