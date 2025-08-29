#ifndef MARKET_TICK_H
#define MARKET_TICK_H

#include "Buffer.h"
#include "MarketData.h"

class MarketTick : public SWMRArray<Marketdata>
{
public:
    MarketTick(std::size_t size) : SWMRArray<Marketdata>(size) {}
    bool update(int index, const Marketdata& data);
};

#endif