#include "Dispatcher.h"

void Dispatcher::start()
{
    bookupdatrs_.start();
    strategyEngine_.start();
    dispatcherThread_ = std::thread(&Dispatcher::exec, this);
}

void Dispatcher::stop()
{
    runFlag_ = false;
    if (dispatcherThread_.joinable())
        dispatcherThread_.join();
    bookupdatrs_.stop();
    strategyEngine_.stop();
}

void Dispatcher::exec()
{
    Marketdata data;
    while (runFlag_ || !ringBuffer_.empty()) {
        if (ringBuffer_.read(data)) {
            if (marketTick_.update(getIndex(data), data)) {
                bookupdatrs_.sendToUpdater(data);
                strategyEngine_.update(data.getSymbolID());
            }
        }
    }
}