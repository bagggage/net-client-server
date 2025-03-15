#include "client.h"

#include "server.h"

using namespace Net;

Ptr<Connection> TcpClient::Connect(const Address& address) {
    Ptr<SocketConnection> connection = std::make_unique<SocketConnection>();
    if (!connection->socket.Open(address.GetFamily(), Protocol::TCP)) connection;
    if (!connection->socket.Connect(address)) connection;

    return std::move(connection);
}
