#include <iostream>
#include <filesystem>
#include <future>

#include <core/args.h>
#include <core/message.h>
#include <core/socket.h>

#include "server.h"

struct ServerConfig {
    Net::Address::port_t port = Msg::DEFAULT_SERVER_PORT;
    Net::Protocol protocol = Net::Protocol::TCP;
    const char* hostFilesDirectory = Server::DEFAULT_FILES_DIR;
};

static void PrintHelp() {
    std::cout <<
        "Usage: server [options]\n"
        "  -port <port>\tListen port number.\n"
        "  -p\n"
        "  -dir <path>\tSpecify directory to host.\n"
        "  -d\n"
        "  -udp\tStart server over UDP protocol.\n"
        "  -help\tShow this help.\n"
        "  -h\n";
    ;

    exit(EXIT_SUCCESS);
}

static bool ParseServerArgs(int argc, const char** argv, ServerConfig& outConfig) {
    ArgIterator argIter(argc, argv);
    bool result = true;
    bool printHelp = false;

    while (argIter.Next()) {
        if (argIter.IsValue()) [[unlikely]] {
            std::cerr << "Unexpected argument: \"" << argIter.Get() << "\"\n";
        } else {
            const auto value = argIter.GetAsOption();

            if (value == "port" || value == "p") {
                result &= RequireArgParameter<Net::Address::port_t>(
                    argIter,
                    "Expected port number: -port, p <port number>.",
                    outConfig.port
                );
            } else if (value == "dir" || value == "d") {
                result &= RequireArgParameter<const char*>(
                    argIter,
                    "Expected host directory: -dir, d <directory path>.",
                    outConfig.hostFilesDirectory
                );
            } if (value == "udp") {
                outConfig.protocol = Net::Protocol::UDP;
            } else if (value == "help" || value == "h") {
                printHelp = true;
            } else {
                std::cerr << "Unknown argument: \"" << argIter.Get() << "\", use \"-help\" to see list of arguments.\n";
            }
        }
    }

    if (printHelp) PrintHelp();

    return result;
}

void ClientHandleLoop(Server* server, int clientIndex) {
    while (server->Handle(clientIndex) != false);

    std::cout << "Client propably disconnected.\n";
}

int main(int argc, const char** argv) {
    // Read command line arguments
    ServerConfig config;
    if (ParseServerArgs(argc, argv, config) == false) [[unlikely]] {
        std::cerr << "Incorrect input.\n";
        return EXIT_FAILURE;
    }

    if (std::filesystem::exists(config.hostFilesDirectory) == false) {
        std::cout << "There is no host directory: '" << config.hostFilesDirectory << "'.\n";

        try {
            std::filesystem::create_directories(config.hostFilesDirectory);
        }
        catch (const std::filesystem::filesystem_error err) {
            std::cerr << "Cannot create host directory: " << err.what() << '\n';
            return EXIT_FAILURE;
        };

        std::cout << "Host directory was succefully created.\n";
    } else if (std::filesystem::is_directory(config.hostFilesDirectory) == false) {
        std::cerr << config.hostFilesDirectory << ": is not a directory.";
        return EXIT_FAILURE;
    }

    std::vector<std::thread> threads;

    Server server(config.protocol, config.port);
    server.SetHostDirectory(config.hostFilesDirectory);

    if (Net::Status fail = server.Fail()) [[unlikely]] {
        std::cerr << "Failed to startup server: " << Net::GetStatusName(fail) << ".\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server listening at port: " << config.port << ".\n";

    while(true) {
        const int clientIndex = server.Listen();
        if (clientIndex == Server::INVALID_CLIENT_INDEX) {
            std::cout << "Listen failed: " << Net::GetStatusName(server.Fail()) << ".\n";
            continue;
        }

        threads.push_back(std::thread(ClientHandleLoop, &server, clientIndex));
    }

    return EXIT_SUCCESS;
}