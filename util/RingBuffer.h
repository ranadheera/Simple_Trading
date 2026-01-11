
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <memory>
#include <utility>
#include <stdexcept>
#include <atomic>
#include <cstdint>

template<typename T> class NonOverridableRingBuffer {
public:
    NonOverridableRingBuffer(std::size_t t);
    bool write(const T& t);
    bool read(T& t);
    bool empty() { return (count_ == 0); }
private:
    std::size_t size_ = 0;
    std::unique_ptr<T[]> array_;
    std::size_t readIndex_ = 0;
    std::size_t writeIndex_ = 0;
    std::size_t count_ = 0;
};

template<typename T> NonOverridableRingBuffer<T>::NonOverridableRingBuffer(std::size_t size) : size_(size), array_(new T[size])
{
    
}

template<typename T> bool NonOverridableRingBuffer<T>::write(const T& t)
{
    if (count_ == size_)
        return false;

    array_[writeIndex_] = t;
    writeIndex_ = ++writeIndex_ % size_;
    ++count_;
}

template<typename T> bool NonOverridableRingBuffer<T>::read(T& t)
{
    if (count_ == 0)
        return false;

    t = array_[readIndex_];
    readIndex_ = ++readIndex_ % size_;
    --count_;
}

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

template<typename T> class SWSRNonOverridableRingBuffer {
public:
    SWSRNonOverridableRingBuffer(std::size_t size);
    ~SWSRNonOverridableRingBuffer();
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

template<typename T> class MWMRNonOverridableSlotRingBuffer {
private:
    using Status = uint32_t;

public:
    struct Slot {
    friend class MWMRNonOverridableSlotRingBuffer<T>;
    public:
        Slot() { status_.store(0, std::memory_order_release);}
        T& getData();;
    private:
        T data_;
        std::atomic<Status> status_;
        
    };

public:
    MWMRNonOverridableSlotRingBuffer(std::size_t size) : size_(size) {
        array_ = new Slot[size];
        writeIndex_.store(0);
        readIndex_.store(0);
    }
    ~MWMRNonOverridableSlotRingBuffer() {
        delete[] array_;
    }
public:
    Slot* getWriteSlot();
    Slot* getReadSlot();
    void setWriteComplete(Slot *t);
    void setReadComplete(Slot *t);

private:
    enum SlotStatus {WRITER_IN = 1, READER_IN = 2, DATA_VALID = 4};
    void setWriterIn(Status &value) { value =  value | WRITER_IN ; }
    void clearWriterIn(Status &value) { value = value & ~WRITER_IN; }
    void setReaderIn(Status &value) { value = value | READER_IN; }
    void clearReaderIn(Status &value) { value = value & ~READER_IN;}
    void setDataValid(Status &value) { value = value | DATA_VALID; }
    void clearDataValid(Status &value) { value = value & ~DATA_VALID; }

    bool isWriterIn(Status value) { return value & WRITER_IN; }
    bool isReaderIn(Status value) { return value & READER_IN; }
    bool isDataValid(Status value) { return value & DATA_VALID; }

private:
    inline std::size_t incrementIndex(std::size_t index) {
    return ++index % size_;
}
private:
   std::size_t size_;
   std::atomic<int> writeIndex_;
   std::atomic<int> readIndex_ ;
   Slot* array_;
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
 
template<typename T> SWSRNonOverridableRingBuffer<T>::SWSRNonOverridableRingBuffer(std::size_t size) : size_(size)
{
    if (size_ <= 0) {
        throw std::invalid_argument("Size must be greater than zero.");
    }
    array_ = new T[size_];
    writePtr_ =  array_;
    readPtr_ = array_;
    count_.store(0, std::memory_order_release);
}

template<typename T> SWSRNonOverridableRingBuffer<T>::~SWSRNonOverridableRingBuffer()
{
    delete[] array_;
}

template<typename T> bool SWSRNonOverridableRingBuffer<T>::write(const T &data)
{
    if (count_.load(std::memory_order_acquire) == size_)
            return false;

        *writePtr_ = data;
        incrementPointer(writePtr_);
        count_.fetch_add(1, std::memory_order_acq_rel);
}

template<typename T> bool SWSRNonOverridableRingBuffer<T>::read(T &data) {

    if (count_.load(std::memory_order_acquire) == 0)
        return false;

    auto readPtrTmp = readPtr_;

    data = std::move(*readPtr_);
    incrementPointer(readPtr_);
    count_.fetch_sub(1, std::memory_order_acq_rel);
    return true;
}

template<typename T> bool SWSRNonOverridableRingBuffer<T>::empty() {
    return (count_.load(std::memory_order_acquire) == 0);
}

template<typename T> T& MWMRNonOverridableSlotRingBuffer<T>::Slot::getData()
{
    return data_;
}

template<typename T> MWMRNonOverridableSlotRingBuffer<T>::Slot* MWMRNonOverridableSlotRingBuffer<T>::getWriteSlot()
{
    while(true) {
 
        auto wi = writeIndex_.load(std::memory_order_acquire);

        auto &slot = array_[wi];
        auto st = slot.status_.load();

        if (isWriterIn(st))
            continue;
        
        if (isDataValid(st))
            return nullptr;

        auto stcopy = st;
        setWriterIn(st);

        if (!slot.status_.compare_exchange_strong(stcopy, st, std::memory_order_acq_rel))
            continue;

        auto win = incrementIndex(wi);
        writeIndex_.store(win, std::memory_order_release);
        return &slot;

    }
}

template<typename T> void MWMRNonOverridableSlotRingBuffer<T>::setWriteComplete(Slot *t)
{
    auto st = t->status_.load(std::memory_order_acquire);
    clearWriterIn(st);
    setDataValid(st);
    t->status_.store(st, std::memory_order_release);
}

template<typename T> MWMRNonOverridableSlotRingBuffer<T>::Slot* MWMRNonOverridableSlotRingBuffer<T>::getReadSlot()
{
    while(true) {

        auto ri = readIndex_.load(std::memory_order_acquire);

        auto &slot = array_[ri];
        auto st = slot.status_.load();

        if (isReaderIn(st))
            continue;

        if (!isDataValid(st))
            return nullptr;

        auto stcopy = st;
        setReaderIn(st);

        if (!slot.status_.compare_exchange_strong(stcopy, st, std::memory_order_acq_rel))
            continue;

        auto rin = incrementIndex(ri);
        readIndex_.store(rin, std::memory_order_release);
        return &slot;
 
    }
}

template<typename T> void MWMRNonOverridableSlotRingBuffer<T>::setReadComplete(Slot *t)
{
    auto st = t->status_.load(std::memory_order_acquire);
    clearReaderIn(st);
    clearDataValid(st);
    t->status_.store(st, std::memory_order_release);

}
#endif