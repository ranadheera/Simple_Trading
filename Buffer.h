#ifndef BUFFER_H
#define BUFFER_H

#include "RingBuffer.h"
#include "MarketData.h"
#include <atomic>
#include <vector>
#include <thread>

template<typename T> class SWMRBuffer
{
private:
    struct BufferItem {
        std::atomic<std::size_t> updateCount_;
        T data_;
    };
public:
    SWMRBuffer(std::size_t size) : buffer_(size) {}
public:
    void update(int index, const T& data);
    bool getData(int index, T& data) const;
private:
    std::vector<BufferItem> buffer_;
};

template<typename T> void SWMRBuffer<T>::update(int index, const T& data)
{
    buffer_[index].data_ = data;
    buffer_[index].updateCount_.fetch_add(1);
}

template<typename T> bool SWMRBuffer<T>::getData(int index, T& data) const
{
    auto updateCount = buffer_[index].updateCount_.load();

    if (updateCount == 0)
        return false;

    data = buffer_[index].data_;


    for (auto tmpCount = buffer_[index].updateCount_.load(); tmpCount != updateCount ; updateCount = tmpCount) {
        data = buffer_[index].data_;
    }

    return true;
}


#endif