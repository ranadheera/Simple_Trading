#include "Connection.h"
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fcntl.h> 
#include <unistd.h>
#include <memory>
 
Connection::Connection(const std::string &host, int port, IpvType ipvType) : host_(host), port_(port), ipvType_(ipvType)
{

}

 Connection::~Connection()
 {
    disconnect();
 }

 void Connection::disconnect()
 {

    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }

    status_ = Status::NOTCONNECTED;
 }

bool Connection::connect()
{
    struct addrinfo hints, *addres = nullptr;

    // Prepare hints
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = (ipvType_ == IpvType::IPV4) ? AF_INET : AF_INET6;       // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    // Resolve host
    std::string port_str = std::to_string(port_);

    if (getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &addres) != 0) {
        std::cout << "getaddrinfo failed for host: " << host_ <<  " " << port_ << std::endl;
        return false;
    }
    
    std::unique_ptr<addrinfo, void(*)(addrinfo*)> res_ptr(addres, freeaddrinfo);

    // Create socket
    fd_ = socket(addres->ai_family, addres->ai_socktype, addres->ai_protocol);

    if (fd_ < 0) {
        perror("socket");
        return false;
    }

    // Set non-blocking
    int flags = fcntl(fd_, F_GETFL, 0);

    if (flags < 0 || fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl");
        return false;
    }

    // Try to connect
    int ret = ::connect(fd_, addres->ai_addr, addres->ai_addrlen);

    if (ret < 0) {
        if (errno == EINPROGRESS) {
            // This is expected for non-blocking sockets
            std::cout << "Connection in progress..." << std::endl;
        } else {
            perror("connect");
            close(fd_);
            fd_ = -1;
            return false;
        }
    } else {
        std::cout << "Connected immediately!" << std::endl;
    }

    status_ = Status::CONNECTED;
    return true;
}

int Connection::send(const char *message, int length)
{
    return ::send(fd_, message, length, 0);
}

int Connection::receive(char *message, int length)
{
    return ::recv(fd_, message, length, 0);
}