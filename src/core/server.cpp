#include "server.h"

using namespace Net;

static bool SocketOpenAndBind(Socket& socket, const Address& address) {
    if (!socket.Open(address.GetFamily(), Protocol::TCP)) return false;
    socket.SetOption<int>(Socket::Option::ReuseAddress, true);

    if (!socket.Bind(address)) {
        socket.Close();
        return false;
    }

    return true;
}

bool TcpServer::Bind(const Address& address) {
    return SocketOpenAndBind(socket, address);
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
    return SocketOpenAndBind(socket, address);
}

Ptr<Connection> UdpServer::Listen() {
    Ptr<ClientConnection> clientConnection = std::make_unique<ClientConnection>();
    char connectBuffer[sizeof(CONNECT_MAGIC)] = { 0 };

    socket.ReceiveFrom(connectBuffer, sizeof(connectBuffer), clientConnection->clientAddress, Socket::WaitAll);
    if (strcmp(connectBuffer, CONNECT_MAGIC) != 0) return nullptr;

    clientConnection->serverSocket = &socket;
    if (!clientConnection->Send(ACCEPT_MAGIC, sizeof(ACCEPT_MAGIC))) return nullptr;

    return clientConnection;
}