#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include <cstddef>
#include <cinttypes>
#include <vector>
#include <memory>

#include "socket.h"

namespace Net {
    template<typename T>
    using Ptr = std::unique_ptr<T>;

    class Transport {
    public:
        virtual ~Transport() { Disconnect(); }

        virtual Ptr<Transport> Listen(const Net::Address& address) = 0;

        virtual bool Connect(const Net::Address& address) = 0;
        virtual void Disconnect() {}

        virtual uint Send(const void* data, const uint32_t size) = 0;
        virtual uint Receive(void* buffer, const uint32_t size) = 0;

        template<typename T>
        uint Send(const T& object) {
            return Send(reinterpret_cast<const void*>(&object), sizeof(T));
        }

        template<typename T>
        uint Receive(T& object) {
            return Receive(reinterpret_cast<void*>(&object), sizeof(T));
        }

        virtual Status GetStatus() = 0;
        virtual Status Fail() = 0;
    };

    class TcpTransport final : public Transport {
    private:
        Socket socket;

    public:
        TcpTransport() = default;
        TcpTransport(Net::Socket& socket) : socket(std::move(socket)) {}

        Ptr<Transport> Listen(const Net::Address& address) override;

        bool Connect(const Net::Address& address) override;
        void Disconnect() override;

        uint Send(const void* data, const uint32_t size) override;
        uint Receive(void* buffer, const uint32_t size) override;

        Status GetStatus() override { return socket.GetStatus(); }
        Status Fail() override { return socket.Fail(); }
    };

    class UdpTransport final : public Transport {
    private:
        Socket socket = Socket();
        Socket* receiveSocket = nullptr;

        Address remoteAddress;
        size_t receiveIdx = 0;
        size_t sendIdx = 0;

        static constexpr const char CONNECT_MAGIC[] = "connect";
        static constexpr const char ACCEPT_MAGIC[] = "accept_";

        uint RawReceive(void* buffer, const uint32_t size, Net::Socket::Flags flags = Net::Socket::Flags::None);
        uint RawSend(const void* buffer, const uint32_t size, Net::Socket::Flags flags = Net::Socket::Flags::None);

    public:
        UdpTransport() = default;
        UdpTransport(Net::Socket& socket) : socket(std::move(socket)) {}

        Ptr<Transport> Listen(const Net::Address& address) override;

        bool Connect(const Net::Address& address) override;
        void Disconnect() override;

        uint Send(const void* data, const uint32_t size) override;
        uint Receive(void* buffer, const uint32_t size) override;

        Status GetStatus() override { return socket.GetStatus(); }
        Status Fail() override { return socket.Fail(); }
    };
}

#endif