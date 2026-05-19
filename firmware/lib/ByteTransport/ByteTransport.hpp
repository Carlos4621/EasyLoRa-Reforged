#ifndef BYTE_TRANSPORT_HEADER
#define BYTE_TRANSPORT_HEADER

#include <cstdint>

class ByteTransport {
public:
    virtual uint8_t read() = 0;
    virtual void write(uint8_t byte) = 0;
};

#endif // !BYTE_TRANSPORT_HEADER