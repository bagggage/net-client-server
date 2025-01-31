#include "client.h"
#include "packet.h"

Client& Client::GetInstance() {
    static Client instance;
    return instance;
}

Client::Client() {}

Client::~Client() {}

bool Client::Connect(const std::string& address, unsigned short port) {
    serverAddress = Net::Address::FromString(address.c_str(), port);
    std::cout << "Server address: " << serverAddress.ConvertToString() << std::endl;

    clientSock.Open(Net::Address::Family::IPv4, Net::Protocol::TCP);
    LIBPOG_ASSERT(clientSock.IsOpen(), "Socket must be open");

    bool result = clientSock.Connect(serverAddress);
    LIBPOG_ASSERT(result == clientSock.IsConnected(), "Socket internal state must be the same");

    if (!clientSock.IsConnected()) {
        std::cerr << "Socket is not connected. Exit." << std::endl;
        return false;
    }

    std::cout << "Client connected." << std::endl;
    return true;
}

void Client::Close() {
    clientSock.Close();
    LIBPOG_ASSERT(!clientSock.IsConnected(), "Socket must be disconnected");
    LIBPOG_ASSERT(!clientSock.IsOpen(), "Socket must be closed");
    std::cout << "Connection closed. Goodbye!" << std::endl;
}

bool Client::SendEcho(const std::string& message) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Echo);
    const auto* packet = builder.Append(message).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send echo command." << std::endl;
        return false;
    }

    return true;
}

bool Client::SendTime() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Time);
    const auto* packet = builder.Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send time command." << std::endl;
        return false;
    }

    return true;
}

bool Client::SendDownload(const std::string& fileName) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Download);
    const auto* packet = builder.Append(fileName).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send download command." << std::endl;
        return false;
    }

    return true;
}

bool Client::SendUpload(const std::string& fileName) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Upload);
    const auto* packet = builder.Append(fileName).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send upload command." << std::endl;
        return false;
    }

    return true;
}

bool Client::SendClose() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Close);
    const auto* packet = builder.Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send close command." << std::endl;
        return false;
    }

    return true;
}
