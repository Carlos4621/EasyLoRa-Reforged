#include "Header/Header.hpp"
#include "detail/BytesUtilities.hpp"
#include <utility>

namespace protocol {

    std::array<std::byte, Header::Header_Size> Header::toBytes() const noexcept {
        std::array<std::byte, Header_Size> headerBytes{};

        headerBytes[std::to_underlying(HeaderIndex::Version)] = version;
        headerBytes[std::to_underlying(HeaderIndex::Flags)] = flags;
        headerBytes[std::to_underlying(Header::HeaderIndex::PayloadLenght_High)] = detail::getHighByte(payloadLenght);
        headerBytes[std::to_underlying(HeaderIndex::PayloadLenght_Low)] = detail::getLowByte(payloadLenght);

        return headerBytes;
    }

} // namespace protocol