#include "L2Book.h"


SymbolBook::SymbolBook(int lowerbound, int upperbound, int tikSize): 
lowerbound_(lowerbound), upperbound_(upperbound), tikSize_(tikSize),  size_((upperbound_ -lowerbound + 1)*tikSize_), bidArray_(size_, 0), 
askArray_(size_, 0), maxAskValIndex_(lowerbound - 1), minBidValIndex_(upperbound_ + 1)
{

}

SymbolBook::SymbolBook(const SymbolBook& book) : lowerbound_(book.lowerbound_), upperbound_(book.upperbound_), tikSize_(book.tikSize_),  size_(book.size_), bidArray_(size_, 0), 
askArray_(size_, 0), maxAskValIndex_(book.maxAskValIndex_), minBidValIndex_(book.minBidValIndex_)
{
    int update1;
    int update2;

    do {
        update1 = book.updates.load(std::memory_order_acquire);
        std::copy(book.askArray_.begin(), book.askArray_.end(), askArray_.begin());
        std::copy(book.bidArray_.begin(), book.bidArray_.end(), bidArray_.begin());
        maxAskValIndex_ = book.maxAskValIndex_;
        minBidValIndex_ = book.minBidValIndex_;
        update2 = book.updates.load(std::memory_order_acquire);
        updates.store(update2, std::memory_order_release);
    } while ((update1 != update2) || (update2 %2));

}

bool SymbolBook::update(const Marketdata &data) {

    int askIndex = data.ask_price * tikSize_ - lowerbound_;
    int bidIndex = data.bid_price * tikSize_ - lowerbound_;

    if (askIndex < 0 || askIndex >= size_)
        return false;

    if (bidIndex < 0 || bidIndex >= size_)
        return false;

    auto prevAskVal = askArray_[askIndex];
    auto prevBidVal = bidArray_[askIndex];
    int newMaxAskValIndex = maxAskValIndex_;
    int newMinBidValIndex = minBidValIndex_;

    //askside

    if (data.ask_volume > 0) {
         if (askIndex > maxAskValIndex_) {
            newMaxAskValIndex = askIndex;
        }
    } else if (askIndex == maxAskValIndex_) {
        auto indexT = askIndex -1;

        while (indexT >= 0 && askArray_[indexT] == 0)
            --indexT;

        newMaxAskValIndex = indexT;
    }

    // bidSide

    if (data.bid_volume > 0) {
        if (bidIndex < minBidValIndex_) {
            newMinBidValIndex = bidIndex;
        }
    } else if (bidIndex == minBidValIndex_) {
        auto indexT = bidIndex -1;

        while (indexT < size_ && bidArray_[indexT] == 0)
            ++indexT;
        
        newMinBidValIndex = indexT;
    }


    updates.fetch_add(1, std::memory_order_release);
    bidArray_[bidIndex] = data.bid_volume;
    askArray_[askIndex] = data.ask_volume;
    maxAskValIndex_ = newMaxAskValIndex;
    minBidValIndex_ = newMinBidValIndex;
    updates.fetch_add(1, std::memory_order_release);
    return true;
}

bool SymbolBook::getMinBidPriceVolume(double &price, int &volume) const {

    bool retVal = false;
    int updateCount1;
    int updateCount2;
    do {
        updateCount1 = updates.load(std::memory_order_acquire);
        if (minBidValIndex_ < size_) {
            price = (double) minBidValIndex_ / tikSize_;
            volume = bidArray_[minBidValIndex_];
            retVal = true;
        } else {
            retVal = false;
        }
        updateCount2 = updates.load(std::memory_order_acquire);
    } while ((updateCount2 != updateCount1) || (updateCount2 % 2));

    return retVal;

}

bool SymbolBook::getMaxAskPriceVolume(double &price, int &volume) const {

    bool retVal = false;
    int updateCount1;
    int updateCount2;

    do {
        updateCount1 = updates.load(std::memory_order_acquire);
        if (maxAskValIndex_ >= 0) {           
            price = (double) maxAskValIndex_ / tikSize_;
            volume = askArray_[maxAskValIndex_];
            retVal = true;
        } else {
            retVal = false;
        }
        updateCount2 = updates.load(std::memory_order_acquire);
    } while ((updateCount2 != updateCount1) || (updateCount2 % 2));

    return retVal;
}

L2Book::L2Book(std::size_t size):symbolBooks_(size, SymbolBook(0,1000,100)) {}

bool L2Book::update(std::size_t index, const Marketdata &data) {
    return symbolBooks_[index].update(data);    
}

bool L2Book::getMinBidPriceVolume(std::size_t index, double &price, int &volume) const
{
    return symbolBooks_[index].getMinBidPriceVolume(price, volume);
}

bool L2Book::getMaxAskPriceVolume(std::size_t index, double &price, int &volume) const
{
    return symbolBooks_[index].getMaxAskPriceVolume(price, volume);
}


SymbolBook L2Book::getSnapShot(std::size_t index) const
{
    return symbolBooks_[index];
}