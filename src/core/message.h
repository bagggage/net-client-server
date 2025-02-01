#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <cstdint>
#include <chrono>

namespace Msg {
    static constexpr uint16_t DEFAULT_SERVER_PORT = 5252;

    enum class Opcodes : uint8_t {
        None,
        Echo,
        Time,
        Download,
        Upload,
        Close,

        MAX
    };

namespace Request {
    struct Download { char fileName[256]; };
    struct Upload {
        size_t fileSize;
        char fileName[];
    };
};

namespace Response {
    struct Download {
        enum Status {
            Ready,
            NoSuchFile,
            IsNotFile,
        };

        Status status;
        uintmax_t totalSize;
    };

    struct Time {
        std::time_t time;
    };
};

};

#endif