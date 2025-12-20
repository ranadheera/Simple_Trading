#ifndef MARKET_DATA_H
#define MARKET_DATA_H

#include <cstdint>
#include <iostream>

struct Marketdata {
    uint64_t timpstamp;
    double bid_price;
    double ask_price;
    int symbol;
    int bid_volume;
    int ask_volume;
};

inline std::ostream& operator<<(std::ostream& os, const Marketdata& mt) {
    os << mt.timpstamp << " " << mt.symbol << " " << mt.bid_price << " " << mt.ask_price << " " << mt.bid_volume << " " << mt.ask_volume;
    return os;
}

inline int getIndex(const Marketdata &data) {
    return data.symbol;
}

#endif