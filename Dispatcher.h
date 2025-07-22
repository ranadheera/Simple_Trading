#ifndef DISPATCHER_H
#define DISATCHER_H

#include "RingBuffer.h"
#include "Buffer.h"
#include "Updaters.h"
#include <thread>

class Dispatcher
{
public:
    Dispatcher(RingBuffer<Marketdata> &ringBuffer, UpdatersList &updaterList) : 
    ringBuffer_(ringBuffer), updaterList_(updaterList), numUpdaters_(updaterList.getNumUpdaters()) {}
    void start();
private:
    RingBuffer<Marketdata> &ringBuffer_;
    UpdatersList &updaterList_;
    std::size_t numUpdaters_;
};

#endif