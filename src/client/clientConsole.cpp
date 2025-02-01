#include "clientConsole.h"

Client ClientConsole::client = {};
CommandSet ClientConsole::commandSet = {};

const CommandSet& ClientConsole::GetCommandSet() {
    static bool isInitialized = false;
    if (isInitialized) return commandSet;

    commandSet.RegisterCommand("close",     "\tSending close command to the server",          CloseCmd);
    commandSet.RegisterCommand("connect",    "Connecting to the server with <ip> and <port>", ConnectCmd);
    commandSet.RegisterCommand("download",   "Downloading file <name> from srver",            DownloadCmd);
    commandSet.RegisterCommand("disconnect", "Close connection on the client side",           DisconnectCmd);
    commandSet.RegisterCommand("echo",      "\tReturns <msg> from server",                    EchoCmd);
    commandSet.RegisterCommand("time",      "\tReturns current server time",                  TimeCmd);
    commandSet.RegisterCommand("upload",     "Uploading file <name> to server",               UploadCmd);

    isInitialized = true;
    return commandSet;
}

void ClientConsole::CloseCmd() {
    if (!client.Close()) {
        std::cerr << "Close failed: " << Net::GetStatusName(client.GetStatus()) << ".\n";
    }
}

void ClientConsole::ConnectCmd(std::string ipAddress, unsigned short port) {
    if (!client.Connect(ipAddress, port)) {
        std::cerr << "Connection failed: " << Net::GetStatusName(client.GetStatus()) << ".\n";
        return;
    }

    std::cout << "Connected succefully.\n";
}

void ClientConsole::EchoCmd(std::string_view message) {
    const std::string_view response = client.Echo(message);
    if (response.empty()) {
        std::cerr << "Invalid echo response: " << Net::GetStatusName(client.GetStatus()) << ".\n";
        return;
    }

    std::cout << response << '\n';
}

void ClientConsole::TimeCmd() {
    std::time_t response = client.Time();
    if (response == 0) [[unlikely]] {
        std::cerr << "Command failed: " << Net::GetStatusName(client.GetStatus()) << ".\n";
        return;
    }

    std::cout << "Server time: " << std::ctime(&response);
}

void ClientConsole::DownloadCmd(std::string_view fileName) {
    if (Client::LoadResult result = client.Download(fileName)) {
        std::cerr << "Download failed: " <<
            ((result == Client::NetworkError) ? Net::GetStatusName(client.GetStatus()) : Client::GetLoadResultName(result))
            << ".\n";
        return;
    }

    std::cout << "File saved at " << client.downloadPath << ".\n";
}

void ClientConsole::UploadCmd(std::string_view fileName) {
    if (!std::filesystem::exists(fileName)) {
        std::cerr << "No such file.\n";
        return;
    }
    if (!std::filesystem::is_regular_file(fileName)) {
        std::cerr << "Is not a regular file.\n";
        return;
    }

    if (Client::LoadResult result = client.Upload(fileName)) {
        std::cerr << "Uploading failed: " <<
            ((result == Client::NetworkError) ? Net::GetStatusName(client.GetStatus()) : Client::GetLoadResultName(result))
            << ".\n";
        return;
    }

    std::cout << "File uploaded succefully.\n";
}

void ClientConsole::DisconnectCmd() {
    if (!client.Close()) {
        std::cerr << Net::GetStatusName(client.GetStatus()) << ".\n";
    }
}