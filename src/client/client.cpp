#include "client.h"
#include "packet.h"
#include "net.h"

#include <fstream>

static void takeBitrate(const std::chrono::system_clock::time_point begin, const uint bytes) {
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

bool Client::Connect(const std::string& ipAddress, unsigned short port) {
    const Net::Address serverAddress = Net::Address::FromString(ipAddress.c_str(), port);

    clientSock.Open(Net::Address::Family::IPv4, Net::Protocol::TCP);
    if (!clientSock.IsOpen()) [[unlikely]] return false;

    clientSock.Connect(serverAddress);
    if (!clientSock.IsConnected()) return false;

    const Net::MacAddress macAddress = Net::GetMacAddress();
    if (!clientSock.Send(reinterpret_cast<const char*>(macAddress.bytes.data()), macAddress.bytes.size())) {
        return false;
    }

    return true;
}

std::string_view Client::Echo(const std::string_view message) {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Echo);
    const auto* packet = builder.Append(message.begin(), message.size() + 1).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) [[unlikely]] return {};
    if (!clientSock.Receive(buffer.data(), message.size() + 1, Net::Socket::WaitAll)) [[unlikely]] return {};

    return std::string_view(buffer.data(), message.size());
}

std::time_t Client::Time() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Time);
    const auto* packet = builder.Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) [[unlikely]] return 0;
    if (!clientSock.Receive(buffer.data(), sizeof(std::time_t), Net::Socket::WaitAll)) [[unlikely]] return 0;

    return *reinterpret_cast<const std::time_t*>(buffer.data());
}

Client::LoadResult Client::Download(const std::string_view fileName) {
    const std::filesystem::path filePath = downloadPath / fileName;
    std::ofstream fileStream(filePath, std::ios::binary);
    if (!fileStream.is_open()) [[unlikely]] return InvalidSavePath;

    LoadResult result = NetworkError;

    auto builder = Msg::Packet::Build(Msg::Opcodes::Download);
    const auto* packet = builder.Append(fileName.data(), fileName.size()).Append((char)0).Complete();

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) [[unlikely]] goto ret;
    if (!clientSock.Receive(buffer.data(), sizeof(Msg::Response::Download), Net::Socket::WaitAll)) [[unlikely]] goto ret;

    {
        const Msg::Response::Download* response = reinterpret_cast<Msg::Response::Download*>(buffer.data());
        if (response->status != Msg::Response::Download::Ready) {
            result = (response->status == Msg::Response::Download::NoSuchFile) ? NoSuchFile : NotRegularFile;
            goto ret;
        }

        // Send file by chunks.
        {
            const size_t fileSize = response->totalSize;
            const auto beginTime = std::chrono::system_clock::now();

            size_t bytesToReceive = response->totalSize;
            while (bytesToReceive > 0) {
                const size_t chunkSize = std::min(buffer.size(), bytesToReceive);

                uint received = clientSock.Receive(buffer.data(), chunkSize);
                if (received == 0) goto ret;

                fileStream.write(buffer.data(), received);
                bytesToReceive -= received;
            }

            takeBitrate(beginTime, fileSize);
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

    if (!clientSock.Send(packet->RawPtr(), packet->GetSize())) [[unlikely]] return NetworkError;

    const auto beginTime = std::chrono::system_clock::now();

    size_t bytesToSend = request.fileSize;
    while (bytesToSend > 0) {
        const size_t chunkSize = std::min(buffer.size(), bytesToSend);
        fileStream.read(buffer.data(), chunkSize);

        uint sended = clientSock.Send(
            buffer.data(), chunkSize,
            (bytesToSend > chunkSize) ?
                Net::Socket::MoreDataToSend :
                Net::Socket::None
        );
        if (sended != chunkSize) [[unlikely]] return NetworkError;

        bytesToSend -= chunkSize;
    }

    takeBitrate(beginTime, request.fileSize);

    return Success;
}

bool Client::Close() {
    auto builder = Msg::Packet::Build(Msg::Opcodes::Close);
    const auto* packet = builder.Complete();

    return clientSock.Send(packet->RawPtr(), packet->GetSize()) == packet->GetSize();
}
