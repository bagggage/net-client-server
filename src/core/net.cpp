#include "net.h"

#include <iostream>
#include <vector>
#include <sstream>

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

std::string NetUtils::GetMacAddress() {
    std::ostringstream macAddressStream;
    
#ifdef _WIN32
    IP_ADAPTER_INFO AdapterInfo[16];
    DWORD dwBufLen = sizeof(AdapterInfo);
    
    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

        while (pAdapterInfo) {
            if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET) { 
                for (UINT i = 0; i < pAdapterInfo->AddressLength; i++) {
                    if (i != 0) macAddressStream << ":";
                    macAddressStream << std::hex << std::uppercase << (int)pAdapterInfo->Address[i];
                }
                break; // Возьмем первый Ethernet-адаптер
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }

#else
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    
    if (sock == -1) return "";

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        close(sock);
        return "";
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    unsigned char* mac = (unsigned char*)ifr.ifr_hwaddr.sa_data;
                    for (int i = 0; i < 6; i++) {
                        if (i != 0) macAddressStream << ":";
                        macAddressStream << std::hex << std::uppercase << (int)mac[i];
                    }
                    break;
                }
            }
        }
    }
    close(sock);
#endif

    return macAddressStream.str();
}
