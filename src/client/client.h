#ifndef _CLIENT_H
#define _CLIENT_H

#include "utils.h"
#include "socket.h"
#include <iostream>
#include <string>

#define DEFAULT_SERVER_PORT 8088
#define DEFAULT_SERVER_ADDRESS "127.0.0.1"

class Client {
public:
    static Client& GetInstance();

    bool Connect(const std::string& address, unsigned short port);
    void Close();

    bool SendEcho(const std::string& message);
    bool SendTime();
    bool SendDownload(const std::string& fileName);
    bool SendUpload(const std::string& fileName);
    bool SendClose();

private:
    Net::Address serverAddress;
    Net::Socket clientSock;

    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;
};

#endif // _CLIENT_H
