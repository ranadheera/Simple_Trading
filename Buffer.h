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
    buffer_[index].updateCount_.fetch_add(1, std::memory_order_release);
    buffer_[index].data_ = data;
    buffer_[index].updateCount_.fetch_add(1, std::memory_order_release);
}

template<typename T> bool SWMRBuffer<T>::getData(int index, T& data) const
{
    std::size_t updateCount1;
    std::size_t updateCount2;
    do {
        updateCount1 = buffer_[index].updateCount_.load(std::memory_order_acquire);

        if (updateCount1 == 0)
            return false;

        data = buffer_[index].data_;

        updateCount2 = buffer_[index].updateCount_.load(std::memory_order_acquire);
    } while ((updateCount1 != updateCount2) && (updateCount2 %2));
    return true;
}


#endif