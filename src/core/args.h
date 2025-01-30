#ifndef _ARGS_H
#define _ARGS_H

#include <iostream>
#include <string>
#include <cstring>

class ArgIterator {
private:
    static constexpr char OPTION_PREFIX = '-';

    const char** argStrings = nullptr;
    unsigned int argsNumber = 0;
public:
    ArgIterator() = default;
    ArgIterator(int argc, const char** argv) {
        if (argc < 0 || argv == nullptr) return;

        argStrings = argv;
        argsNumber = argc;
    }

    inline const char* Get() const { return argStrings[0]; }

    inline bool IsEnd() const { return argsNumber == 0; }
    inline bool IsOption() const { return argStrings[0][0] == OPTION_PREFIX; }
    inline bool IsValue() const { return !IsOption(); }

    bool Next() {
        if (argsNumber == 0 || argStrings == nullptr) return false;
        argsNumber--;
        argStrings++;
        return !IsEnd();
    }

    inline bool HasParameter(unsigned int index = 0) const {
        if (argsNumber <= index + 1) return false;
        return argStrings[index + 1][0] != OPTION_PREFIX;
    }

    inline const char* GetParameter(unsigned int index = 0) const {
        return argStrings[index + 1];
    }

    std::string_view GetAsOption() const {
        return std::string_view(argStrings[0] + sizeof(OPTION_PREFIX));
    }

    inline bool Is(const char* string) const {
        return std::string_view(argStrings[0]) == std::string_view(string);
    }
    inline bool Is(const std::string_view stringView) const {
        return std::string_view(argStrings[0]) == stringView;
    }
};

template<typename T>
bool ParseTo(const char* string, T& outValue) {
    static_assert(false && "Unsupported type");
    return false;
}

template<>
bool ParseTo<int>(const char*, int&);

template<>
bool ParseTo<unsigned>(const char*, unsigned&);

template<>
bool ParseTo<short>(const char*, short&);

template<>
bool ParseTo<unsigned short>(const char*, unsigned short&);

template<>
bool ParseTo<bool>(const char*, bool&);

template<>
bool ParseTo<const char*>(const char*, const char*&);

template<typename T>
static bool RequireArgParameter(ArgIterator& argIter, const char* failMessage, T& outValue) {
    if (argIter.HasParameter() == false) {
        std::cerr << failMessage << '\n';
        return false;
    }
    if (ParseTo<T>(argIter.GetParameter(), outValue) == false) [[unlikely]] {
        std::cerr << "Invalid argument format for \"" << argIter.Get() << "\". " << failMessage << '\n';
        argIter.Next();
        return false;
    }

    argIter.Next();
    return true;
}

#endif