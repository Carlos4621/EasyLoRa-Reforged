#ifndef FRAME_WAITER_HEADER
#define FRAME_WAITER_HEADER

#include "ringbuffer.hpp"
#include <cstdint>

/// @brief Clase que contabiliza los frames llegados y administra el ringbuffer. Se supone que se usa COBS con delimitador 0x00
/// @tparam BufferSize Tamaño del ringbuffer
template<size_t BufferSize>
class FrameWaiter {
public:

    /// @brief Añade un byte al buffer y lo analiza para indicar si hubo un frame completo llegado u overflow
    /// @param byte Byte a agregar
    void feed(uint8_t byte) noexcept;

    /// @brief Indica si el último feed provocó un overflow
    /// @return Indicación de overflow
    [[nodiscard]]
    bool wasOverflow() const noexcept;

    /// @brief Regresa el número de frames que han llegado
    /// @return Número de frames en el buffer
    [[nodiscard]]
    size_t framesInBuffer() const noexcept;

    /// @brief Establece la función que se llamará en caso de que llegue un frame completo
    /// @param callback Función a llamar
    void setFrameReceivedCallback(void (*callback)()) noexcept;

private:
    static constexpr uint8_t Packet_Delimiter{ 0x00 };

    jnk0le::Ringbuffer<uint8_t, BufferSize> buffer_m;

    size_t framesInBuffer_m{ 0 };
    bool wasOverflow_m{ false };

    void (*callback_m)();
};

template <size_t BufferSize>
inline void FrameWaiter<BufferSize>::feed(uint8_t byte) noexcept {
    if (!buffer_m.insert(byte)) {
        wasOverflow_m = true;
        framesInBuffer_m = 0;
        buffer_m.producerClear();
        return;
    }
    
    if (byte == Packet_Delimiter) {
        ++framesInBuffer_m;

        if (callback_m != nullptr) {
            callback_m();
        }
    }
}

template <size_t BufferSize>
inline bool FrameWaiter<BufferSize>::wasOverflow() const noexcept {
    return wasOverflow_m;
}

template <size_t BufferSize>
inline size_t FrameWaiter<BufferSize>::framesInBuffer() const noexcept {
    return framesInBuffer_m;
}

template <size_t BufferSize>
inline void FrameWaiter<BufferSize>::setFrameReceivedCallback(void(*callback)()) noexcept {
    callback_m = callback;
}

#endif // !FRAME_WAITER_HEADER
