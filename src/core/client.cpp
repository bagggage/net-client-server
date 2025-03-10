#include "client.h"

#include "server.h"

using namespace Net;

Ptr<Connection> TcpClient::Connect(const Address& address) {
    Ptr<SocketConnection> connection = std::make_unique<SocketConnection>();
    if (!connection->socket.Open(address.GetFamily(), Protocol::TCP)) connection;
    if (!connection->socket.Connect(address)) connection;

    return std::move(connection);
}

Ptr<Connection> UdpClient::Connect(const Address& address) {
    Ptr<SocketConnection> connection = std::make_unique<SocketConnection>();
    if (!connection->socket.Open(address.GetFamily(), Protocol::UDP)) return nullptr;
    if (!connection->socket.Connect(address)) return nullptr;
    if (!connection->Send(UdpServer::CONNECT_MAGIC, sizeof(UdpServer::CONNECT_MAGIC))) return nullptr;

    char acceptBuffer[sizeof(UdpServer::ACCEPT_MAGIC)] = { 0 };
    if (!connection->Receive(acceptBuffer, sizeof(acceptBuffer))) return nullptr;
    if (strcmp(acceptBuffer, UdpServer::ACCEPT_MAGIC) != 0) return nullptr;

    return std::move(connection);
}
