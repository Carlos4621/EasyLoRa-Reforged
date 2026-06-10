#include <cstdint>
#include <vector>

#include "Frame.hpp"
#include "ProtocolCodec.hpp"

int main() {
    std::vector<uint8_t> payload{ 0x10, 0x20, 0x30 };

    Frame frame{};
    frame.version = Frame::Actual_Frame_Version;
    frame.kind = PackageKind::Request;
    frame.flags = 0x00;
    frame.reserved = 0x00;
    frame.seq = 0x0001;
    frame.type = MessageType::SendRadioPacket;
    frame.payload = payload;

    std::vector<uint8_t> rawBuffer(ProtocolCodec::minimumFrameBytesBufferSize(frame));
    std::vector<uint8_t> outputBuffer(ProtocolCodec::minimumOutputBufferSize(frame));

    const auto encoded = ProtocolCodec::encode(frame, rawBuffer, outputBuffer);
    return encoded.has_value() ? 0 : 1;
}
