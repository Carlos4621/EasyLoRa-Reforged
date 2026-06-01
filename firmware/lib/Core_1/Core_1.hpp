#ifndef CORE_1_MAIN_HEADER
#define CORE_1_MAIN_HEADER

#include "hardware/sync.h"
#include <array>
#include "ProtocolDecoder.hpp"
#include "Instances.hpp"

/*
    TODO: Terminar estas clases
*/

/// @brief Clase que representa al Core 1, el core se encarga de la codificación, decodificación y armado de respuestas
///        También notifica al Core 0 en caso de un paquete Tx listo para ser enviado
class Core_1 final {
public:

    /// @brief Programa principal del Core 1, no retorna
    [[noreturn]]
    static void main() noexcept;

private:
    static size_t bytesWritten_m;

    static std::array<uint8_t, Rx_Buffer_Size> codifiedBuffer_m;
    static std::array<uint8_t, Rx_Buffer_Size> frameBytes_m;
    static std::array<uint8_t, Rx_Buffer_Size - Frame::Header_Size> framePayload_m;
};

#endif // !CORE_1_MAIN_HEADER