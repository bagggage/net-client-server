#include "net.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <Winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#endif

using namespace Net;

std::string MacAddress::ToString() const {
    std::ostringstream macStream;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (i != 0) macStream << ":";
        macStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return macStream.str();
}

MacAddress Net::GetMacAddress() {
    MacAddress mac;

#ifdef _WIN32
    IP_ADAPTER_INFO AdapterInfo[16];
    DWORD dwBufLen = sizeof(AdapterInfo);
    
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

        while (pAdapterInfo) {
            if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET) { 
                std::copy(pAdapterInfo->Address, 
                          pAdapterInfo->Address + mac.bytes.size(), 
                          mac.bytes.begin());
                break; // Берем первый Ethernet-адаптер
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }

#else
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    
    if (sock == -1) return mac;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        close(sock);
        return mac;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    std::copy(reinterpret_cast<uint8_t*>(ifr.ifr_hwaddr.sa_data),
                              reinterpret_cast<uint8_t*>(ifr.ifr_hwaddr.sa_data) + mac.bytes.size(),
                              mac.bytes.begin());
                    break;
                }
            }
        }
    }
    close(sock);
#endif

    return mac;
}
