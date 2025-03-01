#include "client.h"

#include "server.h"

using namespace Net;

SocketConnection TcpClient::Connect(const Address& address) {
    SocketConnection connection;
    if (!connection.socket.Open(address.GetFamily(), Protocol::TCP)) return connection;
    
    connection.socket.Connect(address);
    return connection;
}

SocketConnection UdpClient::Connect(const Address& address) {
    SocketConnection connection;
    if (!connection.socket.Open(address.GetFamily(), Protocol::UDP)) return {};
    if (!connection.socket.Connect(address)) return {};
    if (!connection.Send(UdpServer::CONNECT_MAGIC, sizeof(UdpServer::CONNECT_MAGIC))) return {};

    char acceptBuffer[sizeof(UdpServer::ACCEPT_MAGIC)] = { 0 };
    connection.Receive(acceptBuffer, sizeof(acceptBuffer));

    if (strcmp(acceptBuffer, UdpServer::ACCEPT_MAGIC) != 0) return {};
    return connection;
}
