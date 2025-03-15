#ifndef _NET_SERVER_H
#define _NET_SERVER_H

#include "socket.h"
#include "connection.h"

#include <memory>
#include <vector>

#include <poll.h>

namespace Net {
    template<typename T>
    using Ptr = std::unique_ptr<T>;

    class Server {
    public:
        virtual ~Server() = default;

        virtual bool Bind(const Address& address) = 0;
        virtual Connection* Listen() = 0;

        virtual Status Fail() = 0;
    };

    class TcpServer final : public Server {
        std::vector<Connection*> clientConnections;
        std::vector<pollfd> pollSet;

        Socket socket;

        Connection* Accept();
        void RemoveClient(const uint client_idx);

    public:
        bool Bind(const Address& address) override;
        Connection* Listen() override;

        Status Fail() override { return socket.Fail(); }
    };
}

#endif