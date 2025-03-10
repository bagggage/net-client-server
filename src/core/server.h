#ifndef _NET_SERVER_H
#define _NET_SERVER_H

#include "socket.h"
#include "connection.h"

#include <memory>

namespace Net {
    template<typename T>
    using Ptr = std::unique_ptr<T>;

    class Server {
    public:
        virtual ~Server() = default;

        virtual bool Bind(const Address& address) = 0;
        virtual Ptr<Connection> Listen() = 0;

        virtual Status Fail() = 0;
    };

    class TcpServer final : public Server {
        Socket socket;

    public:
        bool Bind(const Address& address) override;
        Ptr<Connection> Listen() override;

        Status Fail() override { return socket.Fail(); }
    };

    class UdpServer final : public Server {
        static constexpr const char CONNECT_MAGIC[] = "connect";
        static constexpr const char ACCEPT_MAGIC[]  = "accept_";
        static constexpr const char CLOSE_MAGIC[]   = "disconn";

        Socket socket;

        class ClientConnection final : public Connection {
            Address clientAddress;
            Socket* serverSocket = nullptr;

            friend class UdpServer;

        public:
            bool Send(const void* buffer, const unsigned int size) override {
                return serverSocket->SendTo(clientAddress, reinterpret_cast<const char*>(buffer), size) > 0;
            }

            bool Receive(void* buffer, const unsigned int size) override {
                return serverSocket->Receive(reinterpret_cast<char*>(buffer), size, Net::Socket::WaitAll) > 0;
            }

            Status Fail() override { return serverSocket->Fail(); }
        };

        friend class UdpClient;

    public:
        bool Bind(const Address& address) override;
        Ptr<Connection> Listen() override;

        Status Fail() override { return socket.Fail(); }
    };
}

#endif