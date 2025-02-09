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

        DownloadRecovery,

        MAX
    };

namespace Request {
    struct Download {
        size_t position;
        char fileName[];
    };
    struct Upload {
        size_t fileSize;
        char fileName[];
    };
};

namespace Response {
    struct DownloadRecovery {
        size_t position;
        char fileName[];
    };

    struct Download {
        enum Status {
            Ready,
            NoSuchFile,
            IsNotFile,
        };

        Status status;
        size_t totalSize;
    };

    struct Time {
        std::time_t time;
    };
};

};

#endif