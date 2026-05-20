#ifndef VALID_PINS_HEADER
#define VALID_PINS_HEADER

#include <array>
#include <algorithm>
#include "hardware/uart.h"

static constexpr uint8_t QFN80_Pins{ 48 };

static constexpr std::array RP2040_UART_0_Tx_Valid_Pins{ 0, 12, 16, 28 };
static constexpr std::array RP2040_UART_0_Rx_Valid_Pins{ 1, 13, 17, 29 };

static constexpr std::array RP2040_UART_1_Tx_Valid_Pins{ 4, 8, 20, 24 };
static constexpr std::array RP2040_UART_1_Rx_Valid_Pins{ 5, 9, 21, 25 };


static constexpr std::array RP2350_UART_0_Tx_Valid_Pins{ 0, 2, 12, 14, 16, 18, 28 };
static constexpr std::array RP2350_UART_0_Rx_Valid_Pins{ 1, 3, 13, 15, 17, 19, 29 };

static constexpr std::array RP2350_UART_1_Tx_Valid_Pins{ 4, 6, 8, 10, 20, 22, 24, 26 };
static constexpr std::array RP2350_UART_1_Rx_Valid_Pins{ 5, 7, 9, 11, 21, 23, 25, 27 };


static constexpr std::array RP2350_QFN80_UART_0_Tx_Valid_Pins{ 30, 32, 34, 44, 46 };
static constexpr std::array RP2350_QFN80_UART_0_Rx_Valid_Pins{ 31, 33, 35, 45, 47 };

static constexpr std::array RP2350_QFN80_UART_1_Tx_Valid_Pins{ 36, 38, 40, 42 };
static constexpr std::array RP2350_QFN80_UART_1_Rx_Valid_Pins{ 37, 39, 41, 43 };

template <typename T, size_t Size>
static constexpr bool containsPin(const std::array<T, Size>& pins, uint gpio) {
    return std::find(pins.cbegin(), pins.cend(), gpio) != pins.cend();
}

static constexpr bool isValidUART_Rx(uint uartNumber, uint gpio) {
    #if defined(PICO_RP2040)
        switch (uartNumber) {
            case 0:
                return containsPin(RP2040_UART_0_Rx_Valid_Pins, gpio);
            case 1:
                return containsPin(RP2040_UART_1_Rx_Valid_Pins, gpio);
            default:
                return false;
        }
    #elif defined(PICO_RP2350A)
        constexpr bool isQfn80{ NUM_BANK0_GPIOS == QFN80_Pins };
        switch (uartNumber) {
            case 0:
                return containsPin(RP2350_UART_0_Rx_Valid_Pins, gpio)
                    || (isQfn80 && containsPin(RP2350_QFN80_UART_0_Rx_Valid_Pins, gpio));
            case 1:
                return containsPin(RP2350_UART_1_Rx_Valid_Pins, gpio)
                    || (isQfn80 && containsPin(RP2350_QFN80_UART_1_Rx_Valid_Pins, gpio));
            default:
                return false;
        }
    #else
        (void)uartNumber;
        (void)gpio;
        return false;
    #endif
}

static constexpr bool isValidUART_Tx(uint uartNumber, uint gpio) {
    #if defined(PICO_RP2040)
        switch (uartNumber) {
            case 0:
                return containsPin(RP2040_UART_0_Tx_Valid_Pins, gpio);
            case 1:
                return containsPin(RP2040_UART_1_Tx_Valid_Pins, gpio);
            default:
                return false;
        }
    #elif defined(PICO_RP2350A)
        constexpr bool isQfn80{ NUM_BANK0_GPIOS == QFN80_Pins };
        switch (uartNumber) {
            case 0:
                return containsPin(RP2350_UART_0_Tx_Valid_Pins, gpio)
                    || (isQfn80 && containsPin(RP2350_QFN80_UART_0_Tx_Valid_Pins, gpio));
            case 1:
                return containsPin(RP2350_UART_1_Tx_Valid_Pins, gpio)
                    || (isQfn80 && containsPin(RP2350_QFN80_UART_1_Tx_Valid_Pins, gpio));
            default:
                return false;
        }
    #else
        (void)uartNumber;
        (void)gpio;
        return false;
    #endif
}

#endif // !VALID_PINS_HEADER