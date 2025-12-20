#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

#include <string>
#include <cstring>
#include <cstdint>
#include <memory>
 
enum class MessageType : uint32_t
{
    CONNECTION_SUCCESS_1 = 0,
    FILE_END_1,
    COUNT
};

struct LogMessage
{
    constexpr static std::size_t dataSize_ = 64;
    char data_[dataSize_];
};


template<typename T> concept ValidLogMessage = requires {
    validateMessage(std::declval<T>());
};

template<typename T> constexpr bool validateMessage(const T &t)
{
   return (sizeof(T) <= LogMessage::dataSize_ &&
    std::is_trivial_v<T> &&
    std::is_standard_layout_v<T> &&
    offsetof(T, type_) == 0 &&
    std::is_same_v<decltype(t.type_), MessageType>);
}


template<ValidLogMessage T> inline void convertToLogMessage(const T& msg, LogMessage &logMsg)
{
    memcpy(logMsg.data_, &msg, sizeof(msg));
}

class ConnectionSuccess1
{
template<typename T> friend constexpr bool validateMessage(const T &t);
public:
    static ConnectionSuccess1 make(std::string_view hostName, int port);
    MessageType getType() { return type_; }
    std::string_view getHostName() { return std::string_view(hostName_); }
    int getPort() { return port_; }
private:
    MessageType type_;
    char hostName_[32];
    int32_t port_;
};

class FileEnd1
{
template<typename T> friend constexpr bool validateMessage(const T &t);

public:
    MessageType getType() { return type_; }
    static FileEnd1 make();
private:
    MessageType type_;
};


#endif