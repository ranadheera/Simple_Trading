#include "ParsedFixMessage.h"

void ParsedFixMarketData::reset()
{
    for (auto &symboldata : fixMarketData_) {
        symboldata.reset();
    }
}