#include "Liquidity.h"

Liquidity::Liquidity(std::size_t size, double tickSize): 
    size_(size), mask_(size - 1), tickSize_(tickSize), endIndex_(size > 0 ? size - 1 : 0), volumes_(0)
{
    if (!std::has_single_bit(size)) {
        throw std::invalid_argument("size of Liquidity must be a power of two ");
    }
}

bool Liquidity::init()
{ 
    if (!initialized_) {
        volumes_.resize(size_);
        mask_ = size_ -1;
        endIndex_ = size_ -1;
        midOffset_ = size_ / 2;
        initialized_ = true;
        return true;
    }
    return false;
}

bool Liquidity::setSize(std::size_t size)
{
    if (initialized_)
        return false;

    if (!std::has_single_bit(size)) {
        throw std::invalid_argument("size of Liquidity must be a power of two " + std::to_string(size));
    }

    size_ = size;
    return true;
}

bool Liquidity::setTickSize(double tickSize)
{
    if (initialized_)
        return false;

    tickSize_ = tickSize;
    return true;
}

Liquidity::Iterator Liquidity::begin()
{
    return Iterator(*this, 0);
}

Liquidity::Iterator Liquidity::end()
{
    Iterator ei = initialized_ ? Iterator(*this, size_) : Iterator(*this, 0);
    return ei;
}

Liquidity::ReverseIterator Liquidity::rbegin()
{
    ReverseIterator rb = initialized_ ? ReverseIterator(*this, size_ -1) : ReverseIterator(*this, -1);
    return rb;
}

Liquidity::ReverseIterator Liquidity::rend()
{
    return ReverseIterator(*this, -1);
}

PriceVolumePair Liquidity::getBestLiquidity() const
{
    return std::pair(bestPrice_,bestVolume_);
}

BidLiquidity::BidLiquidity(std::size_t size, double tickSize): Liquidity(size, tickSize)
{

}

void BidLiquidity::shiftTowardsHigherPrices(int distance,  Price price, Volume volume)
{
    int numShiftReq = distance - size_ + 1;
    int totalShifts = 0;

    if (numShiftReq < size_) {

        for (int numShift = 1; numShift <= numShiftReq; ++numShift) {
            volumes_[(endIndex_ + numShift) & mask_] = 0;
        }
        startIndex_ = (startIndex_ + numShiftReq) & mask_;
        endIndex_ = (endIndex_ + numShiftReq) & mask_;
        startIndexPrice_ = startIndexPrice_ + (tickSize_ * numShiftReq);
        volumes_[endIndex_ ] = volume;
        
    } else {
        std::fill(volumes_.begin(), volumes_.end(), 0);
        bestPrice_ = NoPrice;
        bestVolume_ = NoVolume;
        volumes_[(startIndex_ + midOffset_) & mask_] = volume;
        startIndexPrice_ = price - midOffset_ * tickSize_;
    }

}

bool BidLiquidity::shiftTowardsLowerPrices(int distance,  Price price, Volume volume)
{
    auto numShiftReq = std::min(-distance, static_cast<int>(size_));

    for (int numShift = 0; numShift < numShiftReq; ++numShift) {
        
        if (volumes_[(endIndex_ - numShift) & mask_] != 0)
            return false;
    }

    if (numShiftReq < size_) {
        endIndex_ = (endIndex_ - numShiftReq) & mask_;
        startIndex_ = (startIndex_ - numShiftReq) & mask_;
        volumes_[startIndex_] = volume;
        startIndexPrice_ = startIndexPrice_ - tickSize_ * numShiftReq;
    } else {
        volumes_[(startIndex_ + midOffset_) & mask_] = volume;
        startIndexPrice_ = price - midOffset_ * tickSize_;
    }

    return true;
}

bool BidLiquidity::update(Price price, Volume volume)
{
    topChanges_ = false;

    if (startIndexPrice_ == NoPrice) {
        init();
        startIndexPrice_ = price - midOffset_ * tickSize_;
    }

    int distance = getOffsetFromStartIndex(price);

    bool volumeAdded =  true;

    if (distance >= 0 && distance < size_) {
        int index =  (distance + startIndex_) & mask_;
        volumes_[index] = volume;
    } else if (distance < 0) {
        volumeAdded = shiftTowardsLowerPrices(distance, price, volume);
    } else {
        shiftTowardsHigherPrices(distance, price, volume);
    }

    if (!volumeAdded)
        return false;

    if (volume > 0) {

        if (bestPrice_ == NoPrice || bestPrice_ <= price) {
            bestPrice_ = price;
            bestVolume_ = volume;
            topChanges_ = true;
        } 

    } else if (price == bestPrice_) {
        bestVolume_ = 0;
        bestPrice_ = NoPrice;
        topChanges_ = true;
        int topValIndex = getIndexFromPrice(price);

        for (int i = topValIndex; i != endIndex_;) {

            if (volumes_[i] != 0) {
                bestPrice_ = getPriceFromIndex(i);
                bestVolume_ = volumes_[i];
                break;
            }

            i = (i - 1) & mask_;
        }
    }

    return true;
}

void BidLiquidity::copyTo(BidLiquidity &liquidity) const
{
    std::fill(liquidity.volumes_.begin(), liquidity.volumes_.end(), 0);
    liquidity.startIndexPrice_ = NoPrice;
    liquidity.startIndex_ = 0;
    liquidity.endIndex_ = liquidity.size_ ? liquidity.size_ - 1 : 0;
    liquidity.topChanges_ = false;
    liquidity.tickSize_ = tickSize_;

    liquidity.bestPrice_ = bestPrice_;
    liquidity.bestVolume_ = bestVolume_;

    if (bestPrice_ == NoPrice)
        return;

    auto size = liquidity.size_;
    liquidity.startIndexPrice_ = liquidity.bestPrice_ - (liquidity.tickSize_ *  (size - 1));
    auto topIndex   = getIndexFromPrice(bestPrice_);

    for (int i = topIndex, j = size -1;  ; i = (i-1) & mask_, --j) {
        liquidity.volumes_[j] = volumes_[i];

        if ((i == startIndex_) || (j == 0))
            break;
    }
    liquidity.topChanges_ = true;
}

AskLiquidity::AskLiquidity(std::size_t size, double tickSize) : Liquidity(size, tickSize)
{

}

bool AskLiquidity::shiftTowardsHigherPrices(int distance,  Price price, Volume volume)
{
    auto numShiftReq = std::min(distance - static_cast<int>(size_) + 1, static_cast<int>(size_));

    int numShift = 0;

    for (int i = startIndex_; numShift < numShiftReq; ++numShift) {
        
        if (volumes_[i] != 0)
            return false;

        i = (i + 1) & mask_;
    }

    if (numShift < size_) {
        startIndex_ = (startIndex_ + numShiftReq) & mask_;
        endIndex_ = (endIndex_ + numShiftReq) & mask_;
        volumes_[endIndex_] = volume;
        startIndexPrice_ = startIndexPrice_ + tickSize_ * numShift;
    } else {
        volumes_[(startIndex_ + midOffset_) & mask_] = volume;
        startIndexPrice_ = price - midOffset_ * tickSize_;
    }

    return true;
}

void AskLiquidity::shiftTowardsLowerPrices(int distance,  Price price, Volume volume)
{
    int numShiftReq = -distance;
    int totalShifts = 0;

    if (numShiftReq < size_) {
        for (int numShift = 1; numShift <= numShiftReq; ++numShift) {
            volumes_[(startIndex_ - numShift) & mask_] = 0;
        }

        startIndex_ = (startIndex_ - numShiftReq) & mask_;
        endIndex_ = (endIndex_ - numShiftReq) & mask_;
        startIndexPrice_ = startIndexPrice_ - (tickSize_ * numShiftReq);
        volumes_[startIndex_] = volume; 
    } else {
        std::fill(volumes_.begin(), volumes_.end(), 0);
        bestPrice_ = NoPrice;
        bestVolume_ = NoVolume;
        volumes_[(startIndex_ + midOffset_) & mask_] = volume;
        startIndexPrice_ = price - midOffset_ * tickSize_;
    }
}

bool AskLiquidity::update(Price price, Volume volume)
{
    topChanges_ = false;

    if (startIndexPrice_ == NoPrice) {
        init();
        startIndexPrice_ = price - midOffset_ * tickSize_;
    }

    int distance = getOffsetFromStartIndex(price);
    bool volumeAdded = true;

    if (distance >= 0 && distance < size_) {
        int index =  (distance + startIndex_) & mask_;
        volumes_[index] = volume;
    } else if (distance < 0) {
        shiftTowardsLowerPrices(distance, price, volume);
    } else {
        volumeAdded = shiftTowardsHigherPrices(distance, price, volume);
    }

    if (!volumeAdded)
        return false;

    if (volume > 0) {

        if (bestPrice_ == NoPrice || bestPrice_ >= price) {
            bestPrice_ = price;
            bestVolume_ = volume;
            topChanges_ = true;
        }

    } else if (price == bestPrice_) {
        bestVolume_ = NoVolume;
        bestPrice_ = NoPrice;
        topChanges_ = true;
        int topValIndex = getIndexFromPrice(price);

        for (int i = topValIndex; i != startIndex_ ;) {

            if (volumes_[i] != 0) {
                bestPrice_ = getPriceFromIndex(i);
                bestVolume_ = volumes_[i];
                break;
            }
            i = (i + 1) & mask_;
        }
    }

    return true;
}

void AskLiquidity::copyTo(AskLiquidity &liquidity) const
{
    std::fill(liquidity.volumes_.begin(), liquidity.volumes_.end(), 0);
    liquidity.startIndexPrice_ = NoPrice;
    liquidity.startIndex_ = 0;
    liquidity.endIndex_ = liquidity.size_ ? liquidity.size_ - 1 : 0;
    liquidity.topChanges_ = false;
    liquidity.tickSize_ = tickSize_;

    liquidity.bestPrice_ = bestPrice_;
    liquidity.bestVolume_ = bestVolume_;

    if (bestPrice_ == NoPrice)
        return;

    auto topIndex   = getIndexFromPrice(bestPrice_);
    liquidity.startIndexPrice_ = liquidity.bestPrice_;


    for (int i = topIndex, j = 0; ; i = (i + 1) & mask_, ++j) {

        liquidity.volumes_[j] = volumes_[i];

        if (i == endIndex_ || j == liquidity.getSize() - 1)
            break;
    }

    liquidity.topChanges_ = true;
}

Liquidity::Iterator::Iterator(Liquidity& liquidity, int currentIndex) : liquidity_(liquidity), currentIndex_(currentIndex)
{

}

void Liquidity::Iterator::operator+=(int distance)
{
    currentIndex_ += distance;
}

void Liquidity::Iterator::operator-=(int distance)
{
    currentIndex_ -= distance;
}

Volume& Liquidity::Iterator::operator*()
{
    return liquidity_[currentIndex_];
}

bool Liquidity::Iterator::operator==(const Iterator& cmp) const
{
    return ((&liquidity_ == &cmp.liquidity_) && (currentIndex_ == cmp.currentIndex_));
}

bool Liquidity::Iterator::operator!=(const Iterator& cmp) const
{
    return !(*this == cmp);
}

Liquidity::ReverseIterator::ReverseIterator(Liquidity& liquidity, int currentIndex) : liquidity_(liquidity), currentIndex_()
{

}

void Liquidity::ReverseIterator::operator+=(int distance)
{
    currentIndex_ -= distance;
}

void Liquidity::ReverseIterator::operator-=(int distance)
{
    currentIndex_ += distance;
}

Volume& Liquidity::ReverseIterator::operator*()
{
    return liquidity_[currentIndex_];
}

bool Liquidity::ReverseIterator::operator==(const ReverseIterator& cmp) const
{
    return ((&liquidity_ == &cmp.liquidity_) && (currentIndex_ == cmp.currentIndex_));
}

bool Liquidity::ReverseIterator::operator!=(const ReverseIterator& cmp) const
{
    return !(*this == cmp);
}