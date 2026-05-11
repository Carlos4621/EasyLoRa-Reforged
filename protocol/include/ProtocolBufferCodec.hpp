#ifndef FRAME_CODEC_HEADER
#define FRAME_CODEC_HEADER

#include <span>
#include <optional>
#include <algorithm>
#include "Frame.hpp"
#include "cobs/cobsr.h"
#include "BitsUtilities.hpp"
#include "CRC.h"
#include "cobs/cobsr.h"

/*
    TODO:
*/

class ProtocolBufferCodec {
public:

    [[nodiscard]]
    std::optional<size_t> encode(const Frame& frame, std::span<uint8_t> outputBuffer);

private:
};

#endif // !FRAME_CODEC_HEADER