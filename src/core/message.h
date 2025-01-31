#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <cstdint>

namespace Msg {
    static constexpr uint16_t DEFAULT_SERVER_PORT = 5252;

    enum class Opcodes : uint8_t {
        Echo,
        Time,
        Download,
        Upload,
        Close,

        MAX
    };

namespace Request {
    struct Download { char fileName[256]; };
};

namespace Response {
    struct Download {
        enum Status {
            Ready,
            NoSuchFile,
        };

        Status status;
        uintptr_t totalSize;
    };
};

};

#endif