#include "MessageBuilder.h"
#include <cstring>
#include <chrono>


bool LoginMessageBuilder::buildMessage(OutMessage &message)
{
    auto retVal =  message.addTagValue(FixTags::MsgType, 'A');

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    retVal = message.addTagValue(FixTags::SenderCompID, senderCompId_);

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    retVal = message.addTagValue(FixTags::TargetCompID, senderCompId_);

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    if (!userName_.empty() && !passWord_.empty()) {
        retVal = message.addTagValue(FixTags::Username, userName_);

        if (retVal != OutMessage::UpdateStatus::SUCCESS)
            return false;

        retVal = message.addTagValue(FixTags::Password, passWord_);
    }

    retVal = message.addTagValue(FixTags::EncryptMethod, encryptMethod_);

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    retVal = message.addTagValue(FixTags::HeartBtInt, hearBtInt_);

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    return true;
    
}