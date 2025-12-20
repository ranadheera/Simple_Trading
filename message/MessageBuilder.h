#ifndef MESSAGE_BUILDER_H
#define MESSAGE_BUILDER_H

#include <string>
#include "Message.h"

class LoginMessageBuilder 
{
public:
    bool buildMessage(OutMessage &message);
    void setSenderCompID(std::string_view id) { senderCompId_ = id; }
    void setTargetCompID(std::string_view id) { targetCompID_= id; }
    void setUserName(std::string_view userName) { userName_ = userName; }
    void setPassWord(std::string_view passWord) { passWord_ = passWord; }
    void setEncryptMethod(int encryptMethod) { encryptMethod_ = encryptMethod; }
    void setHeartBtInt(int hearBtInt) { hearBtInt_ = hearBtInt; }
private:
    std::string_view senderCompId_;
    std::string_view targetCompID_;
    std::string_view userName_;
    std::string_view passWord_;
    int encryptMethod_ = 0;
    int hearBtInt_ = 30;
};

#endif