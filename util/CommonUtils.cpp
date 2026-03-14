#include "CommonUtils.h"

SymbolID TradeSymbols::addSymbol(std::string_view symbolName)
{
    int symbolId = symbolIdvsName_.size();

    auto status = symbolNameVsIDs_.insert(std::make_pair(symbolName, symbolId));

    if (status.second) {
        symbolIdvsName_.push_back(std::string(symbolName));
        return symbolId;
    }

    return status.first->second;
}