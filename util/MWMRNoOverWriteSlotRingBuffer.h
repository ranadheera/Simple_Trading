#ifndef MWMR_NO_OVERWRITE_RING_BUFFER_H
#define MWMR_NO_OVERWRITE_RING_BUFFER_H

#include <memory>
#include <atomic>

template<typename T> class MWMRNoOverWriteSlotRingBuffer
{
private:
    using Status = uint32_t;

public:
    struct Slot {
    friend class MWMRNoOverWriteSlotRingBuffer<T>;
    public:
        Slot() { status_.store(0, std::memory_order_release);}
        T& getData();;
    private:
        T data_;
        std::atomic<Status> status_;
        
    };

public:
    MWMRNoOverWriteSlotRingBuffer(std::size_t size) : size_(size) {
        array_ = new Slot[size];
        writeIndex_.store(0);
        readIndex_.store(0);
    }
    ~MWMRNoOverWriteSlotRingBuffer() {
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
    inline std::size_t incrementIndex(std::size_t index) { return ++index % size_;}
private:
   std::size_t size_;
   std::atomic<int> writeIndex_;
   std::atomic<int> readIndex_ ;
   Slot* array_;
};
 
template<typename T> T& MWMRNoOverWriteSlotRingBuffer<T>::Slot::getData()
{
    return data_;
}

template<typename T> MWMRNoOverWriteSlotRingBuffer<T>::Slot* MWMRNoOverWriteSlotRingBuffer<T>::getWriteSlot()
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

template<typename T> void MWMRNoOverWriteSlotRingBuffer<T>::setWriteComplete(Slot *t)
{
    auto st = t->status_.load(std::memory_order_acquire);
    clearWriterIn(st);
    setDataValid(st);
    t->status_.store(st, std::memory_order_release);
}

template<typename T> MWMRNoOverWriteSlotRingBuffer<T>::Slot* MWMRNoOverWriteSlotRingBuffer<T>::getReadSlot()
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

template<typename T> void MWMRNoOverWriteSlotRingBuffer<T>::setReadComplete(Slot *t)
{
    auto st = t->status_.load(std::memory_order_acquire);
    clearReaderIn(st);
    clearDataValid(st);
    t->status_.store(st, std::memory_order_release);

}

#endif