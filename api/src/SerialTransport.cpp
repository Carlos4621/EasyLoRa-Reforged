#include "SerialTransport.hpp"

using namespace boost;

SerialTransport::SerialTransport(boost::asio::io_context &context)
    : strand_m{ context.get_executor() }
    , serial_m{ context }
{
}

void SerialTransport::open(std::string_view portPath, const SerialConfig &config) {
    asio::post(
        strand_m,
        [this, config, portPath] {
            serial_m.set_option(asio::serial_port_base::baud_rate(config.baudRate));
            serial_m.set_option(asio::serial_port_base::character_size(config.characterSize));
            serial_m.set_option(asio::serial_port_base::parity(config.parityByte));
            serial_m.set_option(asio::serial_port_base::stop_bits(config.stopBits));
            serial_m.set_option(asio::serial_port_base::flow_control(config.flowControl));

            serial_m.open(portPath.data());

            startRead();
        }
    );
}

void SerialTransport::close() {
    asio::post(strand_m, [this] {
        boost::system::error_code ignored;

        serial_m.cancel(ignored);
        serial_m.close(ignored);

        txDeque_m.clear();
        rxBuffer_m.clear();
    });
}

void SerialTransport::setFrameHandler(const FrameHandler &handler) noexcept {
    frameHandler_m = handler;
}

void SerialTransport::setErrorHandler(const ErrorHandler &handler) noexcept {
    errorHandler_m = handler;
}

void SerialTransport::asyncWrite(std::vector<uint8_t> toWrite) {
    asio::post(
        strand_m,
        [this, toWrite = std::move(toWrite)] {
            const bool idle{ txDeque_m.empty() };

            txDeque_m.emplace_back(std::move(toWrite));
    
            if (idle) {
                doWrite();
            }
        }
    );
}

void SerialTransport::startRead() {
    asio::async_read_until(
        serial_m,
        asio::dynamic_buffer(rxBuffer_m),
        Frame::Frame_Delimiter,
        asio::bind_executor(
            strand_m,
            [this](boost::system::error_code ec, std::size_t n) {
                handleRead(ec, n);
            }
        )
    );
}

void SerialTransport::handleRead(boost::system::error_code ec, size_t bytesWritten) {
    if (ec.failed()) {
        if (errorHandler_m != nullptr) {
            errorHandler_m(ec);
        }
        return;
    }
    
    std::vector<uint8_t> package(rxBuffer_m.cbegin(), rxBuffer_m.cbegin() + bytesWritten);

    rxBuffer_m.erase(rxBuffer_m.cbegin(), rxBuffer_m.cbegin() + bytesWritten);
    
    if (frameHandler_m != nullptr) {
        frameHandler_m(std::move(package));
    }
}

void SerialTransport::doWrite() {
    if(txDeque_m.empty()) {
        return;
    }

    asio::async_write(
        serial_m,
        asio::dynamic_buffer(txDeque_m.front()),
        asio::bind_executor(
            strand_m,
            [this](boost::system::error_code ec, size_t bytesWritten) {
                handleWrite(ec, bytesWritten);
            }
        )
    );
}

void SerialTransport::handleWrite(boost::system::error_code ec, size_t bytesWritten) {
    if (ec) {
        if (errorHandler_m != nullptr) {
            errorHandler_m(ec);
        }
        return;
    }

    txDeque_m.pop_front();

    if (!txDeque_m.empty()) {
        doWrite();
    }
}
