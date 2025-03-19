#ifndef _NET_CONNECTION_H
#define _NET_CONNECTION_H

#include "socket.h"

namespace Net {
    class Connection {
        virtual void Close() {}
    public:
        virtual ~Connection() { Close(); }

        virtual uint Send(const void* buffer, const unsigned int size) = 0;
        virtual uint Receive(void* buffer, const unsigned int size) = 0;
        virtual uint ReceiveAll(void* buffer, const unsigned int size) = 0;

        virtual Status Fail() = 0;

        template<typename T>
        uint Send(const T& object) {
            return Send(reinterpret_cast<const void*>(&object), sizeof(T));
        }

        template<typename T>
        uint Receive(T& object) {
            return Receive(reinterpret_cast<void*>(&object), sizeof(T));
        }

        template<typename T>
        uint ReceiveAll(T& object) {
            return ReceiveAll(reinterpret_cast<void*>(&object), sizeof(T));
        }
    };

    class SocketConnection final : public Connection {
        Net::Socket socket;

        void Close() override { socket.Close(); }

        friend class TcpClient;
        friend class UdpClient;
        friend class TcpServer;
    public:
        uint Send(const void* buffer, const unsigned int size) override {
            return socket.Send(reinterpret_cast<const char*>(buffer), size);
        }

        uint Receive(void* buffer, const unsigned int size) override {
            return socket.Receive(reinterpret_cast<char*>(buffer), size);
        }

        uint ReceiveAll(void* buffer, const unsigned int size) override {
            return socket.Receive(reinterpret_cast<char*>(buffer), size, Net::Socket::WaitAll);
        }

        Status Fail() override { return socket.Fail(); }
    };
}

#endif