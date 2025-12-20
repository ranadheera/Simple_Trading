#include "MarketTick.h"

bool MarketTick::update(int index, const Marketdata& data)
{
    Marketdata tmp;
    bool entrPresent = getData(index, tmp);

    if (!entrPresent || data.timpstamp > tmp.timpstamp) {
        SWMRArray<Marketdata>::update(index, data);
        return true;
    }

    return false;
}