#include "utils.h"
#include "socket.h"

#include <iostream>
#include <string>
#include <ctime>

#define SERVER_PORT 8088

void HandleCommand(const std::string& command, Net::Socket& clientSock) {
    if (command == "ECHO") {
        char buffer[256] = {0};
        uint num = clientSock.Receive(buffer, sizeof(buffer));
        if (num > 0) {
            std::string echoData(buffer, num);
            std::cout << "ECHO data received: " << echoData << std::endl;
            clientSock.Send(echoData.c_str(), echoData.size());
        } else {
            std::cerr << "Failed to receive ECHO data." << std::endl;
        }
    } else if (command == "TIME") {
        auto now = time(nullptr);
        std::string response = std::ctime(&now);
        clientSock.Send(response.c_str(), response.size());
    } else {
        std::string response = "Unknown command";
        clientSock.Send(response.c_str(), response.size());
    }
}

void HandleClient(Net::Socket& clientSock) {
    while (true) {
        char buffer[256] = {0};
        uint num = clientSock.Receive(buffer, sizeof(buffer));
        if (num > 0) {
            std::string command(buffer, num);
            std::cout << "Received command: " << command << std::endl;

            HandleCommand(command, clientSock);
        } else {
            std::cerr << "Failed to receive data or connection closed." << std::endl;
            break;
        }
    }

    clientSock.Close();
    std::cout << "Client connection closed." << std::endl;
}

int main() {
    Net::Address bindAddress = Net::Address::MakeBind(SERVER_PORT);
    LIBPOG_ASSERT(bindAddress.IsValid(), "Bind address must be valid");

    Net::Socket serverSock(Net::Address::Family::IPv4);
    LIBPOG_ASSERT(serverSock.IsOpen(), "Socket must be open");

    Net::Address::port_t port = serverSock.Listen(bindAddress);
    if (!serverSock.IsListening()) {
        std::cerr << "Socket is not listening. Exit." << std::endl;
        return -1;
    }

    std::cout << "Socket is listening on port: " << port << std::endl;

    while (true) {
        std::cout << "Waiting for a new connection..." << std::endl;
        Net::Socket clientSock = serverSock.Accept(bindAddress);

        if (clientSock.IsOpen()) {
            std::cout << "Client connected." << std::endl;
            HandleClient(clientSock);
        } else {
            std::cerr << "Failed to accept a client connection." << std::endl;
        }
    }

    serverSock.Close();

    LIBPOG_ASSERT(serverSock.GetState() == Net::Socket::State::None, "Socket must be in None state");
    LIBPOG_ASSERT(!serverSock.IsOpen(), "Socket must be closed");

    std::cout << "Server shut down. Goodbye!" << std::endl;
    return 0;
}