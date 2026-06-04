#ifndef CORE_1_MAIN_HEADER
#define CORE_1_MAIN_HEADER

#include "hardware/sync.h"
#include <array>
#include "ProtocolDecoder.hpp"
#include "Instances.hpp"
#include "FrameResponder.hpp"
#include "ProtocolCodec.hpp"

/*
    TODO:
        - Considerar lo siguiente:
            Realmente es buena idea colocar un Framewaiter como cola de Tx? Puede ser algo redundante e innecesario el análisis byte a byte de un
            paquete que se ha codificado totalmente y verificado en el propio firmware
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

    static std::array<uint8_t, Rx_Buffer_Size> receivedFrameCodifiedBuffer_m;
    static std::array<uint8_t, Rx_Buffer_Size> receivedFrameBytes_m;
    static std::array<uint8_t, Rx_Buffer_Size> receivedFramePayload_m;
    
    static std::array<uint8_t, Tx_Buffer_Size> responseFrameCodifiedBuffer_m;
    static std::array<uint8_t, Tx_Buffer_Size> responseFramePayload_m;
    static std::array<uint8_t, Tx_Buffer_Size> responseFrameBytes_m;
};

#endif // !CORE_1_MAIN_HEADER