#include "utils.h"
#include "socket.h"

#define DEFAULT_PORT 8088
#define DEFAULT_ADDRESS "127.0.0.1"

int main() {
    std::string address;
    int port = DEFAULT_PORT;

    std::cout << "address[" << DEFAULT_ADDRESS << "]: ";
    std::getline(std::cin, address);
    if (address.empty()) {
        address = DEFAULT_ADDRESS;
    }

    std::cout << "port[" << DEFAULT_PORT << "]: ";
    std::string portInput;
    std::getline(std::cin, portInput);
    if (!portInput.empty()) {
        try {
            port = std::stoi(portInput);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port input. Using default port: " << DEFAULT_PORT << std::endl;
            port = DEFAULT_PORT;
        }
    }

    const Net::Address serverAddress = Net::Address::FromString(address.c_str(), port);
    std::cout << "Server address: " << serverAddress.ConvertToString() << std::endl;

    Net::Socket clientSock(Net::Address::Family::IPv4, Net::Protocol::TCP);
    LIBPOG_ASSERT(clientSock.IsOpen(), "Socket must be open");

    bool result = clientSock.Connect(serverAddress);
    LIBPOG_ASSERT(result == clientSock.IsConnected(), "Socket internal state must be the same");

    if (!clientSock.IsConnected()) {
        std::cerr << "Socket is not connected. Exit." << std::endl;
        return -1;
    }

    std::cout << "Client connected." << std::endl;

    std::string userInput;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, userInput);

        if (userInput == "CLOSE") {
            break;
        } else if (userInput == "ECHO") {
            if (!clientSock.Send("ECHO")) {
                std::cerr << "Failed to send ECHO command." << std::endl;
                break;
            }

            std::cout << "Enter data to echo: ";
            std::string echoData;
            std::getline(std::cin, echoData);

            if (!clientSock.Send(echoData)) {
                std::cerr << "Failed to send ECHO data." << std::endl;
                break;
            }

            char buffer[256] = {0};
            uint num = clientSock.Receive(buffer, sizeof(buffer));
            if (num > 0) {
                std::cout << "Echoed back: " << std::string(buffer, num) << std::endl;
            } else {
                std::cerr << "Failed to receive echoed data." << std::endl;
                break;
            }
        } else if (userInput == "TIME") {
            if (!clientSock.Send("TIME")) {
                std::cerr << "Failed to send TIME command." << std::endl;
                break;
            }

            char buffer[256] = {0};
            uint num = clientSock.Receive(buffer, sizeof(buffer));
            if (num > 0) {
                std::cout << "Server time: " << std::string(buffer, num) << std::endl;
            } else {
                std::cerr << "Failed to receive time data." << std::endl;
                break;
            }
        } else {
            if (!clientSock.Send(userInput)) {
                std::cerr << "Failed to send message." << std::endl;
                break;
            }

            char buffer[256] = {0};
            uint num = clientSock.Receive(buffer, sizeof(buffer));
            if (num > 0) {
                std::cout << std::string(buffer, num) << std::endl;
            } else {
                std::cerr << "Failed to receive data." << std::endl;
                break;
            }
        }
    }

    clientSock.Close();

    LIBPOG_ASSERT(!clientSock.IsConnected(), "Socket must be disconnected");
    LIBPOG_ASSERT(!clientSock.IsOpen(), "Socket must be closed");

    std::cout << "Connection closed. Goodbye!" << std::endl;
    return 0;
}