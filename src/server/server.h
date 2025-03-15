#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <filesystem>
#include <unordered_map>

#include <core/server.h>
#include <core/socket.h>
#include <core/packet.h>
#include <core/net.h>

class Server {
public:
    static constexpr int INVALID_CLIENT_INDEX = -1;
    static constexpr size_t DEFAULT_BUFFER_SIZE = 4096 * 2;

    static constexpr const char* DEFAULT_FILES_DIR = "./hosted-files";
private:
    class ClientHandle {
    public:
        Net::Ptr<Net::Connection> connection;
        Net::MacAddress identifier;
        char* buffer = nullptr;

        ClientHandle(Net::Connection* connection) : connection(connection) {
            buffer = new char[DEFAULT_BUFFER_SIZE];
        }
        ClientHandle(ClientHandle&& other) {
            connection.reset(other.connection.release());

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

    Net::Ptr<Net::Server> listenServer;

    std::unordered_map<Net::Connection*, ClientHandle> clients;
    std::unordered_map<Net::MacAddress, DownloadStamp> recoveryStamps;

    Net::Address::port_t port;
    std::filesystem::path hostDirectory;

    bool CheckFail(ClientHandle& client);
    bool HandlePacket(ClientHandle& client, const Msg::Packet* packet);
    bool HandleDownload(ClientHandle& client, const size_t startPos, const char* fileName);
    bool HandleUpload(ClientHandle& client, const Msg::Request::Upload* request);

    void Accept(Net::Connection* clientConnection);

public:
    Server(const Net::Address::port_t port);

    Net::Connection* Listen();
    bool HandleClient(Net::Connection* clientConnection);

    inline void SetHostDirectory(std::string_view path) { hostDirectory = path; }

    inline Net::Status Fail() { return listenServer->Fail(); }
    inline Net::Status ClientFail(Net::Connection* clientConnection) { return clients.at(clientConnection).connection->Fail(); }
};

#endif