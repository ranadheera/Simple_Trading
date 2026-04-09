#include "FixMessage.h"
#include "MessageBuilder.h"

void FixMessageHeader::reset()
{
    msgSeqNum_ = 0;
    senderCompID_[0] = '\0';
    targetCompID_[0] = '\0';
    timestamp_ = 0;
}

void FixLogonMessage::reset()
{
    resetSeqNumFlag_ = ResetSeqNumFlag::Types::UNDEFINED;
    heartBtSecond_ = 0;
    nextExpectedSeqNum_ = 0;
    encryptMethod_ = 0;
}

void FixHeartBeatMessage::reset()
{
    testID_[0] = '\0';
}

FixMarketUpdate::FixMarketUpdate(SymbolID id, UpdateAction::Types updateAction, EntryType::Types entryType, TimeStamp time, Price price, Volume volume, int position):
    id_(id), updateAction_(updateAction), entryType_(entryType), time_(time), price_(price), volume_(volume), position_(position) {}


void FixMarketDataMessage::reset()
{
    for (auto &symboldata : fixMarketData_) {
        symboldata.reset();
    }
}

std::ostream& operator<<(std::ostream& os, const FixMessageHeader& message)
{
    os << "MessageType:" << static_cast<int>(message.getMessageType()) << 
    " SenderCompId:" << message.getSenderCompID() <<
    " TargetCmpId:" << message.getTargetCompID() <<
    " MsgSeqNo:" << message.getMessageSeqNum() <<
    " Timestamp:" << message.getTimeStamp();
    return os;
}

std::ostream& operator<<(std::ostream& os, const FixMarketDataMessage& message)
{
    for (auto &symbolMarketData : message.fixMarketData_) {
        if (!symbolMarketData.getMarketData().empty()) {
            os << symbolMarketData << std::endl;
        }
    }
    return os;
}
std::ostream& operator<<(std::ostream& os, const SymbolMarketData& symbolMarketData)
{
    auto &marketDataList =  symbolMarketData.getMarketData();

    for (auto &marketData : marketDataList) {
        os << marketData << std::endl;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const FixMarketUpdate& marketUpdate)
{
    os << "SymbolId:" << static_cast<int>(marketUpdate.getSymbolID()) <<
    " UpdateAction:" << static_cast<int>(marketUpdate.getUpdateAction()) <<
    " EntryType: " << static_cast<int>(marketUpdate.getEntryType()) <<
    " Price: " << marketUpdate.getPrice() <<
    " Volume: " << marketUpdate.getVolume() <<
    " Position: " << marketUpdate.getPosition();

    return os;
}

std::ostream& operator<<(std::ostream& os, const FixLogonMessage& message)
{
    os << " ResetSeqNumFlag:" << static_cast<int>(message.getResetSeqNumFlag()) <<
    " HeartBeatInterval:" << message.getHeartBeatInterval() <<
    " NextExpectedSeqNum:" << message.getNextExpectedSeqNum();
    return os;
}

std::ostream& operator<<(std::ostream& os, const FixHeartBeatMessage& message)
{
    os << " TestId: " << message.getTestID();
    return os;
}

bool FixLogonMessage::convertToOutMessage( OutMessage &message, MessageBuilder &messageBuilder) const
{
    
    if (!userName_.empty() && !passWord_.empty()) {
        auto st = messageBuilder.addTagValue(message, Username::name_, Username::length_, userName_.data(), userName_.length());

        if (st != MessageBuilder::UpdateStatus::SUCCESS)
            return false;

        st = messageBuilder.addTagValue(message, Password::name_, Password::length_, passWord_.data(), passWord_.length());

        if (st != MessageBuilder::UpdateStatus::SUCCESS)
            return false;
    
    }

    auto st = messageBuilder.addTagValue(message, EncryptMethod::name_, EncryptMethod::length_, encryptMethod_);

    if (st != MessageBuilder::UpdateStatus::SUCCESS)
        return false;

    st = messageBuilder.addTagValue(message, HeartBtInt::name_, HeartBtInt::length_, heartBtSecond_);

    if (st != MessageBuilder::UpdateStatus::SUCCESS)
        return false;

    return true;
}