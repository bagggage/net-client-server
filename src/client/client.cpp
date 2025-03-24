#include "client.h"

#include <fstream>

#include <core/client.h>
#include <core/packet.h>
#include <core/net.h>

static void TakeBitrate(const std::chrono::system_clock::time_point begin, const uint bytes) {
    const auto end = std::chrono::system_clock::now();

    const size_t timeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    const float bitrate = ((double)bytes / 125000) / ((double)timeNs / 1e+9);

    std::cout << "Bitrate: " << bitrate << " Mb/s.\n";
}

const char* Client::GetLoadResultName(const LoadResult result) {
    switch (result) {
        case Success: return "success";
        case NetworkError: return "network error";
        case NoSuchFile: return "no such file";
        case NotRegularFile: return "not a regular file";
        default: return "unknown";
    }
}

Client::LoadResult Client::HandleDownloadRecovery(std::string& outFileName) {
    Msg::Packet packet {};
    if (connection->ReceiveAll(packet) < sizeof(packet)) [[unlikely]] return NetworkError;
    if (!packet.Is(Msg::Opcodes::DownloadRecovery)) return NoSuchFile;

    if (connection->ReceiveAll(buffer.data(), packet.GetDataSize()) < packet.GetDataSize()) [[unlikely]] return NetworkError;

    const auto response = reinterpret_cast<Msg::Response::DownloadRecovery*>(buffer.data());
    outFileName = response->fileName;

    return Success;
}

bool Client::Connect(const Net::Protocol protocol, const std::string& address, unsigned short port) {
    Net::Address serverAddress = Net::Address::FromDomain(address.c_str(), port);

    if (protocol == Net::Protocol::TCP) {
        connection = Net::TcpClient::Connect(serverAddress);
    } else {
        connection = Net::UdpClient::Connect(serverAddress);
    }

    if (connection == nullptr || connection->Fail()) return false;

    std::cout << "Send mac address.\n";

    const Net::MacAddress macAddress = Net::GetMacAddress();
    if (connection->Send(macAddress) == false) [[unlikely]] return false;

    return true;
}

std::string_view Client::Echo(const std::string_view message) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Echo);
    const auto* packet = builder.Append(message.begin(), message.size() + 1).Complete();

    if (!connection->Send(packet->RawPtr(), sizeof(Msg::Packet::Header))) [[unlikely]] return {};
    if (!connection->Send(packet->RawPtr() + sizeof(Msg::Packet::Header), packet->GetDataSize())) [[unlikely]] return {};

    if (!connection->ReceiveAll(buffer.data(), message.size() + 1)) [[unlikely]] return {};

    return std::string_view(buffer.data(), message.size());
}

std::time_t Client::Time() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Time);
    const auto* packet = builder.Complete();

    if (!connection->Send(packet->RawPtr(), packet->GetSize())) [[unlikely]] return 0;
    if (!connection->ReceiveAll(buffer.data(), sizeof(std::time_t))) [[unlikely]] return 0;

    return *reinterpret_cast<const std::time_t*>(buffer.data());
}

Client::LoadResult Client::Download(const std::string_view fileName, const size_t startPos) {    
    const std::filesystem::path filePath = downloadPath / fileName;
    std::ofstream fileStream(
        filePath,
        std::ios_base::binary | ((startPos > 0) ? std::ios_base::app : std::ios_base::out)
    );
    if (!fileStream.is_open()) [[unlikely]] return InvalidSavePath;

    LoadResult result = NetworkError;

    const Msg::Request::Download request = { startPos };

    auto builder = Msg::Packet::Build(Msg::Opcodes::Download);
    const auto* packet = builder.Append(request).Append(fileName).Complete();

    if (!connection->Send(packet->RawPtr(), sizeof(Msg::Packet::Header))) [[unlikely]] goto ret;
    if (!connection->Send(packet->RawPtr() + sizeof(Msg::Packet::Header), packet->GetDataSize())) [[unlikely]] goto ret;

    if (!connection->ReceiveAll(buffer.data(), sizeof(Msg::Response::Download))) [[unlikely]] goto ret;

    {
        const Msg::Response::Download* response = reinterpret_cast<Msg::Response::Download*>(buffer.data());
        if (response->status != Msg::Response::Download::Ready) {
            result = (response->status == Msg::Response::Download::NoSuchFile) ? NoSuchFile : NotRegularFile;
            goto ret;
        }

        // Receive file by chunks.
        {
            if (startPos != 0) fileStream.seekp(startPos);

            const size_t dataSize = response->totalSize;
            const auto beginTime = std::chrono::system_clock::now();

            size_t bytesToReceive = response->totalSize;
            while (bytesToReceive > 0) {
                const size_t chunkSize = std::min(buffer.size(), bytesToReceive);
                const uint received = connection->ReceiveAll(buffer.data(), chunkSize);

                if (received == 0) goto ret;
                // ACK
                if (!connection->Send(buffer.data(), 1)) goto ret;

                fileStream.write(buffer.data(), received);
                bytesToReceive -= received;
            }

            TakeBitrate(beginTime, dataSize);
        }

        result = Success;
    }

ret:
    if (result != Success) {
        fileStream.close();
        std::filesystem::remove(filePath);
    }

    return result;
}

Client::LoadResult Client::Upload(const std::string_view filePathStr) {
    const std::filesystem::path filePath = filePathStr;

    std::ifstream fileStream(filePath, std::ios::binary);
    if (!fileStream.is_open()) [[unlikely]] return NoSuchFile;

    Msg::Request::Upload request;
    request.fileSize = std::filesystem::file_size(filePath);

    auto builder = Msg::Packet::Build(Msg::Opcodes::Upload);
    const auto* packet = builder
        .Append(request)
        .Append(filePath.filename().c_str())
        .Complete();

    if (!connection->Send(packet->RawPtr(), sizeof(Msg::Packet::Header))) [[unlikely]] return NetworkError;
    if (!connection->Send(packet->RawPtr() + sizeof(Msg::Packet::Header), packet->GetDataSize())) [[unlikely]] return NetworkError;

    const auto beginTime = std::chrono::system_clock::now();

    size_t bytesToSend = request.fileSize;
    while (bytesToSend > 0) {
        const size_t chunkSize = std::min(buffer.size(), bytesToSend);
        fileStream.read(buffer.data(), chunkSize);

        if (!connection->Send(buffer.data(), chunkSize)) [[unlikely]] return NetworkError;

        bytesToSend -= chunkSize;
    }

    TakeBitrate(beginTime, request.fileSize);

    return Success;
}

bool Client::Close() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Close);
    const auto* packet = builder.Complete();

    return connection->Send(packet->RawPtr(), packet->GetSize()) == packet->GetSize();
}
