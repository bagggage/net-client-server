#ifndef _NET_CONNECTION_H
#define _NET_CONNECTION_H

#include "socket.h"

namespace Net {
    class Connection {
        virtual void Close() = 0;
    public:
        virtual ~Connection() { Close(); }

        virtual bool Send(const void* buffer, const unsigned int size) = 0;
        virtual bool Receive(void* buffer, const unsigned int size) = 0;

        template<typename T>
        bool Send(const T& object) {
            return Send(reinterpret_cast<const void*>(&object), sizeof(T));
        }

        template<typename T>
        bool Receive(T& object) {
            return Receive(reinterpret_cast<void*>(&object), sizeof(T));
        }
    };

    class SocketConnection final : public Connection {
        Net::Socket socket;

        void Close() override { socket.Close(); }

        friend class TcpClient;
        friend class UdpClient;
        friend class TcpServer;
    public:
        bool Send(const void* buffer, const unsigned int size) override {
            return socket.Send(reinterpret_cast<const char*>(buffer), size) > 0;
        }

        bool Receive(void* buffer, const unsigned int size) override {
            return socket.Receive(reinterpret_cast<char*>(buffer), size, Net::Socket::WaitAll) > 0;
        }
    };
}

#endif