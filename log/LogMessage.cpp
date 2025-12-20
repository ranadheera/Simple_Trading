#include "LogMessage.h"


ConnectionSuccess1 ConnectionSuccess1::make(std::string_view hostName, int port)
{
    ConnectionSuccess1 msg;
    msg.type_ = MessageType::CONNECTION_SUCCESS_1;
    auto size = std::min(hostName.size(), (size_t)50);
    std::memcpy(msg.hostName_, hostName.data(), size);
    msg.hostName_[size] = '\0';
    msg.port_ = port;
    return msg;
}

FileEnd1 FileEnd1::make()
{
    FileEnd1 msg;
    msg.type_ = MessageType::FILE_END_1;
    return msg;
}
