#ifndef _NET_H
#define _NET_H

#include <array>
#include <cstdint>
#include <string>

namespace Net {
    struct MacAddress {
        std::array<uint8_t, 6> bytes = {0};

        std::string ToString() const;

        inline bool operator==(const MacAddress& other) const {
            return bytes == other.bytes;
        }

        //friend void swap(MacAddress& a, MacAddress& b) { std::swap(a.bytes, b.bytes); }
    };

    MacAddress GetMacAddress();
}

template<>
struct std::hash<Net::MacAddress> {
    std::size_t operator()(const Net::MacAddress& address) const noexcept {
        const std::string_view sv(
            reinterpret_cast<const char*>(address.bytes.data()),
            address.bytes.size()
        );
        return std::hash<std::string_view>{}(sv);
    }
};

#endif 