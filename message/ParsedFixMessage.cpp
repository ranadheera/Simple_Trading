#include "ParsedFixMessage.h"


FixMarketUpdate::FixMarketUpdate(SymbolID id, UpdateAction updateAction, EntryType entryType, TimeStamp time, Price price, Volume volume, int position):
    id_(id), updateAction_(updateAction), entryType_(entryType), time_(time), price_(price), volume_(volume), position_(position) {}


void ParsedFixMarketData::reset()
{
    for (auto &symboldata : fixMarketData_) {
        symboldata.reset();
    }
}

std::ostream& operator<<(std::ostream& os, const BaseFixMessage& message)
{
    os << "MessageType:" << static_cast<int>(message.getMessageType()) << 
    " SenderCompId:" << message.getSenderCompID() <<
    " TargetCmpId:" << message.getTargetCompID() <<
    " MsgSeqNo:" << message.getMessageSeqNum() <<
    " Timestamp:" << message.getTimeStamp();
    return os;
}

std::ostream& operator<<(std::ostream& os, const ParsedFixMarketData& message)
{
    os << static_cast<const BaseFixMessage&>(message) << std::endl;
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