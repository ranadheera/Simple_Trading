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
#include "L2Book.h"
#include "MarketTick.h"
#include "StrategyEngine.h"
#include "LogMgr.h"


int main() {

    /*SymbolIds SymbolIdCache("/home/samitha/Downloads/LabelId.csv");

    if (!SymbolIdCache.isReady()) {
        std::cout << "Lables not redy";
        return -1;
    }

    CsvParser parser("/home/samitha/Downloads/market_data_500k.csv");

    if (!parser.isReady()) {
        std::cout << "parser not ready";
        return 0;
    }

    SWSRRingBuffer<Marketdata>  buffer(500001);
    MarketTick marketTick(SymbolIdCache.getNumSymbols());
    L1Book l1book(SymbolIdCache.getNumSymbols());
    L2Book l2book(SymbolIdCache.getNumSymbols(), l1book);
    BookUpdaters bookUpdaters(l1book, l2book, 2, 1000);
    StrategyEngine strategyEngine(marketTick, SymbolIdCache.getNumSymbols(), 2);
    Dispatcher dispatcher(buffer, marketTick, bookUpdaters, strategyEngine);

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

    dispatcher.stop();
    Marketdata d;
    int i = 0;
    std::cout << "-------\n";

    for (int i = 0;  i < SymbolIdCache.getNumSymbols(); ++i) {
        if (marketTick.getData(i, d)) {
            std::cout << d << "\n";
        }
    }


    L1BookEntry l;
    std::cout << "++++++++\n";
    for (int i = 0;  i < SymbolIdCache.getNumSymbols(); ++i) {
        if (l1book.getData(i, l)) {
            std::cout << l << "\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end -start);
    std::cout << "Time : " << duration.count() << std::endl;*/

    LogMgr logmgr(64);
    LogFileContext sysLogcontext("system");
    sysLogcontext.setFileMaxMessageCount(16);
    sysLogcontext.setBatchWriteMessageCount(4);
    logmgr.addLogFileContext(LogFileType::SYSTEM, sysLogcontext);
    logmgr.registerLogMessage<ConnectionSuccess1>(MessageType::CONNECTION_SUCCESS_1);
    logmgr.registerLogMessage<FileEnd1>(MessageType::FILE_END_1);

    if (!logmgr.start())
        std::cout << "Log manager failed to start" << std::endl;

    for (int i = 0; i < 64; ++i) {
        char hostName[20];
        auto st = snprintf(hostName, 20,
                            "host %d", i);
        if (st < 0) {
            std::cout << "name generating failed" << std::endl;
        }

        auto msg = ConnectionSuccess1::make(hostName, 1);
        logmgr.logMessage(LogFileType::SYSTEM, msg);
    }
    sleep(5);
    auto fileReader = logmgr.getLogFileReader(LogFileType::SYSTEM, "system_1.bin");

    if (fileReader) {
        auto typeMsgPair = fileReader->getNextMessage();

        while (typeMsgPair.first) {
            if (typeMsgPair.second == MessageType::CONNECTION_SUCCESS_1) {
                auto conSuccessMsg = static_cast<ConnectionSuccess1*>(typeMsgPair.first);
                std::cout << conSuccessMsg->getHostName() << " " << conSuccessMsg->getPort() << std::endl;
            }
            typeMsgPair = fileReader->getNextMessage();
        }
    }
    logmgr.stop();
    return 0;
}
