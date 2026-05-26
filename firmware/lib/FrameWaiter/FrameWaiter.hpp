#ifndef FRAME_WAITER_HEADER
#define FRAME_WAITER_HEADER

#include "ringbuffer.hpp"
#include <cstdint>
#include <atomic>
#include <utility>
#include <cstring>

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

    /// @brief Constructor base
    /// @param callback Función a ser llamada al recibir un frame
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
    /// @warning Solo debe ser usado por parte del consumidor
    [[nodiscard]]
    ReadFrameStatus tryReadFrame(uint8_t* outputBuffer, size_t bufferCapacity, size_t& bytesWritten) noexcept;

private:
    static constexpr uint8_t Packet_Delimiter{ 0x00 };

    jnk0le::Ringbuffer<uint8_t, BufferSize> buffer_m;
    std::array<uint8_t, BufferSize> pendingFrameBuffer_m;

    size_t pendingFrameSize_m{ 0 };

    std::atomic<bool> overflow_m{ false };

    std::atomic<bool> droppingFeedBytes_m{ false };

    void (*callback_m)();

    /// @brief Usado para descartar paquetes debido a fallos
    void dropBufferUntilDelimiterOrEmpty() noexcept;
};

template <size_t BufferSize>
inline FrameWaiter<BufferSize>::FrameWaiter(void (*callback)()) noexcept 
: callback_m{ callback }
{
    assert(callback_m != nullptr && "callback can't be nullptr");
}

template <size_t BufferSize>
inline void FrameWaiter<BufferSize>::feed(uint8_t byte) noexcept {
    if (droppingFeedBytes_m.load()) {
        if (byte == Packet_Delimiter) {
            droppingFeedBytes_m.store(false);
        }
        return;
    }

    if (!buffer_m.insert(byte)) {
        overflow_m.store(true);
        droppingFeedBytes_m.store(true);
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
        if (byte == Packet_Delimiter) {
            if (pendingFrameSize_m > bufferCapacity) {
                pendingFrameSize_m = 0;
                return ReadFrameStatus::BufferTooSmall;
            }
            
            std::memcpy(outputBuffer, &pendingFrameBuffer_m[0], pendingFrameSize_m);
            bytesWritten = pendingFrameSize_m;

            pendingFrameSize_m = 0;
            return ReadFrameStatus::OK;
        }

        if (pendingFrameSize_m > pendingFrameBuffer_m.size()) {
            dropBufferUntilDelimiterOrEmpty();
            pendingFrameSize_m = 0;
            return ReadFrameStatus::BufferTooSmall;
        }

        pendingFrameBuffer_m[pendingFrameSize_m++] = byte;
    }

    if (overflow_m.exchange(false)) {
        dropBufferUntilDelimiterOrEmpty();
        pendingFrameSize_m = 0;
        return ReadFrameStatus::Overflow;
    }

    return ReadFrameStatus::IncompleteFrame;
}

template <size_t BufferSize>
inline void FrameWaiter<BufferSize>::dropBufferUntilDelimiterOrEmpty() noexcept {
    uint8_t byte{};

    while (buffer_m.remove(byte)) {
        if (byte == Packet_Delimiter) {
            return;
        }
    }
}

#endif // !FRAME_WAITER_HEADER
