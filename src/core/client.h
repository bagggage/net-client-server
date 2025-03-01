#ifndef _NET_CLIENT_H
#define _NET_CLIENT_H

#include "connection.h"

namespace Net {
    class TcpClient {
    public:
        static SocketConnection Connect(const Address& address);
    };

    class UdpClient {
    public:
        static SocketConnection Connect(const Address& address);
    };

    class Client {
    public:
        static SocketConnection Connect(const Address& address, const Protocol protocol = Protocol::TCP) {
            if (protocol == Protocol::TCP) return TcpClient::Connect(address);
            return UdpClient::Connect(address);
        }
    };
}

#endif