#ifndef _SERVER_H
#define _SERVER_H

#include "socket.h"
#include "packet.h"
#include "net.h"

#include <vector>
#include <filesystem>
#include <unordered_map>

class Server {
public:
    static constexpr int INVALID_CLIENT_INDEX = -1;
    static constexpr size_t DEFAULT_BUFFER_SIZE = 4096;

    static constexpr const char* DEFAULT_FILES_DIR = "./hosted-files";
private:
    class ClientHandle {
    public:
        Net::Socket tcpSock;
        Net::MacAddress identifier;
        char* buffer = nullptr;

        ClientHandle(Net::Socket&& socket) : tcpSock(std::move(socket)) {
            buffer = new char[DEFAULT_BUFFER_SIZE];
        }
        ClientHandle(ClientHandle&& other) : tcpSock(std::move(other.tcpSock)) {
            buffer = other.buffer;
            identifier = other.identifier;

            other.buffer = nullptr;
        }

        ~ClientHandle() { delete[] buffer; }

        friend void swap(ClientHandle& a, ClientHandle& b) {
            using std::swap;
            swap(a.buffer,     b.buffer);
            swap(a.identifier, b.identifier);
        }

        bool operator==(const ClientHandle& other) const {
            return this == &other;
        }
    };

    struct DownloadStamp {
        std::filesystem::path filePath;
        size_t position;
    };

    Net::Socket tcpListenSock;
    std::vector<ClientHandle> clients;
    std::unordered_map<Net::MacAddress, DownloadStamp> recoveryStamps;

    std::filesystem::path hostDirectory;

    bool CheckFail(ClientHandle& client);
    bool HandlePacket(ClientHandle& client, const Msg::Packet* packet);
    bool HandleDownload(ClientHandle& client, const size_t startPos, const char* fileName);
    bool HandleUpload(ClientHandle& client, const Msg::Request::Upload* request);

public:
    Server(const Net::Address::port_t port);

    int Accept();
    bool Handle(unsigned int clientIndex);

    inline void SetHostDirectory(std::string_view path) { hostDirectory = path; }

    inline bool IsListening() { return tcpListenSock.IsListening(); }
    inline Net::Status Fail() { return tcpListenSock.Fail(); }

    inline Net::Status ClientFail(unsigned int clientIndex) { return clients[clientIndex].tcpSock.Fail(); }
};

#endif