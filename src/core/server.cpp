#include "server.h"

#include <algorithm>

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
    if (!SocketOpenAndBind(socket, address)) return false;

    pollfd serverSocket;
    serverSocket.fd = socket.GetOsSocket();
    serverSocket.events = POLLIN;

    pollSet.push_back(serverSocket);

    return true;
}

Connection* TcpServer::Accept() {
    Socket clientSocket = socket.Accept();
    if (!clientSocket.IsValid()) return nullptr;

    SocketConnection* connection = new SocketConnection();
    connection->socket = std::move(clientSocket);

    return connection;
}

void TcpServer::RemoveClient(const uint client_idx) {
    clientConnections.erase(clientConnections.begin() + client_idx);
    pollSet.erase(pollSet.begin() + client_idx + 1);
}

void TcpServer::AddToListen(Connection* clientConnection) {
    const auto sockConnection = reinterpret_cast<SocketConnection*>(clientConnection);

    clientConnections.push_back(clientConnection);

    pollfd clientFd;
    clientFd.fd = sockConnection->socket.GetOsSocket();
    clientFd.events = POLLIN | POLLHUP;

    pollSet.push_back(clientFd);
}

void TcpServer::RemoveFromListening(Connection* clientConnection) {
    const auto iter = std::find(clientConnections.begin(), clientConnections.end(), clientConnection);
    const auto index = iter - clientConnections.begin();

    RemoveClient(index);
}

Connection* TcpServer::Listen() {
    static size_t last_poll = pollSet.size();

    while (true) {
        Connection* result = nullptr;
        for (; last_poll < pollSet.size(); ++last_poll) {
            const auto client_idx = last_poll - 1;
            const auto revents = pollSet[last_poll].revents;

            if (revents & POLLIN) {
                if (last_poll == 0) {
                    result = Accept();
                } else {
                    result = clientConnections[client_idx];
                }
            }
            else {
                continue;
            }

            last_poll++;
            return result;
        }

        if (!socket.IsListening()) {
            if (!socket.Listen()) return nullptr;
        }

        last_poll = 0;
        if (poll(pollSet.data(), pollSet.size(), -1) < 0) return nullptr;
    }
}
