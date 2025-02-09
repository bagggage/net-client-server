#ifndef _CLIENT_H
#define _CLIENT_H

#include <array>
#include <ctime>
#include <filesystem>
#include <string>
#include <iostream>

#define LIBPOG_LOGS false

#include "socket.h"

class Client {
public:
    static constexpr unsigned int DEFAULT_BUFFER_SIZE = 4096;
    static constexpr const char* DEFAULT_DOWNLOAD_DIRECTORY = "downloads";

    enum LoadResult {
        Success,
        InvalidSavePath,
        NoSuchFile,
        NotRegularFile,
        NetworkError,
    };

    static const char* GetLoadResultName(const LoadResult result);

private:
    Net::Socket clientSock;
    std::array<char, DEFAULT_BUFFER_SIZE> buffer;

public:
    std::filesystem::path downloadPath;

    bool Connect(const std::string& address, unsigned short port);
    void Disconnect();

    std::string_view Echo(const std::string_view message);
    std::time_t Time();
    LoadResult Download(const std::string_view fileName, const size_t startPos);
    LoadResult Upload(const std::string_view filePath);
    LoadResult HandleDownloadRecovery(std::string& outFileName);
    bool Close();

    inline Net::Status GetStatus() const { return clientSock.Fail(); }
};

#endif // _CLIENT_H
