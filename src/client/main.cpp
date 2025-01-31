#include "args.h"
#include "console.h"
#include "socket.h"
#include "packet.h"
#include "client.h"

#include <iostream>
#include <string_view>
#include <cmath>

struct Config {
    Net::Address::port_t serverPort = DEFAULT_SERVER_PORT;
    const char* serverIp = nullptr;

    bool Validate() const {
        if (serverIp == nullptr) {
            std::cerr << "Specify the server IP address, use \"-ip\"\n";
            return false;
        } else if (serverPort == 0) {
            std::cerr << "Speficy valid server port, use \"-p\"\n";
            return false;
        }

        return true;
    }
};

static void PrintHelp() {
    std::cout <<
        "Usage: client -i <server ip address> -p <server port number>\n"
        "  -ip, -i <address>  \tServer ip address.\n"
        "  -port, -p <port>\tServer port number.\n"
        "  -help, -h\tShow this help.\n";
    ;
}

void helloCmd(std::string_view name) {
    std::cout << "Hi " << name << "!\n";
}

void connect(std::string ip, unsigned short port) {
    Client& client = Client::GetInstance();

    if (!client.Connect(ip, port)) {
        std::cout << "Client cant be connected\n";
    }
}

void echo(std::string message) {
    Client& client = Client::GetInstance();

    if (!client.SendEcho(message)) {
        std::cout << "Error while sending ECHO command\n";
    }
}

void time() {
    Client& client = Client::GetInstance();

    if (!client.SendTime()) {
        std::cout << "Error while sending TIME command\n";
    }
}

void download(std::string fileName) {
    Client& client = Client::GetInstance();

    if (!client.SendDownload(fileName)) {
        std::cout << "Error while sending DOWNLOAD command\n";
    }
}

void upload(std::string fileName) {
    Client& client = Client::GetInstance();

    if (!client.SendUpload(fileName)) {
        std::cout << "Error while sending UPLOAD command\n";
    }
}

int main(int argc, const char** argv) {
    // // Client
    // auto builder = Msg::Packet::Build(Msg::Opcodes::Echo);
    // const auto* packet = builder.Append(1377).Append("file.bin").Complete();
    // Net::Socket sock;
    // sock.Send(packet->RawPtr(), packet->GetSize());

    // // Server
    // char buffer[256];
    // size_t size = sock.Receive(buffer, sizeof(Msg::Packet));

    // const Msg::Packet* pack = reinterpret_cast<const Msg::Packet*>(buffer);

    // if (pack->GetDataSize() > 0) {
    //     sock.Receive(buffer + sizeof(Msg::Packet), pack->GetDataSize());

    //     const auto* msg = pack->GetDataAs<Msg::UploadMsg>();
    // }

    // Console initialization
    CommandSet commandSet;
    commandSet.RegisterCommand("hello", "Prints \'Hi!\' with <name>", helloCmd);
    commandSet.RegisterCommand("connect", "Connecting to the server with <ip> and <port>", connect);
    commandSet.RegisterCommand("ECHO", "Returns <msg> from server", echo);
    commandSet.RegisterCommand("TIME", "Returns current time from server", time);
    commandSet.RegisterCommand("DOWNLOAD", "Downloading file <name> from srver", download);
    commandSet.RegisterCommand("UPLOAD", "Uploading file <name> to server", upload);

    StdIoConsoleStream stream;
    Console console(stream, commandSet);

    while (console.IsShouldExit() == false) {
        console.HandleCommand();
    }

    return 0;
}
