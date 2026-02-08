#ifndef SWSR_RING_BUFFER_H
#define SWSR_RING_BUFFER_H

#include <memory>
#include <atomic>

template<typename T> class SWSRRingBuffer {
public:
    SWSRRingBuffer(std::size_t size);
    ~SWSRRingBuffer();
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

template<typename T>  SWSRRingBuffer<T>::SWSRRingBuffer(std::size_t size) : size_(size)
{
    if (size_ <= 0) {
        throw std::invalid_argument("Size must be greater than zero.");
    }
    array_ = new T[size_];
    writePtr_.store(array_, std::memory_order_release);
    readPtr_.store(array_, std::memory_order_release);
}

template<typename T>  SWSRRingBuffer<T>:: ~SWSRRingBuffer() 
{
    delete[] array_;
}

template<typename T> void  SWSRRingBuffer<T>::write(T data) {
    T* tmpWrite = writePtr_.load(std::memory_order_acquire);
    *tmpWrite = std::move(data);
    incrementPointer(tmpWrite);
    T* tmpWrite2 = tmpWrite;
    incrementPointer(tmpWrite2);
    T *readPtrTmp = tmpWrite;
    readPtr_.compare_exchange_strong(readPtrTmp, tmpWrite2, std::memory_order_acq_rel, std::memory_order_acquire);
    writePtr_.store(tmpWrite, std::memory_order_release);
}


template<typename T> bool SWSRRingBuffer<T>::read(T &data) {
    auto readPtrTmp = readPtr_.load(std::memory_order_acquire);

    if (writePtr_.load(std::memory_order_acquire) == readPtrTmp)
            return false;

    data = std::move(*readPtrTmp);
    incrementPointer(readPtrTmp);
    readPtr_.store(readPtrTmp, std::memory_order_release);
    return true;

}

template<typename T> bool SWSRRingBuffer<T>::empty() {
    return (writePtr_.load(std::memory_order_acquire) == readPtr_.load(std::memory_order_acquire));
}

#endif