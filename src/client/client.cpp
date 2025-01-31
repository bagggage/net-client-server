#include "client.h"
#include "packet.h"
#include "net.h"

#include <fstream>

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
    bool result = clientSock.Connect(serverAddress);

    if (!clientSock.IsConnected()) {
        std::cerr << "Socket is not connected. Exit." << std::endl;
        return false;
    }

    const Net::MacAddress macAddress = Net::GetMacAddress();

    if (!clientSock.Send(reinterpret_cast<const char*>(macAddress.bytes.data()), macAddress.bytes.size())) {
        std::cerr << "Failed to send MAC address." << std::endl;
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

std::string Client::SendEcho(const std::string& message) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Echo);
    const auto* packet = builder.Append(message.data(), message.size() + 1).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send echo command." << std::endl;
        return "";
    }

    buffer.fill(0);
    uint num = clientSock.Receive(buffer.data(), buffer.size());
    
    if (num > 0) {
        return std::string(buffer.data(), num);
    } else {
        std::cerr << "Failed to receive echoed data." << std::endl;
        return "";
    }
}

std::time_t Client::SendTime() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Time);
    const auto* packet = builder.Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send time command." << std::endl;
        return 0; 
    }

    buffer.fill(0);
    uint num = clientSock.Receive(buffer.data(), buffer.size());

    if (num == sizeof(std::time_t)) {
        return *reinterpret_cast<const std::time_t*>(buffer.data());
    } else {
        std::cerr << "Failed to receive time data." << std::endl;
        return 0; 
    }
}

bool Client::SendDownload(const std::string& fileName) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Download);
    const auto* packet = builder.Append(fileName.data(), fileName.size() + 1).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) {
        std::cerr << "Failed to send download command." << std::endl;
        return false;
    }

    uint num = clientSock.Receive(buffer.data(), sizeof(Msg::Packet) + sizeof(Msg::Response::Download));

    if (num == 0) {
        std::cerr << "Failed to get response data." << std::endl;
        return false;
    }

    const Msg::Response::Download* response = reinterpret_cast<Msg::Response::Download*>(buffer.data());

    if (response->status != Msg::Response::Download::Ready) {
        std::cerr << "Failed to receive file data." << std::endl;
        return false;
    }

    std::ofstream fileStream(fileName, std::ios::binary);

    size_t bytesToReceive = response->totalSize;
    while (bytesToReceive > 0) {
        const size_t chunkSize = std::min(buffer.size(), std::max(bytesToReceive, 128ul));

        uint received = clientSock.Receive(buffer.data(), chunkSize);
        if (clientSock.Fail()) return false;

        fileStream.write(buffer.data(), received);
        bytesToReceive -= received;
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
