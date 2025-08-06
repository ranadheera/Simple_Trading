#ifndef UPDATERS_H
#define UPDATERS_H

#include "Buffer.h"
#include "MarketData.h"
#include "L1Book.h"
#include "L2Book.h"

using L1Buffer = SWMRBuffer<Marketdata>;
using MarketDataBuffer = RingBuffer<Marketdata>;

class Updater
{

public:
    Updater(L1Buffer &l1buffer, L1Book &l1book, L2Book &l2book, std::size_t size) : buffer_(size), l1buffer_(l1buffer), l1book_(l1book), l2book_(l2book) {}
public:
    void addData(const Marketdata& data) { buffer_.write(data); }
    void exec() {
        Marketdata t;
        while (true) {
            if (buffer_.read(t)) {
                l1buffer_.update(getIndex(t), t);
                l1book_.update(getIndex(t), t);
                l2book_.update(getIndex(t), t);
            }
        }
    }
private:
    MarketDataBuffer buffer_;
    L1Buffer &l1buffer_;
    L1Book &l1book_;
    L2Book &l2book_;
};

class UpdatersList
{
public:
    UpdatersList(L1Buffer &l1buffer, L1Book &l1book, L2Book &l2book, std::size_t numupdaters, std::size_t bufferSize);
    void start();
    inline Updater* getUpdater(std::size_t index) { return updaters_[index]; }
    auto getNumUpdaters() { return updaters_.size(); }
private:
    L1Buffer &l1buffer_;
    L1Book &l1book_;
    L2Book &l2book_;
    std::vector<Updater*> updaters_;
    std::size_t bufferSize_;
};

#endif