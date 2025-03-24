#include "client.h"

#include "server.h"

#include <iostream>

using namespace Net;

Ptr<Connection> TcpClient::Connect(const Address& address) {
    Ptr<SocketConnection> connection = std::make_unique<SocketConnection>();
    if (!connection->socket.Open(address.GetFamily(), Protocol::TCP)) return connection;
    if (!connection->socket.Connect(address)) return connection;

    return std::move(connection);
}

Ptr<Connection> UdpClient::Connect(const Address& address) {
    Ptr<SocketConnection> connection = std::make_unique<SocketConnection>();
    if (!connection->socket.Open(address.GetFamily(), Protocol::UDP)) return nullptr;
    if (!connection->socket.Connect(address)) return nullptr;
    if (!connection->Send(UdpServer::CONNECT_MAGIC, sizeof(UdpServer::CONNECT_MAGIC))) return nullptr;

    Address::port_t port;
    if (!connection->ReceiveAll(&port, sizeof(port))) return nullptr;

    std::cout << "Remoute port: " << port << ".\n";

    Address newAddress = address;
    newAddress.SetPort(port);

    if (!connection->socket.Connect(newAddress)) return nullptr;

    return std::move(connection);
}
