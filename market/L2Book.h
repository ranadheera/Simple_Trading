#ifndef L2_BOOK_MAP_H
#define L2_BOOK_MAP_H

#include <vector>
#include <atomic>
#include "MarketData.h"
#include "Liquidity.h"

class L2Book
{
public:
    L2Book();
    L2Book(std::size_t size, double tickSize);
    bool setSize(std::size_t size);
    bool setTickSize(double tickSize);
    bool init();
    bool update(const Marketdata &data);
    bool isBestBidChangeWithLastUpdate() const { return bidLiquidity_.isBestChangeWithLastUpdate(); }
    bool isBestAskChangeWithLastUpdate() const { return askLiquidity_.isBestChangeWithLastUpdate(); }
    PriceVolumePair getMaxBidPriceVolume() const { return bidLiquidity_.getBestLiquidity(); }
    PriceVolumePair getMinAskPriceVolume() const { return askLiquidity_.getBestLiquidity(); }
    void getSnapShot(L2Book &l2Book) const;
private:
    BidLiquidity bidLiquidity_;
    AskLiquidity askLiquidity_;
    TimeStamp lastUpdatedTime_;
};
#endif