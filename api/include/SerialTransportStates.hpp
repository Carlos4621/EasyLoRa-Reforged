#ifndef SERIAL_TRANSPORT_STATES_HEADER
#define SERIAL_TRANSPORT_STATES_HEADER

#include <variant>
#include <vector>
#include <cstdint>
#include <string>
#include "SerialTransportTypes.hpp"

namespace States {
    struct Open{};
    struct Closed{};
    struct Opening{};
    struct Closing{};
    struct Faulted{};
}

using PossibleStates = std::variant<States::Open, States::Closed, States::Opening, States::Closing, States::Faulted>;

namespace Events {
    struct Write{ std::vector<uint8_t> toWrite; size_t packetID; };
    struct Close{};
    struct Open{ std::string portPath; SerialConfig serialConfig; BufferConfig bufferConfig; };
    struct Read{ /*TODO*/ };
}

using PossibleEvents = std::variant<Events::Write, Events::Close, Events::Open, Events::Read>;

#endif // !SERIAL_TRANSPORT_STATES_HEADER