
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <utility>
#include <stdexcept>
#include <atomic>

template<typename T> class RingBuffer {
public:
    RingBuffer(std::size_t size);
    ~RingBuffer();
    void write(T data);
    bool read(T &data);
    bool empty();

private:
    inline void incrementPointer(T*& ptr) {
        ++ptr;
        if (ptr == array_ + size_) {
            ptr = array_;
        }
    }

private:
    std::size_t size_;
    T* array_;
    std::atomic<T*> writePtr_;
    std::atomic<T*> readPtr_;
};

template<typename T> class NonOverridableRingBuffer {
public:
    NonOverridableRingBuffer(std::size_t size);
    ~NonOverridableRingBuffer();
    bool write(const T &data);
    bool read(T &data);
    bool empty();

private:
    inline void incrementPointer(T*& ptr) {
        ptr++;
        if (ptr == array_ + size_) {
            ptr = array_;
        }
    }

private:
    std::size_t size_;
    T* array_;
    T* writePtr_;
    T* readPtr_;
    std::atomic<std::size_t> count_;
};

template<typename T>  RingBuffer<T>::RingBuffer(std::size_t size) : size_(size)
{
    if (size_ <= 0) {
        throw std::invalid_argument("Size must be greater than zero.");
    }
    array_ = new T[size_];
    writePtr_.store(array_, std::memory_order_release);
    readPtr_.store(array_, std::memory_order_release);
}

template<typename T>  RingBuffer<T>:: ~RingBuffer() 
{
    delete[] array_;
}

template<typename T> void  RingBuffer<T>::write(T data) {
    T* tmpWrite = writePtr_.load(std::memory_order_acquire);
    *tmpWrite = std::move(data);
    incrementPointer(tmpWrite);
    T* tmpWrite2 = tmpWrite;
    incrementPointer(tmpWrite2);
    T *readPtrTmp = tmpWrite;
    readPtr_.compare_exchange_strong(readPtrTmp, tmpWrite2, std::memory_order_acq_rel, std::memory_order_acquire);
    writePtr_.store(tmpWrite, std::memory_order_release);
}


template<typename T> bool RingBuffer<T>::read(T &data) {
    auto readPtrTmp = readPtr_.load(std::memory_order_acquire);

    if (writePtr_.load(std::memory_order_acquire) == readPtrTmp)
            return false;

    data = std::move(*readPtrTmp);
    incrementPointer(readPtrTmp);
    readPtr_.store(readPtrTmp, std::memory_order_release);
    return true;

}

template<typename T> bool RingBuffer<T>::empty() {
    return (writePtr_.load(std::memory_order_acquire) == readPtr_.load(std::memory_order_acquire));
}
 
template<typename T> NonOverridableRingBuffer<T>::NonOverridableRingBuffer(std::size_t size) : size_(size)
{
    if (size_ <= 0) {
        throw std::invalid_argument("Size must be greater than zero.");
    }
    array_ = new T[size_];
    writePtr_ =  array_;
    readPtr_ = array_;
    count_.store(0, std::memory_order_release);
}

template<typename T> NonOverridableRingBuffer<T>::~NonOverridableRingBuffer()
{
    delete[] array_;
}

template<typename T> bool NonOverridableRingBuffer<T>::write(const T &data)
{
    if (count_.load(std::memory_order_acquire) == size_)
            return false;

        *writePtr_ = data;
        incrementPointer(writePtr_);
        count_.fetch_add(1, std::memory_order_release);
}

template<typename T> bool NonOverridableRingBuffer<T>::read(T &data) {

    if (count_.load(std::memory_order_acquire) == 0)
        return false;

    auto readPtrTmp = readPtr_;

    data = std::move(*readPtr_);
    incrementPointer(readPtr_);
    count_.fetch_sub(1, std::memory_order_release);
    return true;
}

template<typename T> bool NonOverridableRingBuffer<T>::empty() {
    return (count_.load(std::memory_order_acquire) == 0);
}

#endif