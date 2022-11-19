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
#include "../includes/srfc_listener.hpp"
#include "../includes/utilities/array_deleter.hpp"

namespace net
{

void srfc_listener::__bind__(unsigned int port, std::string interf)
{
    // sets socket_t socket_fd
    // sets std::atomic_bool binded;

    WSADATA wsaData;

    struct sockaddr_in address;
    auto server_fd = static_cast<socket_t>(INVALID_SOCKET);
    constexpr std::size_t backlog = 64;
    int opt = 1;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        throw std::runtime_error("__bind__(unsigned int port, std::string interf): WSAStartup error");
    }

    // Creating socket file descriptor
    if ((server_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The socket() function failed:");  
    }

    // Forcefully attaching socket to the port
    if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, 
        reinterpret_cast<char*>(&opt), sizeof(opt)) == SOCKET_ERROR) 
    {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The setsockopt() function failed:");  
    }

    address.sin_family = AF_INET;
    address.sin_port = ::htons(port);
    if(!interf.empty()) {
        address.sin_addr.s_addr = ::inet_addr(interf.c_str());
    }

    // Forcefully attaching socket to the port
    if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The bind() function failed:");  
    }

    if (::listen(server_fd, backlog) == SOCKET_ERROR) {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The listen() function failed:");  
    }

    this->socket_fd = server_fd;
    this->binded.store(true);
}

srfc_listener::socket_t srfc_listener::__accept__()
{
    int new_socket = 0;
    struct sockaddr_in address = {0};
    int addrlen = sizeof(address);

    if((new_socket = 
        ::accept(this->socket_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) == INVALID_SOCKET) 
    {
        throw std::runtime_error("__accept__(): The accept() function failed:");  
    }

    return new_socket;
}

void srfc_listener::__shutdown__()
{
    if(::shutdown(this->socket_fd, SD_BOTH) == SOCKET_ERROR) {
        throw std::runtime_error("__shutdown__(): The shutdown() function failed:");
    }
}

void srfc_listener::__close__()
{
    if(::closesocket(this->socket_fd) == SOCKET_ERROR ) {
        throw std::runtime_error("__close__(): The closesocket() function failed:"); 
    }
}
} // namespace net

#endif