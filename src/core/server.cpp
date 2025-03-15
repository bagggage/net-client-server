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

    clientConnections.push_back(connection);

    pollfd clientFd;
    clientFd.fd = connection->socket.GetOsSocket();
    clientFd.events = POLLIN | POLLHUP;

    pollSet.push_back(clientFd);

    return connection;
}

void TcpServer::RemoveClient(const uint client_idx) {
    clientConnections.erase(clientConnections.begin() + client_idx);
    pollSet.erase(pollSet.begin() + client_idx + 1);
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
            else if (revents & POLLHUP) {
                result = clientConnections[client_idx];
                RemoveClient(client_idx);

                return result;
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
