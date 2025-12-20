#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <sys/socket.h>

using socketfd = int;

class Connection {
public:
    enum class Status {CONNECTED, NOTCONNECTED, CLOSED};
    enum class IpvType {IPV4, IPV6};
public:
    Connection(const std::string &host, int port, IpvType ipvType = IpvType::IPV4);
    ~Connection();
    bool connect();
    int send(const char *message, int length);
    int receive(char *message, int length);
    void disconnect();
private:
    std::string host_;
    int port_;
    IpvType ipvType_;
    Status status_ = Status::NOTCONNECTED;
    socketfd fd_ = -1;
};

#endif