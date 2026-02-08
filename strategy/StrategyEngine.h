#ifndef STRATEGY_ENGINE_H
#define STRATEGY_ENGINE_H

#include "MarketData.h"
#include "MarketTick.h"
#include "Buffer.h"
#include "SWSRRingBuffer.h"
#include <vector>
#include <functional>
#include <thread>


class  MeanReversionNumData {
public:
    MeanReversionNumData(std::size_t size, std::size_t );
    void handleMarketData(const Marketdata &data);
    static void onMarketData(void *strategy, const Marketdata &data);
    static void destroy(void *strategy);
private:
    std::size_t count_ = 0;
    std::size_t writeIndex_ = 0;
    std::size_t volume_;
    double totVal_ = 0;
    std::vector<Marketdata> array_;

};

struct StrategyWrapper {
    void* strategy_ = 0;
    std::function<void(void*, const Marketdata&)> execFunc_;
    std::function<void(void*)> deleter_;
};

using SymbolIdBuffer = SWSRRingBuffer<int>;
class StrategyExecuter;

class StrategyEngine {
friend StrategyExecuter;
public:
    StrategyEngine(MarketTick &marketTick_, std::size_t numSymbols, std::size_t threads);
    ~StrategyEngine();
    void update(int marketTickIndex);
    void start();
    void stop();
    StrategyWrapper* getStrategyWrapper(int symbolId);
private:
    void addStrategy(int symbolId, const StrategyWrapper &wrapper);
private:
    MarketTick &marketTick_;
    std::vector<StrategyWrapper> strategies_;
    std::vector<StrategyExecuter*> executers_;
    std::vector<std::thread> execThreads_;
    std::atomic<int> activeThreadCount_;
};

class StrategyExecuter
{

public:
    StrategyExecuter(StrategyEngine &engine, MarketTick &marketTick, std::size_t size);
public:
    void addSymbolID(int id) { buffer_.write(id); }
    void exec();
    void setStop() {runFlag_ = false;}
private:
    StrategyEngine &engine_;
    MarketTick &marketTick_;
    SymbolIdBuffer buffer_;
    bool runFlag_ = true;
};

#endif