#ifndef L1BOOK_H
#define L1BOOK_H

#include "Buffer.h"

struct L1BookEntry {
    uint64_t timpstamp;
    double bid_price;
    double ask_price;
    int bid_volume;
    int ask_volume;
};

inline std::ostream& operator<<(std::ostream& os, const L1BookEntry& le) {
    os << le.timpstamp << " " << le.bid_price << " " << le.ask_price << " " << le.bid_volume << " " << le.ask_volume;
    return os;
}


class L1Book : public SWMRArray<L1BookEntry>
{
public:
    L1Book(std::size_t size) : SWMRArray<L1BookEntry>(size) {}
   // void update(int index, const L1Boo& data);
};

#endif