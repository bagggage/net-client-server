#include "args.h"
#include "console.h"
#include "socket.h"
#include "packet.h"
#include "client.h"

#include <iostream>

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

    std::string response = client.SendEcho(message);
    if (response == "") {
        std::cout << "No response from server with ECHO\n";
        return;
    }

    std::cout << response << "\n";
}

void time() {
    Client& client = Client::GetInstance();

    std::time_t response = client.SendTime();
    if (response == 0) {
        std::cout << "No response from server with TIME\n";
        return;
    }

    std::cout << "Server time: " << std::ctime(&response);
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

void close() {
    Client& client = Client::GetInstance();

    if (!client.SendClose()) {
        std::cout << "Error while sending COMMAND command\n";
    }
}

void disconnect() {
    Client& client = Client::GetInstance();

    client.Close();
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
    commandSet.RegisterCommand("DISCONNECT", "Closing the client connection", disconnect);
    commandSet.RegisterCommand("CLOSE", "Sending close command to server", close);

    StdIoConsoleStream stream;
    Console console(stream, commandSet);

    while (console.IsShouldExit() == false) {
        console.HandleCommand();
    }

    return 0;
}
