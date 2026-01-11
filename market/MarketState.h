#ifndef MARKET_STATE_H
#define MARKET_STATE_H

#include <list>
#include "MarketData.h"
#include "Liquidity.h"
#include "L2Book.h"
#include "ParsedFixMessage.h"
#include "Events.h"

class L1Book
{
public:
    L1Book() = default;

    void setBestBidPrice(Price price) { bestBidPrice_ = price; }
    void setBestAskPrice(Price price) { bestAskPrice_ = price; }
    void setBestBidVolume(Volume volume) { bestBidVolume_ = volume; }
    void setBestAskVolume(Volume volume) { bestAskVolume_ = volume; }
    void setLastTrade(const Trade &trade) {lastTrade_ = trade; }
    void setUpdatedTime(TimeStamp time) { time_ = time; }

    Price getBestBidPrice() const { return bestBidPrice_; }
    Price getBestAskPrice() const { return bestAskPrice_; }
    const Trade& getLastTrade() const { return lastTrade_; }
    Volume getBestBidVolume() const { return bestBidVolume_; }
    Volume getBestAskVolume() const { return bestAskVolume_; }
    TimeStamp getTime() const { return time_; }

private:
    Price bestBidPrice_ = 0;
    Price bestAskPrice_ = 0;
    Volume bestBidVolume_ = 0;
    Volume bestAskVolume_ = 0;
    Trade lastTrade_;
    TimeStamp time_ = 0;
};

class SymbolMarketState
{
public:
    SymbolMarketState(std::size_t tradeSize = 512, std::size_t l2BookSize = 50, double l2BookTickSize = 0.01);

public:
    bool setL2BookSize(std::size_t size) { return l2Book_.setSize(size); }
    bool setL2BookTickSize(double tickSize) { return l2Book_.setTickSize(tickSize); }
    bool setTradeSize(std::size_t size); 
    bool init();
    const L2Book& getL2Book() const { return l2Book_; }
    const L1Book& getL1Book() const { return l1Book_; }
    const std::vector<Trade> getTrades() const { return trades_; }
    std::size_t getTradeSeuence() const { return tradeSeqNo_.load(std::memory_order_acquire); }
    std::size_t getUpdateCount() const { return updateCount_.load(std::memory_order_acquire);}
    bool update(const std::vector<Marketdata> &marketdata,  const std::vector<Trade> &trades);
private:
    bool update(const Marketdata &data);
    void update(const Trade &data);
private:
    std::size_t tradeSize_;
    std::atomic<std::size_t> tradeSeqNo_;
    std::atomic<std::size_t> updateCount_;
    std::vector<Trade> trades_;
    L2Book l2Book_;
    L1Book l1Book_;
    bool initialized_ = false;

};

class MarketChangeEvent : public EventBase
{
public:
    MarketChangeEvent(SymbolID symbolID) : EventBase(EventType::MARKET_CHANGE), symbolID_(symbolID) {}
    SymbolID getSymbolID() { return symbolID_; }
private:
    SymbolID symbolID_;
};

class MarketState
{
public:
    MarketState(std::size_t symbolCount) : symbolMarketStates_(symbolCount), symbolChangeStatus_(symbolCount, false) {}
public:
    SymbolMarketState& getSymbolMarketState(std::size_t symbolId) { return symbolMarketStates_[symbolId]; }
    void update(const ParsedFixMarketData& fixMarketData);
    void registerForMarketChanges(const Subscriber& subscriber) { marketChangeSubscribers_.push_back(subscriber); }
    bool init();
private:
    std::vector<SymbolMarketState> symbolMarketStates_;
    std::vector<char> symbolChangeStatus_;
    std::list<Subscriber> marketChangeSubscribers_;
    bool initiaLized_ = false;
};

#endif