#ifndef L2_BOOK_MAP_H
#define L2_BOOK_MAP_H

#include <vector>
#include <atomic>
#include "MarketData.h"
#include "L1Book.h"

class SymbolBook
{
public:
    SymbolBook(int lowerbound, int upperbound, int tikSize, L1Book &l1book);
    SymbolBook(const SymbolBook& book);
    bool update(const Marketdata &data);
    bool getMinBidPriceVolume(double &price, int &volume) const;
    bool getMaxAskPriceVolume(double &price, int &volume) const;

private:
    int lowerbound_;
    int upperbound_;
    double tikSize_;
    int size_;
    std::vector<int> bidArray_;
    std::vector<int> askArray_;
    int maxAskValIndex_;
    int minBidValIndex_ ;
    L1Book &l1book_;
    std::atomic<int> updates = 0;
};

class L2Book
{
public:
    L2Book(std::size_t size, L1Book &l1book);
    bool update(std::size_t index, const Marketdata &data);
    bool getMinBidPriceVolume(std::size_t index, double &price, int &volume) const;
    bool getMaxAskPriceVolume(std::size_t index, double &price, int &volume) const;
    SymbolBook getSnapShot(std::size_t index) const;
private:
    std::vector<SymbolBook> symbolBooks_;
};

#endif