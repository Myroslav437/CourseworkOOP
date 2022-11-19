// Compile only for UNIX-like systems:
#if defined(unix) || defined(__unix__) || defined(__unix)

#include "../includes/srfc_connection.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>

#include <stdexcept>

#include "../../utilities/array_deleter.hpp"

namespace net
{

std::vector<char> srfc_connection::__receive__() 
{
    constexpr std::size_t bufSize = 1024;
    char buf[bufSize] = {0};

    std::size_t bytes_received = ::read(this->socket_fd, buf, bufSize);
    if(bytes_received < 0) {
        throw std::runtime_error("Read error"); // add errror code
    }
    // if bytes_received == 0 -> connection closed.

    return std::vector<char>(buf, buf + bytes_received);
}

void srfc_connection::__connect__(unsigned int port, std::string address)
{
    int sock = 0;
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Create socket:
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
    if(send(this->socket_fd, buf, len, 0) < 0) {
        throw std::runtime_error("__send__(const void *buf, std::size_t len): The send() function failed:");  
    }
}  

void srfc_connection::__shutdown__() 
{
    if(::shutdown(this->socket_fd, SHUT_RDWR) < 0) {
        throw std::runtime_error("__shutdown__(): The shutdown() function failed:");
    }
}
    
void srfc_connection::__close__()
{
    if(::close(this->socket_fd) < 0) {
        throw std::runtime_error("__close__(): The close() function failed:"); 
    }
}

} // namespace net

#endif