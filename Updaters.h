#ifndef UPDATERS_H
#define UPDATERS_H

#include "SWSRRingBuffer.h"
#include "MarketData.h"
#include "L2Book.h"
#include <thread>

using MarketDataBuffer = SWSRRingBuffer<Marketdata>;
class Updater;

class BookUpdaters
{
    friend Updater;
public:
    BookUpdaters(L2Book &l2book, std::size_t numupdaters, std::size_t bufferSize);
    ~BookUpdaters();
    void start();
    void stop();
    inline Updater* getUpdater(std::size_t index) { return updaters_[index]; }
    auto getNumUpdaters() { return updaters_.size(); }
    void sendToUpdater(const Marketdata& data);
private:
    L2Book &l2book_;
    std::vector<Updater*> updaters_;
    std::vector<std::thread> updaterThreads_;
    std::atomic<int> numUpdaers_;
    std::size_t bufferSize_;
};

class Updater
{
public:
    Updater(BookUpdaters &bookUpdaters, L2Book &l2book, std::size_t size) : bookUpdaters_(bookUpdaters), buffer_(size), l2book_(l2book) {}
public:
    void addData(const Marketdata& data) { buffer_.write(data); }
    void setStopFlag() { runFlag_ = false; }
    void exec() {
        Marketdata t;
        while (runFlag_ || !buffer_.empty()) {
            if (buffer_.read(t)) {
                l2book_.update(t);
            }
        }
        bookUpdaters_.numUpdaers_.fetch_sub(1,std::memory_order_acq_rel);
    }
private:
    BookUpdaters &bookUpdaters_;
    MarketDataBuffer buffer_;
    L2Book &l2book_;
    bool runFlag_ = true;
};



#endif