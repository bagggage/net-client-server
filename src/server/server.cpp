#include "server.h"

#include "packet.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>

static void TakeBitrate(const std::chrono::system_clock::time_point begin, const uint bytes) {
    const auto end = std::chrono::system_clock::now();

    const size_t timeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    const float bitrate = ((double)bytes / 125000) / ((double)timeNs / 1e+9);

    std::cout << "Bitrate: " << bitrate << " Mb/s.\n";
}

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
    client.tcpSock.Receive(client.identifier);

    if (CheckFail(client)) [[unlikely]] goto fail_ret;

    {
        const auto downloadStamp = recoveryStamps.find(client.identifier);
        if (downloadStamp != recoveryStamps.end()) [[unlikely]] {
            auto builder = Msg::Packet::Build(Msg::Opcodes::DownloadRecovery);
            const auto packet = builder
                .Append(downloadStamp->second.position)
                .Append(downloadStamp->second.filePath.filename().c_str())
                .Complete();

            client.tcpSock.Send(packet->RawPtr(), packet->GetSize());
        } else {
            recoveryStamps.clear();

            Msg::Packet::Header packet { Msg::Opcodes::None };
            client.tcpSock.Send(packet);
        }

        if (CheckFail(client)) [[unlikely]] goto fail_ret;
    }

    std::cout << "Client [" << client.identifier.ToString() << "] connected.\n";
    return clientIndex;

fail_ret:
    std::cerr << "Client connection failed.\n"; 
    return INVALID_CLIENT_INDEX;
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

    std::cerr << "client[" << client.identifier.ToString() << "]: " << Net::GetStatusName(status) << ".\n";

    switch (status) {
        case Net::Status::Timeout:
        case Net::Status::Unreachable:
        case Net::Status::ConnectionRefused:
        case Net::Status::ConnectionReset: {
            // Swap remove.
            auto it = std::find(clients.begin(), clients.end(), client);
            auto last = std::prev(clients.end());
            if (it != last) std::iter_swap(it, last);

            clients.pop_back();
            return false;
        }
        default:
            break;
    }

    return true;
}

bool Server::HandlePacket(ClientHandle& client, const Msg::Packet* packet) {
    std::cout << "Handle packet: type: " << (int)packet->GetHeader().opcode << " - size: " << packet->GetSize() << ".\n";

    switch (packet->GetHeader().opcode) {
        case Msg::Opcodes::Echo: {
            client.tcpSock.Send(packet->GetDataAs<char>(), packet->GetDataSize());
        } break;
        case Msg::Opcodes::Time: {
            const std::time_t serverTime = std::time(nullptr);
            client.tcpSock.Send(reinterpret_cast<const char*>(&serverTime), sizeof(serverTime));
        } break;
        case Msg::Opcodes::Download: {
            const auto request = packet->GetDataAs<Msg::Request::Download>();
            return HandleDownload(client, request->position, request->fileName);
        }
        case Msg::Opcodes::Upload:
            return HandleUpload(client, packet->GetDataAs<Msg::Request::Upload>());
        default:
            std::cerr << "Invalid packet from client[" << client.identifier.ToString() << "].\n";
            return false;
    }

    return !CheckFail(client);
}

bool Server::HandleDownload(ClientHandle& client, const size_t startPos, const char* fileName) {
    const auto filePath = hostDirectory / fileName;

    std::ifstream fileStream;
    Msg::Response::Download response;

    // Check if file exists and can be open.
    if (std::filesystem::exists(filePath)) {
        if (std::filesystem::is_regular_file(filePath) == false) {
            response.status = Msg::Response::Download::IsNotFile;
            std::cout << "Is not file: " << filePath << ".\n";
            goto sendPacket;
        }

        response.status = Msg::Response::Download::Ready;
        response.totalSize = std::filesystem::file_size(filePath);

        if (response.totalSize <= startPos) [[unlikely]] { response.totalSize = 0; }
        else { response.totalSize -= startPos; }

        fileStream.open(filePath, std::ios::binary);

        if (fileStream.is_open() == false) [[unlikely]] {
            response.status = Msg::Response::Download::NoSuchFile;
            std::cout << "Failed to open file: " << filePath << ".\n";
        }
    } else {
        response.status = Msg::Response::Download::NoSuchFile;
        std::cout << "No such file: " << filePath << ".\n";
    }

sendPacket:
    {
        client.tcpSock.Send(response);

        if (CheckFail(client)) [[unlikely]] return false;
        if (response.status != Msg::Response::Download::Ready) return true;
    }

    if (response.totalSize == 0) [[unlikely]] return true;

    const auto beginTime = std::chrono::system_clock::now();

    // Send file.
    size_t bytesToSend = response.totalSize - startPos;
    while (bytesToSend > 0) {
        const size_t chunkSize = std::min(DEFAULT_BUFFER_SIZE, bytesToSend);
        fileStream.read(client.buffer, chunkSize);

        client.tcpSock.Send(
            client.buffer, chunkSize,
            (bytesToSend > chunkSize) ?
                Net::Socket::MoreDataToSend :
                Net::Socket::None
        );
        if (CheckFail(client)) {
            recoveryStamps[client.identifier] = DownloadStamp{ filePath, startPos + response.totalSize - bytesToSend };
            return false;
        }

        bytesToSend -= chunkSize;
    }

    TakeBitrate(beginTime, response.totalSize);

    return true;
}

bool Server::HandleUpload(ClientHandle& client, const Msg::Request::Upload* request) {
    const size_t fileSize = request->fileSize;
    size_t bytesToReceive = request->fileSize;

    if (request->fileSize == 0) return false;
    if (request->fileName[0] == '.' || request->fileName[0] == '/' || request->fileName[0] == '~') {
        // Ignore file content: invalid file name
    ignoreContent:
        while (bytesToReceive > 0) {
            const size_t chunkSize = std::min(DEFAULT_BUFFER_SIZE, bytesToReceive);

            uint received = client.tcpSock.Receive(client.buffer, chunkSize, Net::Socket::WaitAll);
            if (CheckFail(client)) [[unlikely]] return false;

            bytesToReceive -= received;
        }
        return false;
    }

    const std::filesystem::path filePath = hostDirectory / request->fileName;
    std::ofstream fileStream(filePath, std::ios::binary);

    if (fileStream.is_open() == false) [[unlikely]] {
        std::cout << "Failed to create or open file at " << filePath << ".\n";
        goto ignoreContent;
    }

    const auto beginTime = std::chrono::system_clock::now();
    while (bytesToReceive > 0) {
        const size_t chunkSize = std::min(DEFAULT_BUFFER_SIZE, bytesToReceive);

        uint received = client.tcpSock.Receive(client.buffer, chunkSize);
        if (CheckFail(client)) [[unlikely]] {
            fileStream.close();
            std::filesystem::remove(filePath);
            return false;
        }

        fileStream.write(client.buffer, received);
        bytesToReceive -= received;
    }

    TakeBitrate(beginTime, fileSize);

    std::cout << "File saved at " << filePath << ".\n";
    return true;
}