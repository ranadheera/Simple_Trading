#ifndef LIQUIDITY_H
#define LIQUIDITY_H

#include <cstdint>
#include "MarketData.h"
#include <vector>

using PriceVolumePair = std::pair<Price,Volume>;
constexpr Price NoPrice = -1;
constexpr Volume NoVolume = -1;
constexpr int NoIndex = -1;

class Liquidity
{
protected:
    Liquidity(std::size_t size, double tickSize);
public:
    PriceVolumePair getBestLiquidity() const;
public:
    bool init();
    bool getInitState() const { return initialized_; }
    bool setSize(std::size_t size);
    std::size_t getSize() const { return size_; }
    bool setTickSize(double tickSize);
    double getTickSize() const { return tickSize_; }
    bool isBestChangeWithLastUpdate() const { return topChanges_; }

protected:
    void reset();
    int getBestLiquidityIndex() const;
    inline Price getPriceFromIndex(int index) const;
    inline int getIndexFromPrice(Price price) const;
    inline int getDistanceFromReference(Price price) const;
    int getStartIndex() const { return startIndex_; }
    int getEndIndex() const { return endIndex_; }
    int incrementIndex(int i) const { return (i + 1) & mask_; }
    int decrementIndex(int i) const { return (i - 1) & mask_; }
protected:
    std::size_t size_;
    int mask_;
    double tickSize_;
    Price refPrice_ = NoPrice;
    int startIndex_ = 0;
    int endIndex_;
    std::vector<Volume> volumes_;
    Price bestPrice_ = NoPrice;
    Volume bestVolume_ = NoVolume;
    bool initialized_ = false;
    bool topChanges_ = false;
};

inline Price Liquidity::getPriceFromIndex(int index) const
{
    int offset = index - startIndex_;

    if (offset < 0)
        offset += size_;

    return refPrice_ + offset * tickSize_;     
}

inline int Liquidity::getIndexFromPrice(Price price) const
{ 
    return (static_cast<int>((price -refPrice_) / tickSize_) + startIndex_) & mask_;
}

inline int Liquidity::getDistanceFromReference(Price price) const
{
    return (price - refPrice_)/ tickSize_;
}

class BidLiquidity : public Liquidity
{
friend class BidLiquidityIterator;
public:
    BidLiquidity(std::size_t size, double tickSize);
    bool update(Price price, Volume volume);
    void copyTo(BidLiquidity &liquidity) const;
private:
    void shiftTowardsHigherPrices(int distance,  Price price, Volume volume);
    bool shiftTowardsLowerPrices(int distance,  Price price, Volume volume);
};


class AskLiquidity : public Liquidity
{
friend class AskLiquidityIterator;
public:
    AskLiquidity(std::size_t size, double tickSize);
    bool update(Price price, Volume volume);
    void copyTo(AskLiquidity &liquidity) const;
private:
    bool shiftTowardsHigherPrices(int distance,  Price price, Volume volume);
    void shiftTowardsLowerPrices(int distance,  Price price, Volume volume);
};

class BidLiquidityIterator
{
friend class BidLiquidity;
public:
    BidLiquidityIterator(const BidLiquidityIterator &dataIter) = default;
private:
    BidLiquidityIterator(const BidLiquidity &liquidity);
    BidLiquidityIterator& operator=(const BidLiquidityIterator &dataIter) = delete;
public:
    PriceVolumePair getNext();
    void reset();
    ~BidLiquidityIterator() = default;
private:
    const std::vector<Volume> &array_;
    int currentPos_ = NoIndex;
    const BidLiquidity &liquidity_;
};

class AskLiquidityIterator
{
public:
    AskLiquidityIterator(const AskLiquidityIterator &dataIter) = default;
private:
    AskLiquidityIterator(const AskLiquidity &liquidity);
    AskLiquidityIterator& operator=(const AskLiquidityIterator &dataIter) = delete;
public:
    PriceVolumePair getNext();
    void reset();
    ~AskLiquidityIterator() = default;
private:
    const std::vector<Volume> &array_;
    int currentPos_ = NoIndex;
    const AskLiquidity &liquidity_;
};

#endif
