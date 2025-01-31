#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <cstdint>

namespace Msg {
    enum class Opcodes : uint8_t {
        Echo,
        Time,
        Download,
        Upload,
        Close,

        MAX
    };
};

#endif