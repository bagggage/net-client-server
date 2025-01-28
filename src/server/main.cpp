#include "utils.h"
#include "socket.h"

#include <iostream>

#define SERVER_PORT 8088

int main() {
    Net::Address bindAddress = Net::Address::MakeBind(SERVER_PORT);
    LIBPOG_ASSERT(bindAddress.IsValid(), "Bind address must be valid");

    Net::Socket serverSock(Net::Address::Family::IPv4);
    LIBPOG_ASSERT(serverSock.IsOpen(), "Socket must be open");

    Net::Address::port_t port = serverSock.Listen(bindAddress);
    if (serverSock.IsListening() == false) {
        std::cerr << "Socket is not listening. Exit." << std::endl;
        return -1;
    }

    std::cout << "Socket is listening on port: " << port << std::endl;

    Net::Socket sock = serverSock.Accept(bindAddress);

    char buffer[16] = {0};
    if (uint num = sock.Receive(buffer, sizeof(buffer))) {
        std::cout << "Received " << num << " bytes: \"" << &buffer[0] << "\"." << std::endl;
    } else {
        std::cerr << "Failed to receive something." << std::endl;
    }

    serverSock.Close();
    LIBPOG_ASSERT(serverSock.GetState() == Net::Socket::State::None, "Socket must be in `None` state");
    LIBPOG_ASSERT(!serverSock.IsOpen(), "Socket must be closed");

    std::cout << "Done." << std::endl;
    return 0;
}
