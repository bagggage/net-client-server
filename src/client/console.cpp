#include "console.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <iostream>

std::string StdIoConsoleStream::ReadString() {
    std::string result;
    std::getline(std::cin, result);

    return std::move(result);
}

void StdIoConsoleStream::Write(std::string_view string) {
    std::cout << string;
}

std::string_view Console::ArgIterator::Next() {
    if (bufferView.empty()) return {};

    unsigned int i = 0;
    while (std::isspace(bufferView[i])) i++;

    if (i == bufferView.length()) {
        bufferView = {};
        return bufferView;
    }

    bufferView = bufferView.substr(i);
    i = 0;

    while (std::isspace(bufferView[i]) == false && i < bufferView.length()) i++;

    auto result = bufferView.substr(0, i);
    bufferView = bufferView.substr(i);

    return result;
}

void Console::PrintHelp() {
    stream->Write("Help:\n");

    for (const auto& command : *commandSet) {
        stream->Write("  ");
        stream->Write(command.first);
        stream->Write("\t");
        stream->Write(command.second.description);
        stream->Write(".\n");
    }

    stream->Write("\n"
        "  help, h\tPrints this help.\n\n"
        "  quit, q\tExit from console.\n"
        "  exit\n"
    );
}

bool Console::HandleCommand() {
    stream->Write("> ");
    const std::string line = stream->ReadString();

    ArgIterator iter(line);
    const auto commandStr = iter.Next();

    if (commandStr.empty()) return false;
    if (commandStr == "help" || commandStr == "h") {
        PrintHelp();
        return true;
    } else if (commandStr == "exit" || commandStr == "quit" || commandStr == "q") [[unlikely]] {
        isShouldExit = true;
        return true;
    }

    const auto* command = commandSet->TryGetCommand(commandStr);
    if (command == nullptr) {
        stream->Write("Unknown command \"");
        stream->Write(commandStr);
        stream->Write("\". Use 'help' or 'h' to see list of available commands.\n");
        return false;
    }

    return command->executor(*stream, iter, command->handler);
}