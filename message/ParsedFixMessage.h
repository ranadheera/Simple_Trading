#ifndef PARSED_FIX_MESSAGE_H
#define PARSED_FIX_MESSAGE_H

#include <vector>
#include "MarketData.h"
#include "Message.h"
#include <cstring>
#include <ostream>

enum class FixMessageType { MARKET_DATA, UNDEFINED};


class BaseFixMessage
{
protected:
    BaseFixMessage(FixMessageType type) : type_(type) { senderCompID_[0] = '\0'; targetCompID_[0] = '\0';}
public:
    FixMessageType getMessageType() const { return type_; }
    void setTimeStamp(TimeStamp time) { timestamp_ = time; }
    TimeStamp getTimeStamp() const { return timestamp_; }
    void setMessageSeqNum(std::size_t msgSeqNum) {  msgSeqNum_ =  msgSeqNum; }
    std::size_t getMessageSeqNum() const { return msgSeqNum_; }
    void setSenderCompID(char *senderCompID, int length) { memcpy(senderCompID_, senderCompID, length); senderCompID_[length] = '\0';}
    const char* getSenderCompID() const { return senderCompID_; }
    void setTargetCompID(char *targetCompID, int length) { memcpy(targetCompID_, targetCompID, length); targetCompID_[length] = '\0'; }
    const char* getTargetCompID() const { return targetCompID_; }
private:
    FixMessageType type_ = FixMessageType::UNDEFINED;
    std::size_t msgSeqNum_ = 0;
    char senderCompID_[30];
    char targetCompID_[30];
    TimeStamp timestamp_ = 0;
};

class FixMarketUpdate
{
public:
    FixMarketUpdate(SymbolID id, UpdateAction updateAction, EntryType entryType, TimeStamp time, Price price, Volume volume, int position);
    FixMarketUpdate() = default;
    void setSymbolID(SymbolID id) { id_ = id; }
    SymbolID getSymbolID() const { return id_; }
    void setUpdateAction(UpdateAction action)  { updateAction_ = action; }
    UpdateAction getUpdateAction() const { return updateAction_; }
    void setEntryType(EntryType entryType) { entryType_ = entryType;}
    EntryType getEntryType() const { return entryType_; }
    void setTimestamp(TimeStamp time) { time_ = time; }
    TimeStamp getTimeStamp() const { return time_; }
    void setPrice(Price price) { price_ = price; }
    Price getPrice() const { return price_; }
    void setVolume(Volume volume) {volume_ = volume; }
    Volume getVolume() const { return volume_; }
    void setPosition(int position) { position_ = position; }
    int getPosition() const { return position_;}
private:
    SymbolID id_ = NoSymbolID;
    UpdateAction updateAction_ = UpdateAction::UNDEFINED;
    EntryType entryType_ = EntryType::UNDEFINED;
    TimeStamp time_ = 0;
    Price price_ = 0;
    Volume volume_ = 0;
    int position_  = -1;
};

class SymbolMarketData
{
public:
    SymbolMarketData() : marketData_(64) { }
    void addMarketData(const FixMarketUpdate& data) { marketData_.push_back(data); }
    const std::vector<FixMarketUpdate>& getMarketData() const { return marketData_; }
    void reset() { marketData_.clear();}
private:
    std::vector<FixMarketUpdate> marketData_;
};

class ParsedFixMarketData : public BaseFixMessage
{
    friend std::ostream& operator<<(std::ostream& os, const ParsedFixMarketData& marketData);
public:
    ParsedFixMarketData(std::size_t numSymbols) : BaseFixMessage(FixMessageType::MARKET_DATA), fixMarketData_(numSymbols) {}
    const SymbolMarketData& getFixMarketData(SymbolID id) const { return fixMarketData_[id]; }
    SymbolMarketData& getFixMarketData(SymbolID id) { return fixMarketData_[id]; }
    void addMarketData(const FixMarketUpdate& data) { fixMarketData_[data.getSymbolID()].addMarketData(data); }
    void reset();
private:
    std::vector<SymbolMarketData> fixMarketData_;
};

std::ostream& operator<<(std::ostream& os, const BaseFixMessage& message);
std::ostream& operator<<(std::ostream& os, const ParsedFixMarketData& message);
std::ostream& operator<<(std::ostream& os, const SymbolMarketData& symbolMarketData);
std::ostream& operator<<(std::ostream& os, const FixMarketUpdate& marketUpdate);

#endif
