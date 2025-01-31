#include "server.h"

#include "packet.h"

#include <iostream>
#include <fstream>
#include <filesystem>

Server::Server(const Net::Address::port_t port) {
    tcpListenSock.Open(Net::Address::Family::IPv4, Net::Protocol::TCP);
    tcpListenSock.SetOption(Net::Socket::Option::ReuseAddress, true);
    tcpListenSock.Listen(Net::Address::MakeBind(port, Net::Protocol::TCP, Net::Address::Family::IPv4));
};

int Server::Accept() {
    Net::Socket tcpClientSock = tcpListenSock.Accept();
    if (tcpClientSock.IsValid() == false) [[unlikely]] return INVALID_CLIENT_INDEX;

    const auto clientIndex = clients.size();
    clients.push_back(ClientHandle(std::move(tcpClientSock)));

    ClientHandle& client = clients[clientIndex];
    client.tcpSock.Receive(reinterpret_cast<char*>(&client.identifier), sizeof(client.identifier));

    if (CheckFail(client)) {
        std::cerr << "Client connection failed.\n"; 
        return INVALID_CLIENT_INDEX;
    }

    return clientIndex;
}

bool Server::Handle(unsigned int clientIndex) {
    ClientHandle& client = clients[clientIndex];

    Msg::Packet* packet = reinterpret_cast<Msg::Packet*>(client.buffer);
    if (client.tcpSock.Receive(packet) < sizeof(Msg::Packet)) {
        CheckFail(client);
        return false;
    }
    if (packet->GetDataSize() > 0) {
        if (client.tcpSock.Receive(client.buffer + sizeof(Msg::Packet), packet->GetDataSize()) == 0) [[unlikely]] return false;
    }

    return HandlePacket(client, packet);
}

bool Server::CheckFail(ClientHandle& client) {
    Net::Status status = client.tcpSock.Fail();
    if (status == Net::Status::Success) return false;

    std::cerr << "client[" << client.identifier.ToString() << "]: " << Net::GetStatusName(status) << '\n';

    switch (status) {
        case Net::Status::Timeout:
            break;
        case Net::Status::Unreachable:
            break;
        case Net::Status::ConnectionRefused:
            break;
        case Net::Status::ConnectionReset:
            break;
        default:
            break;
    }

    return true;
}

bool Server::HandlePacket(ClientHandle& client, const Msg::Packet* packet) {
    std::cout << "Handle packet: type: " << (int)packet->GetHeader().opcode << " - size: " << packet->GetSize() << '\n';

    switch (packet->GetHeader().opcode) {
        case Msg::Opcodes::Echo: {
            std::cout << "Echo\n";
            client.tcpSock.Send(packet->RawPtr(), packet->GetSize());
        } break;
        case Msg::Opcodes::Time: {
            const std::time_t serverTime = std::time(nullptr);
            client.tcpSock.Send(reinterpret_cast<const char*>(&serverTime), sizeof(serverTime));
        } break;
        case Msg::Opcodes::Download:
            return HandleDownload(client, packet->GetDataAs<Msg::Request::Download>()->fileName);
            break;
        default:
            std::cerr << "Invalid packet from client[" << client.identifier.ToString() << "].\n";
            return false;
    }

    return !CheckFail(client);
}

bool Server::HandleDownload(ClientHandle& client, const char* fileName) {
    const auto filePath = hostDirectory / fileName;

    std::ifstream fileStream;

    Msg::Response::Download response;

    if (std::filesystem::exists(filePath)) {
        if (std::filesystem::is_regular_file(filePath) == false) {
            response.status = Msg::Response::Download::IsNotFile;
            std::cout << "Is not file: " << filePath << '\n';
            goto sendPacket;
        }

        response.status = Msg::Response::Download::Ready;
        response.totalSize = std::filesystem::file_size(filePath);

        fileStream.open(filePath, std::ios::binary);

        if (fileStream.is_open() == false) [[unlikely]] {
            response.status = Msg::Response::Download::NoSuchFile;
            std::cout << "Failed to open file: " << filePath << '\n';
        }
    } else {
        response.status = Msg::Response::Download::NoSuchFile;
        std::cout << "No such file: " << filePath << '\n';
    }

sendPacket:
    {
        client.tcpSock.Send(response);

        if (CheckFail(client)) [[unlikely]] return false;
        if (response.status != Msg::Response::Download::Ready) return true;
    }

    // Send file.
    size_t bytesToSend = response.totalSize;
    while (bytesToSend > 0) {
        const size_t chunkSize = std::min(DEFAULT_BUFFER_SIZE, bytesToSend);
        fileStream.read(client.buffer, chunkSize);

        const uint sended = client.tcpSock.Send(client.buffer, chunkSize, bytesToSend > chunkSize ? MSG_MORE : 0);
        if (CheckFail(client)) [[unlikely]] return false;

        bytesToSend -= chunkSize;
    }

    return true;
}