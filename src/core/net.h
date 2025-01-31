#ifndef _NET_H
#define _NET_H

#include <string>
#include <array>

namespace Net {
    struct MacAddress {
        std::array<uint8_t, 6> bytes = {0};

        std::string ToString() const;
    };

    MacAddress GetMacAddress();
}

#endif 