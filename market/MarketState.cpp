#include "MarketState.h"


SymbolMarketState::SymbolMarketState(std::size_t tradeSize, std::size_t l2BookSize, double l2BookTickSize) :
    tradeSize_(tradeSize), trades_(0), l2Book_(l2BookSize, l2BookTickSize)
{
    tradeSeqNo_.store(0, std::memory_order_release);
    updateCount_.store(0, std::memory_order_release);
}

bool SymbolMarketState::update(const std::vector<Marketdata> &marketdata,  const std::vector<Trade> &trades)
{
    updateCount_.fetch_add(1, std::memory_order_acq_rel);
    bool updated = false;

    for (auto &data : marketdata) {
        updated |= update(data);
    }

    for (auto &trade: trades) {
        update(trade);
        updated = true;
    }
    
    updateCount_.fetch_add(1, std::memory_order_acq_rel);
    return true;

}


bool SymbolMarketState::update(const Marketdata &data)
{
    auto updated =  l2Book_.update(data);

    if (!updated)
        return updated;

    if (l2Book_.isBestAskChangeWithLastUpdate()) {

        l1Book_.setUpdatedTime(data.getTimeStamp());
        auto topAsk = l2Book_.getMinAskPriceVolume();
        l1Book_.setBestAskPrice(topAsk.first);
        l1Book_.setBestAskVolume(topAsk.second);
    }

    return true;
}

void SymbolMarketState::update(const Trade &data)
{
    auto seqNo = tradeSeqNo_.load(std::memory_order_acquire);
    auto indexNo = seqNo % tradeSize_;
    trades_[indexNo] = data;
    tradeSeqNo_.fetch_add(1, std::memory_order_acq_rel);

    l1Book_.setLastTrade(data);
    l1Book_.setUpdatedTime(data.getTimeStamp());

}

bool SymbolMarketState::setTradeSize(std::size_t size)
{
    if (!initialized_)
        return false;

    tradeSize_ = size;
    return true;
}

bool SymbolMarketState::init() {

    if (initialized_)
        return false;

    if (!l2Book_.init())
        return false;

    trades_.resize(tradeSize_);
    return true;
}

bool MarketState::init()
{
    bool initiaLized = true;

    for (auto &symbolstate : symbolMarketStates_) {
        initiaLized_ &= symbolstate.init();
    }

    return initiaLized_;
}

void MarketState::update(const ParsedFixMarketData& fixMarketData)
{
    auto symbolCount = symbolMarketStates_.size();

    for (auto i = 0; i < symbolCount; ++i) {
        auto &symbolMarketData = fixMarketData.getFixMarketData(i);
        auto &marketdata = symbolMarketData.getMarketData();
        auto &trade = symbolMarketData.getTrades();

        if (marketdata.size() || trade.size())
            symbolChangeStatus_[i] = symbolMarketStates_[i].update(marketdata, trade);
        else
            symbolChangeStatus_[i] = false;
    }

    for (auto i = 0; i < symbolCount; ++i) {
        if (symbolChangeStatus_[i]) {
            for (auto &subscriber : marketChangeSubscribers_) {
                subscriber.notify(MarketChangeEvent(i));
            }
        }
    }

}
