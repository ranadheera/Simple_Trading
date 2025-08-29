#include "L1Book.h"

/*void L1Book::update(int index, const Marketdata& data)
{
    L1BookEntry tmp;
    bool entrPresent = getData(index, tmp);

    if (!entrPresent || data.timpstamp > tmp.timpstamp) {
        tmp.bid_price = data.bid_price;
        tmp.ask_price = data.ask_price;
        tmp.bid_volume = data.bid_volume;
        tmp.ask_volume = data.ask_volume;
        tmp.timpstamp = data.timpstamp;
        SWMRArray<L1BookEntry>::update(index, tmp);
    }

    //SWMRArray<L1BookEntry>::update(index, tmp);
}*/