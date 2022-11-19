// Compile only for Windows-like systems:
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#undef UNICODE
#define WIN32_LEAN_AND_MEAN


#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

// #pragma comment (lib, "Mswsock.lib")

#include "../includes/srfc_connection.hpp"

namespace net
{

std::vector<char> srfc_connection::__receive__() 
{
    constexpr std::size_t bufSize = 1024;
    char buf[bufSize] = {0};

    std::size_t bytes_received = ::recv(this->socket_fd, buf, bufSize, 0);
    if(bytes_received < 0) {
        throw std::runtime_error("__receive__(): recv function failed"); // add errror code
    }
    // if bytes_received == 0 -> connection closed.
    return std::vector<char>(buf, buf + bytes_received);
}

void srfc_connection::__connect__(unsigned int port, std::string address)
{
    WSADATA wsaData;
    auto sock = static_cast<socket_t>(INVALID_SOCKET);
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = ::htons(port);

    // Initialize Winsock
    // From docs.microsoft.com: An application can call WSAStartup more than once if 
    // it needs to obtain the WSADATA structure information more than once.
    if (::WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        throw std::runtime_error("__connect__(unsigned int port, std::string address): WSAStartup error");
    }

    // Create socket:
    if ((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        throw std::runtime_error("__connect__(unsigned int port, std::string address): Socket creation error");
    }

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) <= 0) {
        throw std::runtime_error(" __connect__(unsigned int port, std::string address): Invalid address / Address not supported");
    }

    // Connect to the remote server:
    if ((::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        throw std::runtime_error("__connect__(unsigned int port, std::string address): Connection Failed");
    }

    this->socket_fd = sock;
    this->connected.store(true);
}

void srfc_connection::__send__(const void *buf, std::size_t len)
{
    if(::send(this->socket_fd, reinterpret_cast<const char*>(buf), len, 0) 
        == SOCKET_ERROR) 
    {
        throw std::runtime_error("__send__(const void *buf, std::size_t len): The send() function failed:");  
    }
}  

void srfc_connection::__shutdown__() 
{
    if(::shutdown(this->socket_fd, SD_BOTH) == SOCKET_ERROR) {
        throw std::runtime_error("__shutdown__(): The shutdown() function failed:");
    }
}
    
void srfc_connection::__close__()
{
    if(::closesocket(this->socket_fd) == SOCKET_ERROR) {
        throw std::runtime_error("__close__(): The closesocket() function failed:"); 
    }
}

} // namespace net

#endif