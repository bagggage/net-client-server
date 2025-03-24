#include "server.h"

#include <iostream>

using namespace Net;

static bool SocketOpenAndBind(Socket& socket, const Address& address, const Net::Protocol protocol) {
    if (!socket.Open(address.GetFamily(), protocol)) {
        return false;
    }

    socket.SetOption<int>(Socket::Option::ReuseAddress, true);

    if (!socket.Bind(address)) {
        socket.Close();
        return false;
    }

    return true;
}

bool TcpServer::Bind(const Address& address) {
    return SocketOpenAndBind(socket, address, Protocol::TCP);
}

Ptr<Connection> TcpServer::Listen() {
    if (!socket.IsListening()) {
        if (!socket.Listen()) return nullptr;
    }

    Socket clientSocket = socket.Accept();
    if (!clientSocket.IsValid()) return nullptr;

    Ptr<SocketConnection> connection = std::make_unique<SocketConnection>();
    connection->socket = std::move(clientSocket);

    return connection;
}

bool UdpServer::Bind(const Address& address) {
    return SocketOpenAndBind(socket, address, Protocol::UDP);
}

Ptr<Connection> UdpServer::Listen() {
    Ptr<SocketConnection> clientConnection = std::make_unique<SocketConnection>();
    if (!clientConnection->socket.Open(Address::Family::IPv4, Protocol::UDP)) return nullptr;

    char connectBuffer[sizeof(CONNECT_MAGIC)] = { 0 };
    Address clientAddress;

    socket.ReceiveFrom(connectBuffer, sizeof(connectBuffer), clientAddress, Socket::WaitAll);
    if (strcmp(connectBuffer, CONNECT_MAGIC) != 0) return nullptr;

    if (!clientConnection->socket.Bind(Address::MakeBind(0, Protocol::UDP))) return nullptr;
    if (!clientConnection->socket.Connect(clientAddress)) return nullptr;

    const Address endpointAddress = clientConnection->socket.GetAddress();
    const Address::port_t port = endpointAddress.GetPort();

    std::cout << "endpoint port: " << port << ".\n";
    if (!socket.SendTo(clientAddress, reinterpret_cast<const char*>(&port), sizeof(port))) return nullptr;

    return clientConnection;
}