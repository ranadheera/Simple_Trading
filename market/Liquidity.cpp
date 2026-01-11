#include "Liquidity.h"



Liquidity::Liquidity(std::size_t size, double tickSize): 
    size_(size), mask_(size - 1), tickSize_(tickSize), endIndex_(size > 0 ? size - 1 : 0), volumes_(0)
{
    if (!std::has_single_bit(size)) {
        throw std::invalid_argument("size of Liquidity must be a power of two " + std::to_string(size));
    }
}

bool Liquidity::init()
{ 
    if (!initialized_) {
        volumes_.resize(size_);
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
    mask_ = size -1;
    endIndex_ = size -1;
    return true;
}

bool Liquidity::setTickSize(double tickSize)
{
    if (initialized_)
        return false;

    tickSize_ = tickSize;
    return true;
}

void Liquidity::reset()
{
    std::fill(volumes_.begin(), volumes_.end(), 0);
    refPrice_ = NoPrice;
    bestPrice_ = NoPrice;
    startIndex_ = 0;
    endIndex_ = size_ ? size_ - 1 : 0;
    bestVolume_ = NoVolume;
    topChanges_ = false;
}

int Liquidity::getBestLiquidityIndex() const
{
    if (bestVolume_ != NoVolume)
        return getIndexFromPrice(bestPrice_);

    return NoIndex;
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

        for (int numShift = 0; numShift < numShiftReq; ++numShift) {
            volumes_[startIndex_] = 0;
            startIndex_ = incrementIndex(startIndex_) ;
        }
        endIndex_ = (startIndex_ + size_ - 1) & mask_;
        refPrice_ = refPrice_ + (tickSize_ * numShiftReq);
        volumes_[endIndex_ ] = volume;
        
    } else {
        reset();
        volumes_[startIndex_] = volume;
        refPrice_ = price;
    }

}

bool BidLiquidity::shiftTowardsLowerPrices(int distance,  Price price, Volume volume)
{
    auto numShiftReq = std::min(-distance, static_cast<int>(size_));

    int numShift = 0;
    int i = endIndex_;

    for ( ; numShift < numShiftReq; ++numShift) {
        
        if (volumes_[i] != 0)
            return false;

        i = decrementIndex(i); 
    }

    if (numShift != size_) {
        endIndex_ = i;
        startIndex_ = (endIndex_ +  1) & mask_;
        volumes_[startIndex_] = volume;
        refPrice_ = refPrice_ - tickSize_ * numShift;
    } else {
        startIndex_ = 0;
        endIndex_ = size_ -1;
        volumes_[startIndex_] = volume;
        refPrice_ = price;
        bestPrice_ = price;
        bestVolume_ = volume; 
    }

    return true;
}

bool BidLiquidity::update(Price price, Volume volume)
{
    topChanges_ = false;

    if (refPrice_ == NoPrice) {
        init();
        refPrice_ = price;
    }

    int distance = getDistanceFromReference(price);

    bool volumeAdded =  true;

    if (distance >= 0 && distance < size_) {
        int index =  (distance + startIndex_) & mask_;
        volumes_[index] = volume;
    } else if (distance >= size_) {
        shiftTowardsHigherPrices(distance, price, volume);
    } else {
        volumeAdded = shiftTowardsLowerPrices(distance, price, volume);
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

            i = decrementIndex(i);
        }
    }

    return true;
}

void BidLiquidity::copyTo(BidLiquidity &liquidity) const
{
    liquidity.reset();
    auto size = liquidity.getSize();
    liquidity.tickSize_ = tickSize_;

    auto topIndex   = getBestLiquidityIndex();

    if (topIndex == NoIndex)
        return;

    auto topLiquidity = getBestLiquidity();
    liquidity.bestPrice_ = topLiquidity.first;
    liquidity.bestVolume_ = topLiquidity.second;
    liquidity.refPrice_ = liquidity.bestPrice_ - (liquidity.tickSize_ *  (liquidity.getSize() - 1));

    for (int i = topIndex, j = size -1; ; i = decrementIndex(i), --j) {

        liquidity.volumes_[j] = volumes_[i];

        if (i != getStartIndex() && j > 0)
            continue;
        else
            break;
    }
}

AskLiquidity::AskLiquidity(std::size_t size, double tickSize) : Liquidity(size, tickSize)
{

}

bool AskLiquidity::shiftTowardsHigherPrices(int distance,  Price price, Volume volume)
{
    auto numShiftReq = std::min(distance - static_cast<int>(size_) + 1, static_cast<int>(size_));

    int numShift = 0;
    int i = startIndex_;

    for ( ; numShift < numShiftReq; ++numShift) {
        
        if (volumes_[i] != 0)
            return false;

        i = incrementIndex(i); 
    }

    if (numShift != size_) {
        startIndex_ = i;
        endIndex_ = (startIndex_ +  size_ -1) & mask_;
        volumes_[endIndex_] = volume;
        refPrice_ = refPrice_ + tickSize_ * numShift;
    } else {
        startIndex_ = 0;
        endIndex_ = size_ -1;
        volumes_[startIndex_] = volume;
        refPrice_ = price;
        bestPrice_ = price;
        bestVolume_ = volume; 
    }

    return true;
}

void AskLiquidity::shiftTowardsLowerPrices(int distance,  Price price, Volume volume)
{
    int numShiftReq = -distance;
    int totalShifts = 0;

    if (numShiftReq < size_) {
        for (int numShift = 0; numShift < numShiftReq; ++numShift) {
            volumes_[endIndex_] = 0;
            endIndex_ = decrementIndex(endIndex_); 
        }

        startIndex_ = (endIndex_  +  1) & mask_;
        refPrice_ = refPrice_ - (tickSize_ * numShiftReq);
        volumes_[startIndex_] = volume; 
    } else {
        reset();
        volumes_[startIndex_] = volume;
        refPrice_ = price;
    }
}

bool AskLiquidity::update(Price price, Volume volume)
{
   if (refPrice_ == NoPrice) {
        init();
        refPrice_ = price;
    }

    int distance = getDistanceFromReference(price);
    bool volumeAdded = true;

    if (distance >= 0 && distance < size_) {
        int index =  (distance + startIndex_) & mask_;
        volumes_[index] = volume;
    } else if (distance >= size_) {
        volumeAdded = shiftTowardsHigherPrices(distance, price, volume);
    } else {
        shiftTowardsLowerPrices(distance, price, volume);
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
            i = decrementIndex(i);
        }
    }

    return true;
}

void AskLiquidity::copyTo(AskLiquidity &liquidity) const
{
    liquidity.reset();
    liquidity.tickSize_ = tickSize_;
    auto size = liquidity.getSize();
    
    auto topIndex   = getBestLiquidityIndex();

    if (topIndex == NoIndex)
        return;

    auto topLiquidity = getBestLiquidity();
    liquidity.refPrice_ = topLiquidity.first;
    liquidity.bestPrice_ = liquidity.refPrice_;
    liquidity.bestVolume_ = topLiquidity.second;

    for (int i = topIndex, j = 0; ; i = incrementIndex(i), ++j) {

        liquidity.volumes_[j] = volumes_[i];

        if (i != getEndIndex() && j < liquidity.getSize() - 1)
            continue;
        else
            break;
    }
}

BidLiquidityIterator::BidLiquidityIterator(const BidLiquidity &liquidity): array_(liquidity.volumes_), liquidity_(liquidity)
{
    currentPos_ = liquidity_.getBestLiquidityIndex();
}

PriceVolumePair BidLiquidityIterator::getNext()
{
    if (currentPos_ == NoIndex)
        return PriceVolumePair(NoPrice,NoVolume);

    auto price = liquidity_.getPriceFromIndex(currentPos_);
    auto volume = array_[currentPos_];

    if (currentPos_ == liquidity_.getStartIndex()) {
        currentPos_  = NoIndex;
    } else {
        currentPos_  = liquidity_.decrementIndex(currentPos_);
    }
    return std::make_pair(price, volume);
}

void BidLiquidityIterator::reset()
{
    currentPos_ = liquidity_.getBestLiquidityIndex();
}

AskLiquidityIterator::AskLiquidityIterator(const AskLiquidity &liquidity): array_(liquidity.volumes_), liquidity_(liquidity)
{
    currentPos_ = liquidity_.getBestLiquidityIndex();
}

PriceVolumePair AskLiquidityIterator::getNext()
{
    if (currentPos_  == NoIndex)
        return PriceVolumePair(NoPrice, NoVolume);

    auto price = liquidity_.getPriceFromIndex(currentPos_);
    auto volume = array_[currentPos_];
    
    if (currentPos_ == liquidity_.getEndIndex()) {
        currentPos_ = NoIndex;
    } else {
        currentPos_ = liquidity_.incrementIndex(currentPos_);
    }
    return std::make_pair(price, volume);
}

void AskLiquidityIterator::reset()
{
    currentPos_ = liquidity_.getBestLiquidityIndex();
}
