// Compile only for UNIX-like systems:
#if defined(unix) || defined(__unix__) || defined(__unix)

#include "../includes/srfc_listener.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

#include <stdexcept>

#include "../includes/srfc_connection.hpp"
#include "../includes/utilities/array_deleter.hpp"

namespace net
{

void srfc_listener::__bind__(unsigned int port, std::string interface)
{
    // sets socket_t socket_fd
    // sets std::atomic_bool binded;

    constexpr std::size_t backlog = 64;

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
  
    // Creating socket file descriptor
    if ((server_fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The socket() function failed:");  
    }
  
    // Forcefully attaching socket to the port
    if (::setsockopt(server_fd, SOL_SOCKET, 
        SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The setsockopt() function failed:");  
    }

    address.sin_family = AF_INET;
    address.sin_port = ::htons(port);
    if(!interface.empty()) {
        address.sin_addr.s_addr = ::inet_addr(interface.c_str());
    }

  
    // Forcefully attaching socket to the port
    if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("__bind__(unsigned int port, std::string address): The bind() function failed:");  
    }

    if (::listen(server_fd, backlog) < 0) {
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
        ::accept(this->socket_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) 
    {
        throw std::runtime_error("__accept__(): The accept() function failed:");  
    }

    return new_socket;
}

void srfc_listener::__shutdown__()
{
    if(::shutdown(this->socket_fd, SHUT_RDWR) < 0) {
        throw std::runtime_error("__shutdown__(): The shutdown() function failed:");
    }
}

void srfc_listener::__close__()
{
    if(::close(this->socket_fd) < 0) {
        throw std::runtime_error("__close__(): The close() function failed:"); 
    }
}

} // namespace net

#endif