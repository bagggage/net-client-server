#ifndef _UTILS_H
#define _UTILS_H

#include <iostream>

#ifndef LIBPOG_RELEASE
#define LIBPOG_LOGS true
#define LIBPOG_PREFIX "libPOG"

#include <cassert>
#define LIBPOG_ASSERT(cond, msg) assert((cond) && msg)
#else
#define LIBPOG_LOGS false
#define LIBPOG_PREFIX

#define LIBPOG_ASSERT(...) (void())
#endif

namespace Net {
    class Utils {
    public:
        template<typename... Args>
        static inline void Error(const Args&... args) {
            if constexpr (!(LIBPOG_LOGS)) {
                return;
            }

            std::cerr << LIBPOG_PREFIX " [Error]: ";
            ((std::cerr << args), ...);
            std::cerr << std::endl;
        }

        template<typename... Args>
        static inline void Warn(const Args&... args) {
            if constexpr (!(LIBPOG_LOGS)) {
                return;
            }

            std::cerr << LIBPOG_PREFIX " [Warning]: ";
            ((std::cerr << args), ...);
            std::cerr << std::endl;
        }
    };
}; // namespace Net

#endif