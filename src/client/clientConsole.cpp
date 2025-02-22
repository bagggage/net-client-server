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

void ClientConsole::Download(const std::string_view fileName, const size_t startPos) {
    if (Client::LoadResult result = client.Download(fileName, startPos)) {
        std::cerr << "Download failed: " <<
            ((result == Client::NetworkError) ? Net::GetStatusName(client.GetStatus()) : Client::GetLoadResultName(result))
            << ".\n";
        return;
    }

    std::cout << "File saved at " << client.downloadPath << ".\n";
}

void ClientConsole::CloseCmd() {
    if (!client.Close()) {
        std::cerr << "Close failed: " << Net::GetStatusName(client.GetStatus()) << ".\n";
    }
}

void ClientConsole::ConnectCmd(const std::string_view protocolStr, std::string hostAddress, unsigned short port) {
    Net::Protocol protocol = (
        protocolStr == "tcp" ? Net::Protocol::TCP :
        (protocolStr == "udp" ? Net::Protocol::UDP : Net::Protocol::None)
    );
    if (protocol == Net::Protocol::None) {
        std::cerr << "Invalid protocol: " << protocolStr << ".\nconnect <protocol> <ip> <port>\n";
        return;
    }

    if (!client.Connect(protocol, hostAddress, port)) {
        std::cerr << "Connection failed: " << Net::GetStatusName(client.GetStatus()) << ".\n";
        return;
    }

    std::cout << "Connected succefully.\n";

    // Recovery invalid download.
    std::string fileName;
    const Client::LoadResult result = client.HandleDownloadRecovery(fileName);

    if (result == Client::NoSuchFile) return;
    if (result == Client::NetworkError) [[unlikely]] {
        std::cerr << "Failed: " << Client::GetLoadResultName(result) << ".\n";
        return;
    }

    // Ask to continue.
    {
        std::cout << "The download of file \'" << fileName << "\' was not completed, do you want to continue? [y/n]:\n";

        std::string answer;
        do {
            std::getline(std::cin, answer);
        } while (answer.empty());

        if (answer.size() != 1 || (answer[0] != 'y' && answer[0] != 'Y')) return;
    }

    const std::filesystem::path filePath = client.downloadPath / fileName;
    const size_t downloadedSize = std::filesystem::exists(filePath) ? std::filesystem::file_size(filePath) : 0;

    Download(fileName, downloadedSize);
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
    Download(fileName, 0);
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