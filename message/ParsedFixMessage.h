#ifndef PARSED_FIX_MESSAGE_H
#define PARSED_FIX_MESSAGE_H

#include <vector>
#include <MarketData.h>

class SymbolMarketData
{
public:
    SymbolMarketData() : marketData_(64), trades_(64) { }
    void addMarketData(const Marketdata& data) { marketData_.push_back(data); }
    const std::vector<Marketdata>& getMarketData() const { return marketData_; }
    void addTrade(const Trade& trade) { trades_.push_back(trade);}
    const std::vector<Trade>& getTrades() const { return trades_; }
    void reset() { marketData_.clear(); trades_.clear(); }
private:
    std::vector<Marketdata> marketData_;
    std::vector<Trade> trades_;
};

class ParsedFixMarketData
{
public:
    ParsedFixMarketData(std::size_t numSymbols) : fixMarketData_(numSymbols) {}
    const SymbolMarketData& getFixMarketData(SymbolID id) const { return fixMarketData_[id]; }
    SymbolMarketData& getFixMarketData(SymbolID id) { return fixMarketData_[id]; }
    void addMarketData(const Marketdata& data) { fixMarketData_[data.getSymbolID()].addMarketData(data); }
    void addTrade(const Trade& trade) { fixMarketData_[trade.getSymbolID()].addTrade(trade);}
    void reset();
private:
    std::vector<SymbolMarketData> fixMarketData_;
};

#endif