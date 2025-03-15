#include "server.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>

#include <core/packet.h>

static void TakeBitrate(const std::chrono::system_clock::time_point begin, const uint bytes) {
    const auto end = std::chrono::system_clock::now();

    const size_t timeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    const float bitrate = ((double)bytes / 125000) / ((double)timeNs / 1e+9);

    std::cout << "Bitrate: " << bitrate << " Mb/s.\n";
}

Server::Server(const Net::Address::port_t port) : port(port) {
    listenServer = std::make_unique<Net::TcpServer>();
    listenServer->Bind(Net::Address::MakeBind(port, Net::Protocol::TCP));
};

void Server::Accept(Net::Connection* clientConnection) {
    ClientHandle& client = clients.at(clientConnection);
    std::cout << "Receive mac address...\n";
    client.connection->Receive(client.identifier);

    if (CheckFail(client)) [[unlikely]] goto fail_ret;

    {
        const auto downloadStamp = recoveryStamps.find(client.identifier);
        if (downloadStamp != recoveryStamps.end()) [[unlikely]] {
            auto builder = Msg::Packet::Build(Msg::Opcodes::DownloadRecovery);
            const auto packet = builder
                .Append(downloadStamp->second.position)
                .Append(downloadStamp->second.filePath.filename().c_str())
                .Complete();

            client.connection->Send(packet->RawPtr(), sizeof(Msg::Packet::Header));
            client.connection->Send(packet->GetDataAs<char>(), packet->GetDataSize());

            recoveryStamps.erase(downloadStamp);
        } else {
            recoveryStamps.clear();

            std::cout << "Send none stamps.\n";
            Msg::Packet::Header packet { Msg::Opcodes::None };
            client.connection->Send(packet);
        }

        if (CheckFail(client)) [[unlikely]] goto fail_ret;
    }

    std::cout << "Client [" << client.identifier.ToString() << "] connected.\n";
    return;

fail_ret:
    std::cout << "Client connection failed.\n";
    return;
}

Net::Connection* Server::Listen() {
    Net::Connection* clientConnection = nullptr;
    bool is_new_client = false;

    do {
        is_new_client = false;

        clientConnection = listenServer->Listen();
        if (clientConnection == nullptr) return nullptr;

        if (clients.count(clientConnection) == 0) {
            is_new_client = true;
            clients.insert({ clientConnection, ClientHandle(clientConnection) });

            Accept(clientConnection);
        }
    } while (is_new_client);
    
    return clientConnection;
}

bool Server::HandleClient(Net::Connection* clientConnection) {
    ClientHandle& client = clients.at(clientConnection);

    Msg::Packet* packet = reinterpret_cast<Msg::Packet*>(client.buffer);
    if (client.connection->Receive(*packet) == false) {
        CheckFail(client);
        return false;
    }
    if (packet->GetDataSize() > 0) {
        if (client.connection->Receive(client.buffer + sizeof(Msg::Packet), packet->GetDataSize()) == 0) [[unlikely]] return false;
    }

    return HandlePacket(client, packet);
}

bool Server::CheckFail(ClientHandle& client) {
    Net::Status status = client.connection->Fail();
    if (status == Net::Status::Success) return false;

    std::cerr << "client[" << client.identifier.ToString() << "]: " << Net::GetStatusName(status) << ".\n";

    switch (status) {
        case Net::Status::Timeout:
        case Net::Status::TryAgain:
        case Net::Status::Unreachable:
        case Net::Status::ConnectionRefused:
        case Net::Status::ConnectionReset: {
            clients.erase(client.connection.get());
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
            client.connection->Send(packet->GetDataAs<char>(), packet->GetDataSize());
        } break;
        case Msg::Opcodes::Time: {
            const std::time_t serverTime = std::time(nullptr);
            client.connection->Send(reinterpret_cast<const char*>(&serverTime), sizeof(serverTime));
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
        client.connection->Send(response);

        if (CheckFail(client)) [[unlikely]] return false;
        if (response.status != Msg::Response::Download::Ready) return true;
    }

    if (response.totalSize == 0) [[unlikely]] return true;
    if (startPos != 0) fileStream.seekg(startPos, std::ios::beg);

    const auto beginTime = std::chrono::system_clock::now();

    //client.connection->SetupTimeout();

    // Send file.
    size_t bytesToSend = response.totalSize;
    while (bytesToSend > 0) {
        const size_t chunkSize = std::min(DEFAULT_BUFFER_SIZE, bytesToSend);
        fileStream.read(client.buffer, chunkSize);

        client.connection->Send(client.buffer, chunkSize);
        // ACK
        if (client.connection->Receive(client.buffer, 1) != 1 || CheckFail(client)) {
            recoveryStamps[client.identifier] = DownloadStamp{ filePath, startPos + response.totalSize - bytesToSend };
            //client.connection->DropTimeout();
            return false;
        }

        bytesToSend -= chunkSize;
    }

    TakeBitrate(beginTime, response.totalSize);
    //client.connection->DropTimeout();

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

            uint received = client.connection->Receive(client.buffer, chunkSize);
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

    //client.connection->SetupTimeout();

    const auto beginTime = std::chrono::system_clock::now();
    while (bytesToReceive > 0) {
        const size_t chunkSize = std::min(DEFAULT_BUFFER_SIZE, bytesToReceive);

        uint received = client.connection->Receive(client.buffer, chunkSize);
        client.connection->Send(client.buffer, 1); // ACK

        if (CheckFail(client)) [[unlikely]] {
            fileStream.close();
            std::filesystem::remove(filePath);
            //client.connection->DropTimeout();
            return false;
        }

        fileStream.write(client.buffer, received);
        bytesToReceive -= received;
    }

    TakeBitrate(beginTime, fileSize);
    //client.connection->DropTimeout();

    std::cout << "File saved at " << filePath << ".\n";
    return true;
}