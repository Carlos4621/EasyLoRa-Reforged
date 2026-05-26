#ifndef FRAME_WAITER_HEADER
#define FRAME_WAITER_HEADER

#include "ringbuffer.hpp"
#include <cstdint>
#include <atomic>
#include <utility>

/*
    TODO:
        - Asegurarse de que dropUntilDelimiter no provoca bucle infinito o problemático
        - Agregar funcionalidad para lecturas incompletas de un frame
*/

enum class ReadFrameStatus : uint8_t {
    OK = 0,
    NoFrame,
    BufferTooSmall,
    BufferNullptr,
    IncompleteFrame,
    Overflow
};

/// @brief Clase que contabiliza los frames llegados y administra el ringbuffer. Se supone que se usa COBS con delimitador 0x00
/// @tparam BufferSize Tamaño del ringbuffer
template<size_t BufferSize>
class FrameWaiter {
public:

    explicit FrameWaiter(void (*callback)()) noexcept;

    /// @brief Añade un byte al buffer y lo analiza para indicar si hubo un frame completo llegado u overflow.
    /// @param byte Byte a agregar
    /// @warning Solo debe usarse por parte del productor
    void feed(uint8_t byte) noexcept;

    /// @brief Intenta leer un frame del ring buffer
    /// @param outputBuffer Puntero al buffer donde se colocarán los datos
    /// @param bufferCapacity Capacidad máxima del buffer de salida
    /// @param bytesWritten Número de bytes escritos en el buffer
    /// @return Estado de la lectura
    /// @warning Solo debe ser usado por parte del consumidor al recibir el callback
    [[nodiscard]]
    ReadFrameStatus tryReadFrame(uint8_t* outputBuffer, size_t bufferCapacity, size_t& bytesWritten) noexcept;

private:
    static constexpr uint8_t Packet_Delimiter{ 0x00 };

    jnk0le::Ringbuffer<uint8_t, BufferSize> buffer_m;

    std::atomic<bool> overflow_m{ false };

    volatile bool droppingBytes_m{ false };

    void (*callback_m)();

    /// @brief Usado para descartar paquetes debido a fallos
    void dropUntilDelimiter() noexcept;
};

template <size_t BufferSize>
inline FrameWaiter<BufferSize>::FrameWaiter(void (*callback)()) noexcept 
: callback_m{ callback }
{
}

template <size_t BufferSize>
inline void FrameWaiter<BufferSize>::feed(uint8_t byte) noexcept {
    if (droppingBytes_m) {
        if (byte == Packet_Delimiter) {
            droppingBytes_m = false;
        }
        return;
    }

    if (!buffer_m.insert(byte)) {
        overflow_m.store(true);
        droppingBytes_m = true;
        return;
    }
    
    if (byte == Packet_Delimiter) {
        if (callback_m != nullptr) {
            callback_m();
        }
    }
}

template <size_t BufferSize>
inline ReadFrameStatus FrameWaiter<BufferSize>::tryReadFrame(uint8_t *outputBuffer, size_t bufferCapacity, size_t &bytesWritten) noexcept {
    bytesWritten = 0;

    if (outputBuffer == nullptr) {
        return ReadFrameStatus::BufferNullptr;
    }

    uint8_t byte{};

    while (buffer_m.remove(byte)) {
        if (overflow_m) {
            dropUntilDelimiter();
            overflow_m.store(false);
            return ReadFrameStatus::Overflow;
        }
        
        if (byte == Packet_Delimiter) {
            return ReadFrameStatus::OK;
        }

        if (bytesWritten == bufferCapacity) {
            dropUntilDelimiter();
            return ReadFrameStatus::BufferTooSmall;
        }

        outputBuffer[bytesWritten++] = byte;
    }

    return ReadFrameStatus::IncompleteFrame;
}

template <size_t BufferSize>
inline void FrameWaiter<BufferSize>::dropUntilDelimiter() noexcept {
    uint8_t byte{};

    do {
        buffer_m.remove(byte);
    } while (byte != Packet_Delimiter);
}

#endif // !FRAME_WAITER_HEADER
