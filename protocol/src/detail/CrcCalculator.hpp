#ifndef CRC_CALCULATOR_HEADER
#define CRC_CALCULATOR_HEADER

#include <CRC.h>
#include <cstddef>
#include <expected>
#include <span>
#include "ProtocolErrors/ProtocolErrors.hpp"

namespace protocol::detail {
    class CrcCalculator {
    public:

        static constexpr uint8_t CRC_Lenght{ 2 };

        /**
         * @brief Añade CRC-16-CCITTFALSE al buffer dado
         * 
         * @param inputBuffer Buffer con el que se calculará el CRC
         * @param outputBuffer Buffer donde se localizará el inputBuffer con el CRC al final
         * @return std::expected<std::span<std::byte>, ProtocolErrors> con std::span con vista al resultado en el buffer de salida
         *         sino, ProtocolErrors con el tipo de error
        */
        [[nodiscard]]
        static std::expected<std::span<std::byte>, ProtocolErrors> addCRC(std::span<const std::byte> inputBuffer, std::span<std::byte> outputBuffer) noexcept;
        
        /**
         * @brief Valida CRC-16-CCITTFALSE y lo elimina del buffer
         * 
         * @param inputBuffer Buffer donde se localiza la data con el CRC al final
         * @param outputBuffer Buffer donde se localizará la data validada sin el CRC
         * @return std::expected<std::span<std::byte>, ProtocolErrors> con span con vista al resultado, sino ProtocolErrors con el tipo de error
        */
        [[nodiscard]]
        static std::expected<std::span<std::byte>, ProtocolErrors> removeCRC(std::span<const std::byte> inputBuffer, std::span<std::byte> outputBuffer) noexcept;

        /**
         * @brief Obtiene el tamaño mínimo que debe tener el buffer de salida en función al tamaño del buffer de entrada para la operación addCRC
         * 
         * @param inputBufferSize Buffer con el que se calculará el CRC
         * @return size_t inputBufferSize + CRC_Lenght
        */
        [[nodiscard]]
        static size_t minimumAddingOutputBufferSize(size_t inputBufferSize) noexcept;
        
        /**
         * @brief Obtiene el tamaño mínimo que debe tener el buffer de salida en función al tamaño del buffer de entrada para la operación removeCRC
         * 
         * @param inputBufferSize Tamaño del buffer de entrada
         * @return std::expected<size_t, ProtocolErrors> size_t con el tamaño mínimo necesario,
         *         en caso de que inputBufferSize < CRC_Lenght se retorna un ProtocolErrors
        */
        [[nodiscard]]
        static std::expected<size_t, ProtocolErrors> minimumRemovingOutputBufferSize(size_t inputBufferSize) noexcept;
    };
}

#endif // !CRC_CALCULATOR_HEADER