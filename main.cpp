#include "RingBuffer.h"
#include "CsvParser.h"
#include <iostream>
#include <fstream>
#include <charconv>
#include <chrono>
#include <unordered_map>
#include "SymbolIds.h"
#include "MarketData.h"
#include "Buffer.h"
#include "Dispatcher.h"
#include "Updaters.h"


int main() {

    SymbolIds SymbolIdCache("/home/samitha/Downloads/LabelId.csv");

    if (!SymbolIdCache.isReady()) {
        std::cout << "Lables not redy";
        return -1;
    }

    CsvParser parser("/home/samitha/Downloads/market_data_500k.csv");

    if (!parser.isReady()) {
        std::cout << "parser not ready";
        return 0;
    }

    RingBuffer<Marketdata>  buffer(500001);
    L1Buffer l1buffer(SymbolIdCache.getNumSymbols());
    L1Book l1book(SymbolIdCache.getNumSymbols());
    UpdatersList updaterList(l1buffer, l1book, 2, 1000);
    updaterList.start();
    Dispatcher dispatcher(buffer, updaterList);
    dispatcher.start();

    auto start = std::chrono::high_resolution_clock::now();
    char *ptr;
    std::size_t length;


        for (int i = 0; i < 6; ++i)
            parser.getNextToken(ptr, length);

        while(parser.getNextToken(ptr, length)) {
            Marketdata d;
            std::from_chars(ptr, ptr+length, d.timpstamp);
            parser.getNextToken(ptr, length);
            auto lable = std::string(ptr,length);
            d.symbol = SymbolIdCache.getId(lable);

            if (d.symbol == -1) {
                std::cout << "ignore packet for : " << lable << "\n";
            }

            parser.getNextToken(ptr, length);
            std::from_chars(ptr, ptr+length, d.bid_price);
            parser.getNextToken(ptr, length);
            std::from_chars(ptr, ptr+length, d.ask_price);
            parser.getNextToken(ptr, length);
            std::from_chars(ptr, ptr+length, d.bid_volume);
            parser.getNextToken(ptr, length);
            std::from_chars(ptr, ptr+length, d.ask_volume);
            buffer.write(std::move(d));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end -start);
        std::cout << "Time : " << duration.count() << std::endl;

        Marketdata d;
        L1BookEntry l;
        int i = 0;

        while(1) {
            std::cout << "-------\n";

            for (int i = 0;  i < SymbolIdCache.getNumSymbols(); ++i) {
                if (l1buffer.getData(i, d)) {
                    std::cout << d << "\n";
                }
            }

            std::cout << "++++++++\n";

            for (int i = 0;  i < SymbolIdCache.getNumSymbols(); ++i) {
                if (l1book.getData(i, l)) {
                    std::cout << l << "\n";
                }
            }
        }



    return 0;
}