#include "StrategyEngine.h"

MeanReversionNumData::MeanReversionNumData(std::size_t size, std::size_t volum): array_(size), volume_(volum)
{

}

void MeanReversionNumData::handleMarketData(const Marketdata &data)
{
    Marketdata oldval = array_[writeIndex_];
    array_[writeIndex_] = data;
    writeIndex_ = (writeIndex_ + 1) % array_.size();
    auto price = (data.getAskPrice() + data.getBidPrice()) / 2;
    double mean;
    if (count_ < array_.size()) {
        ++count_;
        totVal_ += price;

        if (count_ != array_.size())
            return;

        mean = totVal_ / count_;
        
    } else if (count_ == array_.size()) {
        totVal_ += price;
        auto lastPrice = (oldval.getAskPrice() + oldval .getBidPrice()) / 2;
        totVal_ -= lastPrice;
        mean = totVal_ / count_;
    }

    if (price > mean) {
        // "sell";
    } else if (price < mean) {
        // "Buy";
    }

}

void MeanReversionNumData::onMarketData(void *strategy, const Marketdata &data)
{
    reinterpret_cast<MeanReversionNumData*>(strategy)->handleMarketData(data);
}

void MeanReversionNumData::destroy(void *strategy)
{
    delete reinterpret_cast<MeanReversionNumData*>(strategy);
}


StrategyExecuter::StrategyExecuter(StrategyEngine &engine, MarketTick &marketTick, std::size_t size) : engine_(engine), marketTick_(marketTick), buffer_(size) {}


void StrategyExecuter::exec() 
{
    int id;
    Marketdata data;
    while (runFlag_ || !buffer_.empty()) {
        if (buffer_.read(id)) {
            if (marketTick_.getData(id, data)) {
                auto swrapper = engine_.getStrategyWrapper(id);
                if (swrapper) {
                    swrapper->execFunc_(swrapper->strategy_, data);
                }
            }
        }

    }
    engine_.activeThreadCount_.fetch_sub(1, std::memory_order_acq_rel);
}

StrategyEngine::StrategyEngine(MarketTick &marketTick, std::size_t numSymbols, std::size_t numThreads): strategies_(numSymbols), marketTick_(marketTick), executers_(numThreads), execThreads_( numThreads) {
    for (int i = 0; i < numSymbols; ++i) {
        void *strategy = new MeanReversionNumData(20, 10);
        StrategyWrapper wrapper;
        wrapper.strategy_ = strategy;
        wrapper.deleter_ = &MeanReversionNumData::destroy;
        wrapper.execFunc_ =  &MeanReversionNumData::onMarketData;
        addStrategy(i, wrapper);
    }
}

StrategyEngine::~StrategyEngine()
{
    for (auto &stratagy : strategies_) {
       if (stratagy.strategy_) {
            stratagy.deleter_(stratagy.strategy_);
            stratagy.strategy_ = nullptr;
       }
    }

    for (auto executer : executers_) {
        delete executer;
    }
}

void StrategyEngine::addStrategy(int symbolId, const StrategyWrapper &wrapper)
{
    strategies_[symbolId] = wrapper;
}

void StrategyEngine::start()
{
    for (int i = 0; i < executers_.size(); ++i) {
        executers_[i] = new StrategyExecuter(*this, marketTick_, 1000);
        execThreads_[i] = std::thread(&StrategyExecuter::exec, executers_[i]);
    }
    activeThreadCount_.store(executers_.size(), std::memory_order_release);
}

void StrategyEngine::stop()
{
   for (auto executer : executers_) {
        executer->setStop();
   }

   while (activeThreadCount_.load(std::memory_order_acquire));

   for (auto &thread : execThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
   }
}

void StrategyEngine::update(int marketTickIndex)
{
    int execIndex = marketTickIndex % executers_.size();
    executers_[execIndex]->addSymbolID(marketTickIndex);
}

StrategyWrapper* StrategyEngine::getStrategyWrapper(int symbolId)
{
    if (strategies_[symbolId].strategy_) {
        return &strategies_[symbolId];
    }
    return nullptr;
}


