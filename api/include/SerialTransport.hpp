#ifndef SERIAL_TRANSPORT_HEADER
#define SERIAL_TRANSPORT_HEADER

#include "boost/asio.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include "Frame.hpp"
#include "SerialTransportTypes.hpp"

/// @brief Configuración default del puerto serial (9600 8N1)
static constexpr SerialConfig Default_Serial_Config{ 
    .baudRate = 9600, 
    .flowControl = boost::asio::serial_port_base::flow_control::type::none,
    .parityByte = boost::asio::serial_port_base::parity::type::none,
    .stopBits = boost::asio::serial_port::stop_bits::type::one,
    .characterSize = 8
};

/// @brief Configuración default de los buffers
static constexpr BufferConfig Default_Buffer_Config {
    .rxBufferMaxSize = 1024,
    .txMaxElementsInQueue = 10,
    .txMaxSizePerElement = 512,
};

/// @brief Clase encargada de recibir asíncronamente bytes y notificar al usuario para su posterior procesamiento
template<class Stream>
class BasicSerialTransport {
public:
    using FrameHandler = std::function<void(std::vector<uint8_t>)>;
    using FatalErrorHandler = std::function<void(FatalErrors)>;
    using RecuperableErrorHandler = std::function<void(RecoverableErrors)>;
    using WriteHandler = std::function<void(WriteResult, size_t)>;

    /// @brief Constructor base
    /// @param context Contexto de I/O
    explicit BasicSerialTransport(boost::asio::io_context& context);

    BasicSerialTransport(const BasicSerialTransport&) = delete;
    BasicSerialTransport& operator=(const BasicSerialTransport&) = delete;
    BasicSerialTransport(BasicSerialTransport&&) noexcept = default;
    BasicSerialTransport& operator=(BasicSerialTransport&&) noexcept = default;

    ~BasicSerialTransport() noexcept;

    /// @brief Abre el puerto seleccionado con las opciones dadas y empieza a escuchar, llamando a FrameHandler en caso de frame recibido
    /// @param portPath Dirección del puerto a abrir
    /// @param serialConfig Configuración del puerto serial a usar. Por defecto 9600 8N1
    /// @param bufferConfig Configuración de los tamaños máximos de los buffers internos, por defecto rxBuffer = 1024, txInQueue = 10, txMaxSize = 512
    /// @note Reinicia el transporte después de un error de I/O.
    void open(std::string portPath, SerialConfig serialConfig = Default_Serial_Config, BufferConfig bufferConfig = Default_Buffer_Config);

    /// @brief Solicita un cierre y cancelación de procesos, estos serán realizados ASAP
    void close();

    /// @brief Establece la función a la que se llamará con los datos cuando se reciba un frame completo
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(std::vector<uint8_t>)
    void setFrameHandler(FrameHandler handler) noexcept;

    /// @brief Establece la función a la que se llamará con los datos cuando se produzca un error fatal que requiera un nuevo open
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(FatalErrors)
    void setFatalErrorHandler(FatalErrorHandler handler) noexcept;

    /// @brief Establece la función a la que se llamaŕa con los datos cuando se produzca un error que puede ser manejado internamente
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(RecoverableErrors)
    void setRecuperableErrorHandler(RecuperableErrorHandler handler) noexcept;

    /// @brief Establece la función que se llamará después de escribir un frame o en caso de error
    /// @param handler Handler a colocar
    /// @note La función debe tener firma void(WriteResult, size_t)
    void setWriteHandler(WriteHandler handler) noexcept;

    /// @brief Escribe asincrónicamente los bytes dados
    /// @param toWrite Bytes a escribir
    /// @param packetID Identificador con el que se referirá al paquete al llamar a WriteHandler. Por defecto std::numeric_limits<size_t>::max()
    /// @note Se usa un sistema de colas para situaciones donde el último mensaje no se ha enviado aún y se llama de nuevo a esta función
    /// @note Las escrituras se descartan si ocurre un error de I/O; se requiere open() para reiniciar el transporte.
    /// @note En caso de éxito o error se notificará con el callback de WriteHandler
    [[nodiscard]]
    WriteStatus asyncWrite(std::vector<uint8_t> toWrite, size_t packetID = std::numeric_limits<size_t>::max());

    [[nodiscard]]
    bool isOpen() noexcept;

private:
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    struct Pimpl;

    Strand strand_m;

    std::shared_ptr<Pimpl> pimpl_m;
};

template <class Stream>
class BasicSerialTransport<Stream>::Pimpl : public std::enable_shared_from_this<Pimpl> {
public:
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    explicit Pimpl(boost::asio::io_context& context);

    void open(Strand strand, std::string portPath, SerialConfig serialConfig, BufferConfig bufferConfig);
    void close() noexcept;

    void setFrameHandler(FrameHandler handler) noexcept;
    void setFatalErrorHandler(FatalErrorHandler handler) noexcept;
    void setRecuperableErrorHandler(RecuperableErrorHandler handler) noexcept;
    void setWriteHandler(WriteHandler handler) noexcept;

    [[nodiscard]]
    WriteStatus asyncWrite(Strand strand, std::vector<uint8_t> toWrite, size_t packetID);

    void requestClose(Strand strand) noexcept;
    void closeOnStrand() noexcept;

    [[nodiscard]]
    bool isOpen() const noexcept;

private:
    enum class State : uint8_t { Open, Opening, Closed, Closing, Faulted };

    Stream serial;

    BufferConfig bufferConfig{ Default_Buffer_Config };
    std::vector<uint8_t> rxBuffer;
    std::deque<std::pair<std::vector<uint8_t>, size_t>> txDeque;

    FrameHandler frameHandler{ nullptr };
    FatalErrorHandler fatalErrorHandler{ nullptr };
    RecuperableErrorHandler recuperableErrorHandler{ nullptr };
    WriteHandler writeHandler{ nullptr };
    
    std::atomic_bool closing{ false };
    std::atomic_bool stopped{ true };
    size_t generation{ 0 };
    std::atomic<size_t> pendingWrites{ 0 };

    std::atomic<State> currentState_m{ State::Closed };

    void startRead(Strand strand);
    void handleRead(Strand strand, boost::system::error_code ec, size_t bytesRead, size_t operationGeneration);

    void doWrite(Strand strand);
    void handleWrite(Strand strand, boost::system::error_code ec, size_t bytesWritten, size_t operationGeneration);

    void stopAfterIoError() noexcept;

    [[nodiscard]]
    FatalErrors clarifyFatalError(boost::system::error_code ec);

    [[nodiscard]]
    RecoverableErrors clarifyRecoverableError(boost::system::error_code ec);

    void recoverFromError(Strand strand, RecoverableErrors error);

    void dispatchError(Strand strand, boost::system::error_code ec) noexcept;

    [[nodiscard]]
    bool isRecoverableError(boost::system::error_code ec) noexcept;
};

template <class Stream>
inline BasicSerialTransport<Stream>::BasicSerialTransport(boost::asio::io_context &context) 
    : strand_m{ boost::asio::make_strand(context) }
    , pimpl_m{ std::make_shared<Pimpl>(context) }
{
}

template <class Stream>
inline BasicSerialTransport<Stream>::~BasicSerialTransport() noexcept {
    if (pimpl_m != nullptr) {
        pimpl_m->requestClose(strand_m);
    }
}

template <class Stream>
inline void BasicSerialTransport<Stream>::open(std::string portPath, SerialConfig serialConfig, BufferConfig bufferConfig) {
    boost::asio::post(
        strand_m,
        [pimpl_m = pimpl_m,
         strand = strand_m,
         portPath = std::move(portPath),
         serialConfig = std::move(serialConfig),
         bufferConfig = std::move(bufferConfig)] mutable {
            pimpl_m->open(strand, std::move(portPath), std::move(serialConfig), std::move(bufferConfig));
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::close() {
    boost::asio::post(
        strand_m,
        [pimpl_m = pimpl_m] {
            pimpl_m->close();
        }
    );
}

template<class Stream>
inline void BasicSerialTransport<Stream>::setFrameHandler(FrameHandler handler) noexcept {
    boost::asio::post(
        strand_m,
        [pimpl_m = pimpl_m, handler = std::move(handler)] mutable {
            pimpl_m->setFrameHandler(std::move(handler));
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::setFatalErrorHandler(FatalErrorHandler handler) noexcept {
    boost::asio::post(
        strand_m,
        [pimpl_m = pimpl_m, handler = std::move(handler)] mutable {
            pimpl_m->setFatalErrorHandler(std::move(handler));
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::setRecuperableErrorHandler(RecuperableErrorHandler handler) noexcept {
    boost::asio::post(
        strand_m,
        [pimpl_m = pimpl_m, handler = std::move(handler)] mutable {
            pimpl_m->setRecuperableErrorHandler(std::move(handler));
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::setWriteHandler(WriteHandler handler) noexcept {
   boost::asio::post(
        strand_m,
        [pimpl_m = pimpl_m, handler = std::move(handler)] mutable {
            pimpl_m->setWriteHandler(std::move(handler));
        }
    );
}

template <class Stream>
inline WriteStatus BasicSerialTransport<Stream>::asyncWrite(std::vector<uint8_t> toWrite, size_t packetID) {
    return pimpl_m->asyncWrite(strand_m, std::move(toWrite), packetID);
}

template <class Stream>
inline bool BasicSerialTransport<Stream>::isOpen() noexcept {
    return pimpl_m->isOpen();
}

// pimpl

template <class Stream>
inline BasicSerialTransport<Stream>::Pimpl::Pimpl(boost::asio::io_context &context)
    : serial{ context }
{
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::open(
    Strand strand,
    std::string portPath,
    SerialConfig serialConfig,
    BufferConfig newBufferConfig
) {
    if (closing.load()) {
        return;
    }

    ++generation;
    stopped.store(true);
    currentState_m.store(State::Opening);
    rxBuffer.clear();
    txDeque.clear();
    pendingWrites.store(0);

    bufferConfig = std::move(newBufferConfig);

    rxBuffer.reserve(bufferConfig.rxBufferMaxSize);

    try {
        serial.open(portPath.data());

        serial.set_option(boost::asio::serial_port_base::baud_rate(serialConfig.baudRate));
        serial.set_option(boost::asio::serial_port_base::character_size(serialConfig.characterSize));
        serial.set_option(boost::asio::serial_port_base::parity(serialConfig.parityByte));
        serial.set_option(boost::asio::serial_port_base::stop_bits(serialConfig.stopBits));
        serial.set_option(boost::asio::serial_port_base::flow_control(serialConfig.flowControl));
    }
    catch (...) {
        currentState_m.store(State::Faulted);
        throw;
    }

    stopped.store(false);
    currentState_m.store(State::Open);
    startRead(strand);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::close() noexcept {
    closeOnStrand();
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::setFrameHandler(FrameHandler handler) noexcept {
    if (closing.load()) {
        return;
    }

    frameHandler = std::move(handler);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::setFatalErrorHandler(FatalErrorHandler handler) noexcept {
    if (closing.load()) {
        return;
    }

    fatalErrorHandler = std::move(handler);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::setRecuperableErrorHandler(RecuperableErrorHandler handler) noexcept {
    if (closing.load()) {
        return;
    }

    recuperableErrorHandler = std::move(handler);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::setWriteHandler(WriteHandler handler) noexcept {
    if (closing.load()) {
        return;
    }

    writeHandler = std::move(handler);
}

template <class Stream>
inline WriteStatus BasicSerialTransport<Stream>::Pimpl::asyncWrite(Strand strand, std::vector<uint8_t> toWrite, size_t packetID) {
    if (closing.load() || stopped.load()) {
        return WriteStatus::Closed;
    }

    if (toWrite.size() > bufferConfig.txMaxSizePerElement) {
        return WriteStatus::MessageTooLong;
    }

    auto currentPendingWrites{ pendingWrites.load() };
    while (true) {
        if (currentPendingWrites >= bufferConfig.txMaxElementsInQueue) {
            return WriteStatus::QueueFull;
        }

        if (pendingWrites.compare_exchange_weak(currentPendingWrites, currentPendingWrites + 1)) {
            break;
        }
    }

    auto self{ Pimpl::shared_from_this() };
    boost::asio::post(
        strand,
        [self, strand, toWrite = std::move(toWrite), packetID] mutable {
            if (self->closing.load() || self->stopped.load()) {
                self->pendingWrites.fetch_sub(1);

                if (self->writeHandler != nullptr) {
                    self->writeHandler(WriteResult::Cancelled, packetID);
                }

                return;
            }

            const bool idle{ self->txDeque.empty() };

            self->txDeque.emplace_back(std::move(toWrite), packetID);
    
            if (idle) {
                self->doWrite(strand);
            }
        }
    );

    return WriteStatus::Scheduled;
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::startRead(Strand strand) {
    if (closing.load() || stopped.load()) {
        return;
    }

    auto self{ Pimpl::shared_from_this() };
    const auto operationGeneration{ generation };

    boost::asio::async_read_until(
        serial,
        boost::asio::dynamic_buffer(rxBuffer, bufferConfig.rxBufferMaxSize),
        Frame::Frame_Delimiter,
        boost::asio::bind_executor(
            strand,
            [self, strand, operationGeneration](auto ec, auto bytesRead) {
                self->handleRead(strand, ec, bytesRead, operationGeneration);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::handleRead(
    Strand strand,
    boost::system::error_code ec,
    size_t bytesRead,
    size_t operationGeneration
) {
    if (closing.load() || stopped.load() || operationGeneration != generation) {
        return;
    }

    if (ec) {
        dispatchError(strand, ec);
        return;
    }

    std::vector<uint8_t> package(rxBuffer.cbegin(), rxBuffer.cbegin() + bytesRead);

    if (frameHandler != nullptr) {
        frameHandler(std::move(package));
    }

    rxBuffer.erase(rxBuffer.cbegin(), rxBuffer.cbegin() + bytesRead);

    startRead(strand);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::doWrite(Strand strand) {
    if (closing.load() || stopped.load() || txDeque.empty()) {
        return;
    }

    auto self{ Pimpl::shared_from_this() };
    const auto operationGeneration{ generation };

    boost::asio::async_write(
        serial,
        boost::asio::dynamic_buffer(txDeque.front().first),
        boost::asio::bind_executor(
            strand,
            [self, strand, operationGeneration](auto ec, auto bytesWritten) {
                self->handleWrite(strand, ec, bytesWritten, operationGeneration);
            }
        )
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::handleWrite(
    Strand strand,
    boost::system::error_code ec,
    size_t bytesWritten,
    size_t operationGeneration
) {
    if (closing.load() || stopped.load() || operationGeneration != generation) {
        return;
    }

    pendingWrites.fetch_sub(1);

    if (ec) {
        if (writeHandler != nullptr) {
            writeHandler(WriteResult::Failed, txDeque.front().second);
        }

        dispatchError(strand, ec);
        return;
    }

    if (writeHandler != nullptr) {
        writeHandler(WriteResult::Written, txDeque.front().second);
    }

    txDeque.pop_front();

    if (!txDeque.empty()) {
        doWrite(strand);
    }
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::stopAfterIoError() noexcept {
    stopped.store(true);
    currentState_m.store(State::Faulted);
    ++generation;
    rxBuffer.clear();
    txDeque.clear();

    boost::system::error_code ignored;
    serial.cancel(ignored);
    serial.close(ignored);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::requestClose(Strand strand) noexcept {
    auto self{ Pimpl::shared_from_this() };

    boost::asio::post(
        strand,
        [self] {
            self->closeOnStrand();
        }
    );
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::closeOnStrand() noexcept {
    if (closing.exchange(true)) {
        return;
    }

    stopped.store(true);
    currentState_m.store(State::Closing);
    ++generation;
    rxBuffer.clear();
    txDeque.clear();

    boost::system::error_code ignored;

    serial.cancel(ignored);
    serial.close(ignored);

    currentState_m.store(State::Closed);
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::dispatchError(Strand strand, boost::system::error_code ec) noexcept {
    if (isRecoverableError(ec)) {
        const auto clarifiedError{ clarifyRecoverableError(ec) };

        recoverFromError(strand, clarifiedError);

        if (recuperableErrorHandler != nullptr) {
            recuperableErrorHandler(clarifiedError);
        }
    }
    else {
        stopAfterIoError();

        if (fatalErrorHandler != nullptr) {
            fatalErrorHandler(clarifyFatalError(ec));
        }
    }
}

template <class Stream>
inline bool BasicSerialTransport<Stream>::Pimpl::isRecoverableError(boost::system::error_code ec) noexcept {
    if (ec == boost::asio::error::not_found) {
        return true;
    }

    return false;
}

template <class Stream>
inline FatalErrors BasicSerialTransport<Stream>::Pimpl::clarifyFatalError(boost::system::error_code ec) {
    using namespace boost::asio::error;
    using enum std::errc;

    if (ec == eof) {
        return FatalErrors::UnpluggedDevice;
    }

    if (ec == bad_file_descriptor || ec == bad_descriptor || ec == no_such_device) {
        return FatalErrors::ExternallyClosedPort;
    }

    if (ec == permission_denied) {
        return FatalErrors::PermissionDenied;
    }

    throw std::invalid_argument{ "Function called with a non-fatal error" };
}

template <class Stream>
inline RecoverableErrors BasicSerialTransport<Stream>::Pimpl::clarifyRecoverableError(boost::system::error_code ec) {
    if (ec == boost::asio::error::not_found) {
        return RecoverableErrors::RxBufferFull;
    }

    throw std::invalid_argument{ "Function called with a fatal error" };
}

template <class Stream>
inline void BasicSerialTransport<Stream>::Pimpl::recoverFromError(Strand strand, RecoverableErrors error) {
    rxBuffer.clear();
    startRead(strand);
}

template <class Stream>
inline bool BasicSerialTransport<Stream>::Pimpl::isOpen() const noexcept {
    return currentState_m.load() == State::Open;
}

/// @brief Clase encargada de recibir y enviar datos asíncronamente mediante el puerto serial
using SerialTransport = BasicSerialTransport<boost::asio::serial_port>;

#endif // !SERIAL_TRANSPORT_HEADER
