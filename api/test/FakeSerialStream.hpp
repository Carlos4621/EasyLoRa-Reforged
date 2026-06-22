#ifndef FAKE_SERIAL_STREAM_HEADER
#define FAKE_SERIAL_STREAM_HEADER

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "boost/asio.hpp"
#include "boost/system/error_code.hpp"
#include "Frame.hpp"

class FakeSerialStream {
public:
    struct State {
        bool openCalled{ false };
        bool closeCalled{ false };
        bool cancelCalled{ false };
        size_t closeCount{ 0 };
        size_t cancelCount{ 0 };
        std::string openedPath;
        size_t setOptionCount{ 0 };
        std::optional<unsigned int> baudRate;
        std::optional<unsigned int> characterSize;
        std::optional<boost::asio::serial_port_base::flow_control::type> flowControl;
        std::optional<boost::asio::serial_port_base::parity::type> parity;
        std::optional<boost::asio::serial_port_base::stop_bits::type> stopBits;
        std::deque<uint8_t> incomingBytes;
        std::vector<std::vector<uint8_t>> writes;
        bool autoCompleteWrites{ true };
        std::function<void(boost::system::error_code)> pendingWrite;
        std::optional<boost::system::error_code> nextOpenError;
        std::optional<boost::system::error_code> nextSetOptionError;
        std::optional<boost::system::error_code> nextReadError;
        std::optional<boost::system::error_code> nextWriteError;
        std::function<void()> pendingRead;

        void feedIncoming(std::vector<uint8_t> bytes) {
            incomingBytes.insert(incomingBytes.end(), bytes.begin(), bytes.end());
            completePendingRead();
        }

        void failNextRead(boost::system::error_code ec) {
            nextReadError = ec;
            completePendingRead();
        }

        void failNextWrite(boost::system::error_code ec) {
            nextWriteError = ec;
        }

        void failNextOpen(boost::system::error_code ec) {
            nextOpenError = ec;
        }

        void failNextSetOption(boost::system::error_code ec) {
            nextSetOptionError = ec;
        }

        void completePendingWrite(boost::system::error_code ec = {}) {
            if (pendingWrite == nullptr) {
                return;
            }

            auto writeOperation{ std::move(pendingWrite) };
            pendingWrite = nullptr;
            writeOperation(ec);
        }

    private:
        void completePendingRead() {
            if (pendingRead == nullptr) {
                return;
            }

            auto readOperation{ std::move(pendingRead) };
            pendingRead = nullptr;
            readOperation();
        }
    };

    using executor_type = boost::asio::io_context::executor_type;

    explicit FakeSerialStream(boost::asio::io_context& context)
        : executor_m{ context.get_executor() }
        , state_m{ consumePreparedState() }
    {
    }

    static std::shared_ptr<State> prepareNextInstance() {
        nextState_m = std::make_shared<State>();
        return nextState_m;
    }

    executor_type get_executor() noexcept {
        return executor_m;
    }

    void open(const char* path) {
        state_m->openCalled = true;
        state_m->openedPath = path;

        if (state_m->nextOpenError.has_value()) {
            const auto ec{ *state_m->nextOpenError };
            state_m->nextOpenError.reset();
            throw boost::system::system_error(ec);
        }
    }

    template <class Option>
    void set_option(const Option& option) {
        ++state_m->setOptionCount;

        if (state_m->nextSetOptionError.has_value()) {
            const auto ec{ *state_m->nextSetOptionError };
            state_m->nextSetOptionError.reset();
            throw boost::system::system_error(ec);
        }

        recordOption(option);
    }

    void cancel(boost::system::error_code& ec) {
        state_m->cancelCalled = true;
        ++state_m->cancelCount;
        ec.clear();

        if (state_m->pendingRead != nullptr) {
            state_m->nextReadError = boost::asio::error::operation_aborted;
            auto readOperation{ std::move(state_m->pendingRead) };
            state_m->pendingRead = nullptr;
            readOperation();
        }
    }

    void close(boost::system::error_code& ec) {
        state_m->closeCalled = true;
        ++state_m->closeCount;
        ec.clear();
    }

    template <class MutableBufferSequence, class ReadHandler>
    void async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {
        auto operation = [this, buffers, handler = std::forward<ReadHandler>(handler)]() mutable {
            completeRead(buffers, std::move(handler));
        };

        if (state_m->incomingBytes.empty() && !state_m->nextReadError.has_value()) {
            state_m->pendingRead = std::move(operation);
            return;
        }

        operation();
    }

    template <class ConstBufferSequence, class WriteHandler>
    void async_write_some(const ConstBufferSequence& buffers, WriteHandler&& handler) {
        boost::system::error_code ec;
        size_t bytesWritten{ 0 };

        if (state_m->nextWriteError.has_value()) {
            ec = *state_m->nextWriteError;
            state_m->nextWriteError.reset();
        } else {
            auto& written = state_m->writes.emplace_back();

            for (auto it = boost::asio::buffer_sequence_begin(buffers);
                 it != boost::asio::buffer_sequence_end(buffers);
                 ++it) {
                const auto* data{ static_cast<const uint8_t*>(it->data()) };
                const auto size{ it->size() };
                written.insert(written.end(), data, data + size);
                bytesWritten += size;
            }
        }

        auto completion = [handler = std::forward<WriteHandler>(handler), ec, bytesWritten](
                              boost::system::error_code forcedEc = {}) mutable {
            handler(forcedEc ? forcedEc : ec, forcedEc ? 0U : bytesWritten);
        };

        if (!state_m->autoCompleteWrites) {
            state_m->pendingWrite = [completion = std::move(completion)](boost::system::error_code forcedEc) mutable {
                completion(forcedEc);
            };
            return;
        }

        boost::asio::post(executor_m, [completion = std::move(completion)]() mutable {
            completion();
        });
    }

private:
    executor_type executor_m;
    std::shared_ptr<State> state_m;
    static inline std::shared_ptr<State> nextState_m{};

    static std::shared_ptr<State> consumePreparedState() {
        if (nextState_m == nullptr) {
            return std::make_shared<State>();
        }

        auto state{ nextState_m };
        nextState_m.reset();
        return state;
    }

    static void recordOptionValue(State& state, const boost::asio::serial_port_base::baud_rate& option) {
        state.baudRate = option.value();
    }

    static void recordOptionValue(State& state, const boost::asio::serial_port_base::character_size& option) {
        state.characterSize = option.value();
    }

    static void recordOptionValue(State& state, const boost::asio::serial_port_base::flow_control& option) {
        state.flowControl = option.value();
    }

    static void recordOptionValue(State& state, const boost::asio::serial_port_base::parity& option) {
        state.parity = option.value();
    }

    static void recordOptionValue(State& state, const boost::asio::serial_port_base::stop_bits& option) {
        state.stopBits = option.value();
    }

    template <class Option>
    void recordOption(const Option& option) {
        recordOptionValue(*state_m, option);
    }

    template <class MutableBufferSequence, class ReadHandler>
    void completeRead(const MutableBufferSequence& buffers, ReadHandler&& handler) {
        boost::system::error_code ec;
        size_t bytesRead{ 0 };

        if (state_m->nextReadError.has_value()) {
            ec = *state_m->nextReadError;
            state_m->nextReadError.reset();
        } else {
            for (auto it = boost::asio::buffer_sequence_begin(buffers);
                 it != boost::asio::buffer_sequence_end(buffers) && !state_m->incomingBytes.empty();
                 ++it) {
                auto* data{ static_cast<uint8_t*>(it->data()) };
                const auto size{ it->size() };

                for (size_t offset{ 0 }; offset < size && !state_m->incomingBytes.empty(); ++offset) {
                    const auto byte{ state_m->incomingBytes.front() };
                    data[offset] = byte;
                    state_m->incomingBytes.pop_front();
                    ++bytesRead;

                    if (byte == Frame::Frame_Delimiter) {
                        return postReadCompletion(std::forward<ReadHandler>(handler), ec, bytesRead);
                    }
                }
            }
        }

        postReadCompletion(std::forward<ReadHandler>(handler), ec, bytesRead);
    }

    template <class ReadHandler>
    void postReadCompletion(ReadHandler&& handler, boost::system::error_code ec, size_t bytesRead) {
        boost::asio::post(
            executor_m,
            [handler = std::forward<ReadHandler>(handler), ec, bytesRead]() mutable {
                handler(ec, bytesRead);
            }
        );
    }
};

#endif // !FAKE_SERIAL_STREAM_HEADER
