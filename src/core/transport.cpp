#include "transport.h"

#include "socket.h"

#include <iostream>

using namespace Net;

Ptr<Transport> TcpTransport::Listen(const Net::Address& address) {
    if (socket.IsListening() == false) {
        if (!socket.IsOpen()) {
            if (!socket.Open(Address::Family::IPv4, Protocol::TCP)) return nullptr;
        }

        socket.SetOption(Net::Socket::Option::ReuseAddress, true);
        socket.Listen(address);
        if (socket.IsListening() == false) return nullptr;
    }

    Socket accepted = socket.Accept();
    if (accepted.IsValid() == false) return nullptr;

    return std::move(std::make_unique<TcpTransport>(accepted));
}

bool TcpTransport::Connect(const Net::Address& address) {
    if (socket.IsOpen()) socket.Close();
    if (!socket.Open(Address::Family::IPv4, Protocol::TCP)) return false;

    return socket.Connect(address);
}

void TcpTransport::Disconnect() {
    if (socket.IsOpen()) socket.Close();
}

uint TcpTransport::Send(const void* data, const uint32_t size) {
    return socket.Send(reinterpret_cast<const char*>(data), size, Net::Socket::NoSignal);
}

uint TcpTransport::Receive(void* buffer, const uint32_t size) {
    return socket.Receive(reinterpret_cast<char*>(buffer), size, Net::Socket::WaitAll);
}

static constexpr uint UDP_TIMEOUT = 5;

Ptr<Transport> UdpTransport::Listen(const Net::Address& address) {
    if (!socket.IsOpen()) {
        if (!socket.Open(Address::Family::IPv4, Protocol::UDP)) return nullptr;
        if (!socket.Bind(address)) {
            socket.Close();
            return nullptr;
        }
    }

    Address remoteAddress;

    char buffer[sizeof(CONNECT_MAGIC)] = { 0 };
    if (socket.ReceiveFrom(buffer, sizeof(buffer), remoteAddress, Socket::WaitAll) != sizeof(CONNECT_MAGIC)) return nullptr;
    if (strcmp(buffer, CONNECT_MAGIC) != 0) return nullptr;

    UdpTransport* accepted = new UdpTransport();
    accepted->remoteAddress = remoteAddress;
    accepted->receiveSocket = &socket;

    std::cout << "Accepting...\n";
    if (socket.SendTo(remoteAddress, ACCEPT_MAGIC, sizeof(ACCEPT_MAGIC)) != sizeof(ACCEPT_MAGIC)) {
        delete accepted;
        return nullptr;
    }

    return std::move(Ptr<UdpTransport>(accepted));
}

bool UdpTransport::Connect(const Net::Address& address) {
    this->remoteAddress = address;

    if (socket.IsOpen()) socket.Close();

    if (!socket.Open(Address::Family::IPv4, Protocol::UDP)) return false;
    if (!socket.SetOption<timeval>(Socket::Option::ReceiveTimeout, { UDP_TIMEOUT, 0 })) return false;

    socket.SendTo(address, CONNECT_MAGIC, sizeof(CONNECT_MAGIC));

    char dummyBuffer[sizeof(ACCEPT_MAGIC)] = { 0 };
    const uint received = socket.Receive(dummyBuffer, sizeof(dummyBuffer), Net::Socket::WaitAll);

    if (
        received != sizeof(dummyBuffer) ||
        dummyBuffer[sizeof(ACCEPT_MAGIC) - 1] != '\0' ||
        strcmp(dummyBuffer, ACCEPT_MAGIC) != 0
    ) {
        socket.Close();
        return false;
    }

    sendIdx = 0;
    receiveIdx = 0;

    return true;
}

void UdpTransport::Disconnect() {
    if (socket.IsOpen()) socket.Close();
}

static constexpr uint32_t SIZE_ALIGN = 4;

uint UdpTransport::Send(const void* data, const uint32_t size) {
    uint32_t sendSize = (size + (SIZE_ALIGN - 1)) & -SIZE_ALIGN;
    if (RawSend(reinterpret_cast<const char*>(data), sendSize) != sendSize) return 0;
    return size;
}

uint UdpTransport::RawSend(const void* buffer, const uint32_t size, Net::Socket::Flags flags) {
    //std::cout << "send: " << size << ".\n";

    if (receiveSocket == nullptr) {
        return socket.SendTo(remoteAddress, reinterpret_cast<const char*>(buffer), size, flags);
    }
    return receiveSocket->SendTo(remoteAddress, reinterpret_cast<const char*>(buffer), size, flags);
}

uint UdpTransport::RawReceive(void* buffer, const uint32_t size, Net::Socket::Flags flags) {
    //std::cout << "receive: " << size << ".\n";

    if (receiveSocket == nullptr) {
        return socket.Receive(reinterpret_cast<char*>(buffer), size, flags);
    }
    return receiveSocket->Receive(reinterpret_cast<char*>(buffer), size, flags);
}

uint UdpTransport::Receive(void* buffer, const uint32_t size) {
    uint32_t sendSize = (size + (SIZE_ALIGN - 1)) & -SIZE_ALIGN;
    if (RawReceive(buffer, sendSize, Net::Socket::WaitAll) != sendSize) return 0;
    return size;
}

void UdpTransport::SetupTimeout() {
    if (receiveSocket)
        receiveSocket->SetOption<timeval>(Socket::Option::ReceiveTimeout, { UDP_TIMEOUT, 0 });
    else
        socket.SetOption<timeval>(Socket::Option::ReceiveTimeout, { UDP_TIMEOUT, 0 });
}

void UdpTransport::DropTimeout() {
    if (receiveSocket)
        receiveSocket->SetOption<timeval>(Socket::Option::ReceiveTimeout, { 0, 0 });
    else
        socket.SetOption<timeval>(Socket::Option::ReceiveTimeout, { 0, 0 });
}