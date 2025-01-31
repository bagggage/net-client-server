#ifndef _NET_H
#define _NET_H

#include <array>
#include <cstdint>
#include <string>

namespace Net {
    struct MacAddress {
        std::array<uint8_t, 6> bytes = {0};

        std::string ToString() const;
    };

    MacAddress GetMacAddress();
}

#endif 