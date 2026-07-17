#include "CrcCalculator.hpp"

#include "ProtocolErrors/ProtocolErrors.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <span>
#include <vector>

namespace {

using protocol::ProtocolErrors;
using protocol::detail::CrcCalculator;

constexpr std::size_t crcLength{2};
constexpr std::byte sentinel{0xA5};

[[nodiscard]] std::uint16_t calculateCrc16CcittFalse(
    std::span<const std::byte> input) noexcept
{
    constexpr std::uint16_t polynomial{0x1021};
    std::uint16_t crc{0xFFFF};

    for (const std::byte value : input) {
        crc ^= static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(value))
               << 8U;

        for (std::uint8_t bit = 0; bit < 8; ++bit) {
            const bool highBitSet = (crc & 0x8000U) != 0U;
            crc = static_cast<std::uint16_t>(crc << 1U);
            if (highBitSet) {
                crc ^= polynomial;
            }
        }
    }

    return crc;
}

[[nodiscard]] std::vector<std::byte> makePattern(std::size_t size)
{
    std::vector<std::byte> input(size);
    for (std::size_t index = 0; index < size; ++index) {
        input[index] =
            static_cast<std::byte>(((index * 37U) + (size * 11U)) & 0xFFU);
    }
    return input;
}

[[nodiscard]] std::vector<std::byte> makeFrame(
    std::span<const std::byte> payload)
{
    std::vector<std::byte> frame(payload.begin(), payload.end());
    const std::uint16_t crc = calculateCrc16CcittFalse(payload);
    frame.push_back(static_cast<std::byte>(crc >> 8U));
    frame.push_back(static_cast<std::byte>(crc & 0xFFU));
    return frame;
}

void expectSuccessfulRemoval(std::span<const std::byte> payload,
                             std::size_t outputSize)
{
    const std::vector<std::byte> frame = makeFrame(payload);
    std::vector<std::byte> output(outputSize, sentinel);

    const auto result = CrcCalculator::removeCRC(frame, output);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->data(), output.data());
    ASSERT_EQ(result->size(), payload.size());
    EXPECT_TRUE(std::equal(payload.begin(), payload.end(), result->begin()));
    EXPECT_TRUE(std::all_of(
        output.begin() + static_cast<std::ptrdiff_t>(payload.size()),
        output.end(),
        [](std::byte value) { return value == sentinel; }));
}

[[noreturn]] void checkInsufficientOutputInChild(
    std::span<const std::byte> input,
    std::size_t outputSize)
{
    std::vector<std::byte> backing(input.size() + crcLength, sentinel);
    const std::span<std::byte> output(backing.data(), outputSize);

    const auto result = CrcCalculator::addCRC(input, output);
    const bool correctError =
        !result.has_value() &&
        result.error() == ProtocolErrors::OutputBufferTooSmall;
    const bool unchanged =
        std::all_of(backing.begin(), backing.end(), [](std::byte value) {
            return value == sentinel;
        });

    std::_Exit(correctError && unchanged ? EXIT_SUCCESS : EXIT_FAILURE);
}

void expectSuccessfulEncoding(std::span<const std::byte> input,
                              std::size_t outputSize)
{
    const std::size_t encodedSize = input.size() + crcLength;
    std::vector<std::byte> output(outputSize, sentinel);

    const auto result = CrcCalculator::addCRC(input, output);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data(), output.data());
    ASSERT_EQ(result->size(), encodedSize);
    EXPECT_TRUE(std::equal(input.begin(), input.end(), result->begin()));

    const std::uint16_t expectedCrc = calculateCrc16CcittFalse(input);
    EXPECT_EQ((*result)[input.size()],
              static_cast<std::byte>(expectedCrc >> 8U));
    EXPECT_EQ((*result)[input.size() + 1U],
              static_cast<std::byte>(expectedCrc & 0xFFU));

    EXPECT_TRUE(std::all_of(output.begin() +
                                static_cast<std::ptrdiff_t>(encodedSize),
                            output.end(),
                            [](std::byte value) { return value == sentinel; }));
}

TEST(CRCAdderMinimumOutputBufferSize, AddsTheTwoCrcBytes)
{
    constexpr std::array<std::size_t, 8> inputSizes{
        0U, 1U, 2U, 15U, 255U, 256U, 4096U,
        std::numeric_limits<std::size_t>::max() - crcLength,
    };

    for (const std::size_t inputSize : inputSizes) {
        EXPECT_EQ(CrcCalculator::minimumAddingOutputBufferSize(inputSize),
                  inputSize + crcLength)
            << "input size: " << inputSize;
    }
}

TEST(CRCAdderAddCRC, RejectsEmptyInputWithoutModifyingOutput)
{
    std::array<std::byte, 4> output{};
    output.fill(sentinel);

    const auto result =
        CrcCalculator::addCRC(std::span<const std::byte>{}, output);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::InputBufferTooSmall);
    EXPECT_TRUE(
        std::ranges::all_of(output, [](std::byte value) {
            return value == sentinel;
        }));
}

TEST(CRCAdderAddCRC, RejectsEveryInsufficientOutputSize)
{
    const std::array input{
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0x40},
    };
    constexpr std::size_t requiredSize = input.size() + crcLength;

    for (std::size_t outputSize = 0; outputSize < requiredSize; ++outputSize) {
        // Isolate the call because a defective bounds check can trigger a
        // checked-std::span assertion before GoogleTest regains control.
        EXPECT_EXIT(
            { checkInsufficientOutputInChild(input, outputSize); },
            ::testing::ExitedWithCode(EXIT_SUCCESS),
            "")
            << "output size: " << outputSize;
    }
}

TEST(CRCAdderAddCRC, AcceptsAnExactlySizedOutputBuffer)
{
    const std::array input{
        std::byte{0x00},
        std::byte{0x7F},
        std::byte{0x80},
        std::byte{0xFF},
    };

    expectSuccessfulEncoding(input, input.size() + crcLength);
}

TEST(CRCAdderAddCRC, AcceptsAnOversizedBufferAndReturnsOnlyEncodedBytes)
{
    const std::array input{
        std::byte{0xDE},
        std::byte{0xAD},
        std::byte{0xBE},
        std::byte{0xEF},
    };

    expectSuccessfulEncoding(input, input.size() + crcLength + 8U);
}

TEST(CRCAdderAddCRC, MatchesTheStandardCheckValueFor123456789)
{
    constexpr std::array input{
        std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
        std::byte{'4'}, std::byte{'5'}, std::byte{'6'},
        std::byte{'7'}, std::byte{'8'}, std::byte{'9'},
    };
    std::array<std::byte, input.size() + crcLength> output{};

    const auto result = CrcCalculator::addCRC(input, output);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), output.size());
    EXPECT_EQ((*result)[input.size()], std::byte{0x29});
    EXPECT_EQ((*result)[input.size() + 1U], std::byte{0xB1});
}

TEST(CRCAdderAddCRC, HandlesRepresentativePayloadSizesAndBytePatterns)
{
    constexpr std::array<std::size_t, 12> sizes{
        1U, 2U, 3U, 7U, 8U, 15U, 16U, 31U, 32U, 255U, 256U, 1024U,
    };

    for (const std::size_t size : sizes) {
        const std::vector<std::byte> input = makePattern(size);
        expectSuccessfulEncoding(input, size + crcLength);
    }
}

TEST(CRCAdderAddCRC, HandlesUniformPayloads)
{
    for (const std::byte value :
         {std::byte{0x00}, std::byte{0x55}, std::byte{0xAA},
          std::byte{0xFF}}) {
        const std::vector<std::byte> input(257U, value);
        expectSuccessfulEncoding(input, input.size() + crcLength);
    }
}

TEST(CrcCalculatorMinimumRemovingOutputBufferSize,
     RejectsInputsShorterThanTheCrc)
{
    for (std::size_t inputSize = 0; inputSize < crcLength; ++inputSize) {
        const auto result =
            CrcCalculator::minimumRemovingOutputBufferSize(inputSize);

        ASSERT_FALSE(result.has_value()) << "input size: " << inputSize;
        EXPECT_EQ(result.error(), ProtocolErrors::InputBufferTooSmall)
            << "input size: " << inputSize;
    }
}

TEST(CrcCalculatorMinimumRemovingOutputBufferSize,
     SubtractsTheTwoCrcBytes)
{
    constexpr std::array<std::size_t, 8> inputSizes{
        2U, 3U, 4U, 17U, 257U, 258U, 4098U,
        std::numeric_limits<std::size_t>::max(),
    };

    for (const std::size_t inputSize : inputSizes) {
        const auto result =
            CrcCalculator::minimumRemovingOutputBufferSize(inputSize);

        ASSERT_TRUE(result.has_value()) << "input size: " << inputSize;
        EXPECT_EQ(*result, inputSize - crcLength)
            << "input size: " << inputSize;
    }
}

TEST(CrcCalculatorRemoveCRC, RejectsInputsShorterThanTheCrc)
{
    constexpr std::array input{std::byte{0x42}};

    for (std::size_t inputSize = 0; inputSize < crcLength; ++inputSize) {
        std::array<std::byte, 4> output{};
        output.fill(sentinel);

        const auto result = CrcCalculator::removeCRC(
            std::span<const std::byte>{input.data(), inputSize}, output);

        ASSERT_FALSE(result.has_value()) << "input size: " << inputSize;
        EXPECT_EQ(result.error(), ProtocolErrors::InputBufferTooSmall)
            << "input size: " << inputSize;
        EXPECT_TRUE(std::ranges::all_of(output, [](std::byte value) {
            return value == sentinel;
        }));
    }
}

TEST(CrcCalculatorRemoveCRC, AcceptsAFrameWithAnEmptyPayload)
{
    expectSuccessfulRemoval(std::span<const std::byte>{}, 0U);
}

TEST(CrcCalculatorRemoveCRC, RejectsEveryInsufficientOutputSize)
{
    const std::vector<std::byte> payload = makePattern(8U);
    const std::vector<std::byte> frame = makeFrame(payload);

    for (std::size_t outputSize = 0; outputSize < payload.size();
         ++outputSize) {
        std::vector<std::byte> backing(payload.size(), sentinel);
        const std::span<std::byte> output(backing.data(), outputSize);

        const auto result = CrcCalculator::removeCRC(frame, output);

        ASSERT_FALSE(result.has_value()) << "output size: " << outputSize;
        EXPECT_EQ(result.error(), ProtocolErrors::OutputBufferTooSmall)
            << "output size: " << outputSize;
        EXPECT_TRUE(std::ranges::all_of(backing, [](std::byte value) {
            return value == sentinel;
        })) << "output size: " << outputSize;
    }
}

TEST(CrcCalculatorRemoveCRC, AcceptsAnExactlySizedOutputBuffer)
{
    const std::vector<std::byte> payload = makePattern(16U);
    expectSuccessfulRemoval(payload, payload.size());
}

TEST(CrcCalculatorRemoveCRC,
     AcceptsAnOversizedBufferAndReturnsOnlyThePayload)
{
    const std::vector<std::byte> payload = makePattern(31U);
    expectSuccessfulRemoval(payload, payload.size() + 9U);
}

TEST(CrcCalculatorRemoveCRC, MatchesTheStandard123456789Frame)
{
    constexpr std::array payload{
        std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
        std::byte{'4'}, std::byte{'5'}, std::byte{'6'},
        std::byte{'7'}, std::byte{'8'}, std::byte{'9'},
    };
    constexpr std::array frame{
        std::byte{'1'}, std::byte{'2'}, std::byte{'3'},
        std::byte{'4'}, std::byte{'5'}, std::byte{'6'},
        std::byte{'7'}, std::byte{'8'}, std::byte{'9'},
        std::byte{0x29}, std::byte{0xB1},
    };
    std::array<std::byte, payload.size()> output{};

    const auto result = CrcCalculator::removeCRC(frame, output);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::ranges::equal(*result, payload));
}

TEST(CrcCalculatorRemoveCRC, RejectsEverySingleBitCrcCorruption)
{
    const std::vector<std::byte> payload = makePattern(32U);
    const std::vector<std::byte> validFrame = makeFrame(payload);

    for (std::size_t crcByte = 0; crcByte < crcLength; ++crcByte) {
        for (std::uint8_t bit = 0; bit < 8U; ++bit) {
            std::vector<std::byte> corruptedFrame = validFrame;
            const std::size_t index = payload.size() + crcByte;
            corruptedFrame[index] ^= static_cast<std::byte>(1U << bit);
            std::vector<std::byte> output(payload.size(), sentinel);

            const auto result =
                CrcCalculator::removeCRC(corruptedFrame, output);

            ASSERT_FALSE(result.has_value())
                << "CRC byte: " << crcByte << ", bit: " << +bit;
            EXPECT_EQ(result.error(), ProtocolErrors::CrcMismatch);
            EXPECT_TRUE(std::ranges::all_of(output, [](std::byte value) {
                return value == sentinel;
            }));
        }
    }
}

TEST(CrcCalculatorRemoveCRC, RejectsPayloadCorruption)
{
    const std::vector<std::byte> payload = makePattern(32U);
    std::vector<std::byte> corruptedFrame = makeFrame(payload);
    corruptedFrame[17] ^= std::byte{0x04};
    std::vector<std::byte> output(payload.size(), sentinel);

    const auto result = CrcCalculator::removeCRC(corruptedFrame, output);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ProtocolErrors::CrcMismatch);
    EXPECT_TRUE(std::ranges::all_of(output, [](std::byte value) {
        return value == sentinel;
    }));
}

TEST(CrcCalculatorRemoveCRC, HandlesRepresentativePayloadSizesAndPatterns)
{
    constexpr std::array<std::size_t, 12> sizes{
        1U, 2U, 3U, 7U, 8U, 15U, 16U, 31U, 32U, 255U, 256U, 1024U,
    };

    for (const std::size_t size : sizes) {
        const std::vector<std::byte> payload = makePattern(size);
        expectSuccessfulRemoval(payload, payload.size());
    }
}

TEST(CrcCalculatorRemoveCRC, HandlesUniformPayloads)
{
    for (const std::byte value :
         {std::byte{0x00}, std::byte{0x55}, std::byte{0xAA},
          std::byte{0xFF}}) {
        const std::vector<std::byte> payload(257U, value);
        expectSuccessfulRemoval(payload, payload.size());
    }
}

} // namespace
