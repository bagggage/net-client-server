#ifndef _CLIENT_CONSOLE_H
#define _CLIENT_CONSOLE_H

#include "client.h"
#include "console.h"

#include <filesystem>

class ClientConsole : public Console {
private:
    static Client client;
    static CommandSet commandSet;

    static const CommandSet& GetCommandSet();

    static void CloseCmd();
    static void ConnectCmd(std::string ipAddress, unsigned short port);
    static void DisconnectCmd();
    static void DownloadCmd(std::string_view fileName);
    static void EchoCmd(std::string_view message);
    static void TimeCmd();
    static void UploadCmd(std::string_view filePath);
public:
    ClientConsole(StdIoConsoleStream& stream) : Console(stream, GetCommandSet()) {}
};

#endif