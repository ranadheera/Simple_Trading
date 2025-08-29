#ifndef DISPATCHER_H
#define DISATCHER_H

#include "RingBuffer.h"
#include "Buffer.h"
#include "Updaters.h"
#include "MarketTick.h"
#include "StrategyEngine.h"
#include <thread>

class Dispatcher
{
public:
    Dispatcher(SWSRRingBuffer<Marketdata> &ringBuffer, MarketTick &marketTick, BookUpdaters &bookUpdaters, StrategyEngine &strategyEngine) : 
    ringBuffer_(ringBuffer), marketTick_(marketTick), bookupdatrs_(bookUpdaters), strategyEngine_(strategyEngine), numUpdaters_(bookUpdaters.getNumUpdaters()) {}
    void start();
    void stop();
    void exec();
private:
    SWSRRingBuffer<Marketdata> &ringBuffer_;
    MarketTick &marketTick_;
    BookUpdaters &bookupdatrs_;
    StrategyEngine &strategyEngine_;
    std::size_t numUpdaters_;
    std::thread dispatcherThread_;
    bool runFlag_ = true;
};

#endif