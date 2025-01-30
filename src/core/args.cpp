#include "args.h"

#include <cstdint>
#include <limits>

template<>
bool ParseTo<const char*>(const char* string, const char*& outValue) {
    outValue = string;
    return true;
}

template<>
bool ParseTo<bool>(const char* string, bool& outValue) {
    const auto sv = std::string_view(string);
    if (sv.size() > 1) {
        if (sv != "true" && sv != "false") return false;
        outValue = (sv == "true"); 
    } else {
        if (sv[0] != '0' && sv[0] != '1') return false;
        outValue = (sv[0] == '1');
    }

    return true;
}

template<>
bool ParseTo<short>(const char* string, short& outValue) {
    int value;
    if (ParseTo<int>(string, value) && value <= std::numeric_limits<short>::max() && value >= std::numeric_limits<short>::min()) {
        outValue = value;
        return true;
    }
    return false;
}

template<>
bool ParseTo<int>(const char* string, int& outValue) {
    try {
        outValue = std::stoi(string);
    } catch (...) {
        return false;
    }
    return true;
}

template<>
bool ParseTo<unsigned short>(const char* string, unsigned short& outValue) {
    unsigned int value;
    if (
        ParseTo<unsigned int>(string, value) &&
        value <= std::numeric_limits<unsigned short>::max() &&
        value >= std::numeric_limits<unsigned short>::min()
    ) {
        outValue = value;
        return true;
    }
    return false;
}

template<>
bool ParseTo<unsigned int>(const char* string, unsigned int& outValue) {
    try {
        outValue = std::stoul(string);
    } catch (...) {
        return false;
    }
    return true;
}