#ifndef _NET_H
#define _NET_H

#include <string>

namespace Net {
    class NetUtils {
    public:
        static std::string GetMacAddress();
    };
}

#endif 