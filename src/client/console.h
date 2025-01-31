#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <charconv>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <type_traits>

class CommandSet;

class ConsoleStream {
public:
    virtual ~ConsoleStream() = default;

    virtual std::string ReadString() = 0;
    virtual void Write(const std::string_view string) = 0;
};

class StdIoConsoleStream final : public ConsoleStream {
public:
    std::string ReadString() override;
    void Write(const std::string_view string) override;
};

class Console {
public:
    template<typename T>
    struct Context { T& Get(); };

    class ArgIterator {
    private:
        friend class Console;

        std::string_view bufferView;

        ArgIterator(std::string_view sv) : bufferView(sv) {}
    public:
        ArgIterator() = default;

        std::string_view Next();
    };

private:
    ConsoleStream* stream = nullptr;
    const CommandSet* commandSet = nullptr;

    bool isShouldExit = false;

    void PrintHelp();
public:
    Console(ConsoleStream& stream, const CommandSet& commandSet) :
        stream(&stream), commandSet(&commandSet) {}

    bool HandleCommand();

    inline bool IsShouldExit() const { return isShouldExit; }
};

template<typename TupleT, std::size_t... Is>
struct ParseHelper;

template<typename TupleT, std::size_t Idx, std::size_t... Is>
struct ParseHelper<TupleT, Idx, Is...> {
    template<typename T>
    static bool ParseArg(Console::ArgIterator& iter, const std::string_view& argStr, T& outValue) {
        if constexpr (std::is_same_v<Console::ArgIterator, T>) {
            outValue = iter;
        } else if constexpr (std::is_same_v<std::string_view, T> || std::is_same_v<std::string, T>) {
            outValue = argStr;
        } else {
            const std::from_chars_result result = std::from_chars(argStr.begin(), argStr.end(), outValue);
            if (result.ptr != argStr.end()) return false;
        }
        return true;
    }

    static bool ParseArg(ConsoleStream& stream, Console::ArgIterator& iter, TupleT& tp) {
        const auto argStr = iter.Next();
        if (argStr.empty()) {
            stream.Write("Too few agruments for command call.\n");
            return false;
        }

        auto& argValue = std::get<Idx>(tp);
        if (ParseArg(iter, argStr, argValue) == false) {
            stream.Write("Invalid argument format: \"");
            stream.Write(argStr);
            stream.Write("\".\n");
            return false;
        }
        return ParseHelper<TupleT, Is...>::ParseArg(stream, iter, tp);
    }
};

template<typename TupleT>
struct ParseHelper<TupleT> {
    static bool ParseArg(ConsoleStream& stream, Console::ArgIterator& iter, TupleT& tp) {
        return true;
    }
};

class CommandSet {
private:
    using ExecFn = bool(*)(ConsoleStream&, Console::ArgIterator&, void*);

    template<typename... Args>
    struct Executor {
        using TupleT = std::tuple<typename std::decay<Args>::type...>;

        template<std::size_t... Is>
        static bool ParseArgs(ConsoleStream& stream, Console::ArgIterator& iter, TupleT& tp, std::index_sequence<Is...>) {
            return ParseHelper<TupleT, Is...>::ParseArg(stream, iter, tp);
        }

        static bool Exec(ConsoleStream& stream, Console::ArgIterator& iter, void* handler) {
            const auto& funcPtr = reinterpret_cast<void (*)(Args...)>(handler);
            TupleT args;

            if (ParseArgs(stream, iter, args, std::make_index_sequence<sizeof...(Args)>()) == false) return false;

            std::apply(funcPtr, args);
            return true;
        }
    };

    struct Command {
        std::string_view description;

        unsigned int argsNumber = 0;

        void* handler = nullptr;
        ExecFn executor = nullptr;
    };

    std::unordered_map<std::string_view, Command> commandsMap;

public:
    inline auto begin() const { return commandsMap.cbegin(); }
    inline auto end() const { return commandsMap.cend(); }

    inline bool HasCommand(std::string_view name) const {
        return commandsMap.count(name);
    }

    inline const Command& GetCommand(std::string_view name) const {
        return commandsMap.at(name);
    }
    inline const Command* TryGetCommand(std::string_view name) const {
        const auto iter = commandsMap.find(name);

        if (iter == commandsMap.end()) return nullptr;
        return &iter->second;
    }

    template<typename... Args>
    inline void RegisterCommand(const char* name, const char* description, void (*handler)(Args...)) {
        Command command;
        command.description = description;
        command.handler = reinterpret_cast<void*>(handler);
        command.argsNumber = sizeof...(Args);
        command.executor = &Executor<Args...>::Exec;

        commandsMap[name] = command;
    }
};

#endif