#ifndef FAST_RING_BUFFER_H
#define FAST_RING_BUFFER_H

#include <vector>
#include "CommonUtils.h"


template<typename T> class FastRingBuffer 
{
friend class Iterator;
friend class  ReverseIterator;
public:
    class Iterator
    {
    public:
        Iterator(FastRingBuffer& rinbuffer, std::size_t currentIndex);
        T& operator*();
        void operator+=(std::size_t distance);
        void operator-=(std::size_t distance);
        bool operator!=(const Iterator& cmp) const;
        bool operator==(const Iterator& cmp) const;
    private:
        FastRingBuffer* ringbuffer_;
        std::size_t currentIndex_;
    };

    class ReverseIterator
    {
    public:
        ReverseIterator(FastRingBuffer& rinbuffer, std::size_t currentIndex);
        T& operator*();
        void operator+=(std::size_t distance);
        void operator-=(std::size_t distance);
        bool operator!=(const ReverseIterator& cmp) const;
        bool operator==(const ReverseIterator& cmp) const;
    private:
        FastRingBuffer* ringbuffer_;
        std::size_t currentIndex_;
    };

public:
    FastRingBuffer(std::size_t size);
    bool empty() const { return start_ == end_; }
    std::size_t count () const { return end_ - start_; }
    void push(const T& data);
    T& front();
    const T& front() const;
    T popFront();
    T& back();
    const T& back() const;
    Iterator begin();
    Iterator end();
    ReverseIterator rbegin();
    ReverseIterator rend();

private:
    std::size_t logicalSize_;
    std::size_t physicalSize_;
    std::size_t mask_;
    std::size_t start_ = 0;
    std::size_t end_ = 0;
    std::vector<T> array_;
};

template<typename T> FastRingBuffer<T>::FastRingBuffer(std::size_t size) : logicalSize_(size), physicalSize_(getNextPowerOfTwo(size)),
    mask_(physicalSize_ - 1), array_(physicalSize_) 
{
}

template<typename T> void FastRingBuffer<T>::push(const T& data) 
{
    array_[end_ & mask_] = data;
    ++end_;

    if (end_ - start_ > logicalSize_)
       ++start_; 
}

template<typename T> T& FastRingBuffer<T>::front()
{
    return array_[start_ & mask_];
}

template<typename T> const T& FastRingBuffer<T>::front() const
{
    return array_[start_ & mask_];
}

template<typename T> T FastRingBuffer<T>::popFront()
{
    T ret =  array_[start_ & mask_];
    ++start_;
    return ret;
}

template<typename T> T& FastRingBuffer<T>::back()
{
    return array_[(end_ - 1) & mask_];
}

template<typename T> const T& FastRingBuffer<T>::back() const
{
    return array_[(end_ - 1) & mask_];
}


template<typename T> FastRingBuffer<T>::Iterator FastRingBuffer<T>::begin()
{
    return Iterator(*this, start_);
}

template<typename T> FastRingBuffer<T>::Iterator FastRingBuffer<T>::end()
{
    return Iterator(*this, end_);
}

template<typename T> FastRingBuffer<T>::ReverseIterator FastRingBuffer<T>::rbegin()
{
    return ReverseIterator(*this, end_ - 1);
}

template<typename T> FastRingBuffer<T>::ReverseIterator FastRingBuffer<T>::rend()
{
    return ReverseIterator(*this, start_ - 1);
}

template<typename T> FastRingBuffer<T>::Iterator::Iterator(FastRingBuffer& rinbuffer, std::size_t currentIndex) : ringbuffer_(&rinbuffer), currentIndex_(currentIndex)
{

}

template<typename T> T& FastRingBuffer<T>::Iterator::operator*()
{
    return ringbuffer_->array_[currentIndex_ & ringbuffer_->mask_];
}

template<typename T> void FastRingBuffer<T>::Iterator::operator+=(std::size_t distance)
{
    currentIndex_ += distance;
}

template<typename T> void FastRingBuffer<T>::Iterator::operator-=(std::size_t distance)
{
    currentIndex_ -= distance;
}

template<typename T> bool FastRingBuffer<T>::Iterator::operator==(const Iterator& cmp) const
{
    return ringbuffer_ ==  cmp.ringbuffer_ && currentIndex_ == cmp.currentIndex_;
}

template<typename T> bool FastRingBuffer<T>::Iterator::operator!=(const Iterator& cmp) const
{
    return !(*this == cmp);
}

template<typename T> FastRingBuffer<T>::ReverseIterator::ReverseIterator(FastRingBuffer& rinbuffer, std::size_t currentIndex) : ringbuffer_(&rinbuffer), currentIndex_(currentIndex)
{

}

template<typename T> T& FastRingBuffer<T>::ReverseIterator::operator*()
{
    return ringbuffer_->array_[currentIndex_ & ringbuffer_->mask_];
}

template<typename T> void FastRingBuffer<T>::ReverseIterator::operator+=(std::size_t distance)
{
    currentIndex_ -= distance;
}

template<typename T> void FastRingBuffer<T>::ReverseIterator::operator-=(std::size_t distance)
{
    currentIndex_ += distance;
}

template<typename T> bool FastRingBuffer<T>::ReverseIterator::operator==(const ReverseIterator& cmp) const
{
    return ringbuffer_ ==  cmp.ringbuffer_ && currentIndex_ == cmp.currentIndex_;
}

template<typename T> bool FastRingBuffer<T>::ReverseIterator::operator!=(const ReverseIterator& cmp) const
{
    return !(*this == cmp);
}

#endif