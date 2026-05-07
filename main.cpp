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
#include "FastRingBuffer.h"
#include "MessageParser.h"
#include "MessageBuilder.h"
#include "Session.h"
#include "Broker.h"

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

    /*LogMgr logmgr(64);
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
    logmgr.stop();*/

    /*FastRingBuffer<int> testbuffer(5);

    for (int i = 0; i < 6; ++i) {
        testbuffer.push(i);
    }

    auto bIter = testbuffer.begin();
    auto eIter = testbuffer.end();

    for ( ; bIter != eIter; bIter += 1) {
        std::cout << *bIter;
    }

    auto rbIter = testbuffer.rbegin();
    auto reIter = testbuffer.rend();

    for ( ; rbIter != reIter; rbIter += 1) {
        std::cout << *rbIter;
    }*/

   /* auto printLiquidity = [](Liquidity &bid)
    {
        std::cout << std::endl;

        auto ib = bid.begin();
        auto ie = bid.end();

        for ( ; ie != ib; ib += 1) {
            std:: cout << *ib << " ";
        }

        auto bestPrice = bid.getBestLiquidity();
        std::cout << std::endl;
        std::cout <<  "BestPrice: " << bestPrice.first << " BestVolume: " << bestPrice.second << " best price changeed: " << bid.isBestChangeWithLastUpdate();
    };

    BidLiquidity bid(4, 0.01);
    bid.update(0.04, 4);
    printLiquidity(bid);
    bid.update(0.05, 5);
    printLiquidity(bid);
    bid.update(0.02, 3);
    printLiquidity(bid);
    bid.update(0.01, 1);
    printLiquidity(bid);
    bid.update(0.06, 6);
    printLiquidity(bid);
    bid.update(0.10, 10);
    printLiquidity(bid);

    BidLiquidity bidcopy(2, 0.01);
    bidcopy.init();
    bid.copyTo(bidcopy);
    printLiquidity(bidcopy);

    
    AskLiquidity ask(4, 0.01);
    ask.update(0.04, 4);
    printLiquidity(ask);
    ask.update(0.05, 5);
    printLiquidity(ask);
    ask.update(0.06, 6);
    printLiquidity(ask);
    ask.update(0.03, 3);
    printLiquidity(ask);
    ask.update(0.02, 2);
    printLiquidity(ask);
    ask.update(0.01, 1);
    printLiquidity(ask);
    ask.update(0.05, 5);
    printLiquidity(ask);

    AskLiquidity askcopy(2, 0.01);
    askcopy.init();
    ask.copyTo(askcopy);
    printLiquidity(askcopy);*/

    /*char array[] = "8=FIX.4.4\x01"
                   "9=5\x01"
                   "35=0\x01"
                   "10=161\x01";*/

    /* char array[] = "8=FIX.4.4\x01"
                   "9=147\x01"
                   "35=X\x01"
                   "34=102\x01"
                   "49=EXCHANGE\x01"
                   "56=CLIENT\x01"
                   "52=20260310-09:30:15.123\x01"
                   "268=2\x01"
                   "279=0\x01"
                   "269=0\x01"
                   "55=AAPL\x01"
                   "270=190.40\x01"
                   "271=500\x01"
                   "290=1\x01"
                   "279=0\x01"
                   "269=2\x01"
                   "55=AAPL\x01"
                   "31=190.40\x01"
                   "32=100\x01"
                   "10=226\x01";*/

    char array[128] = "8=FIX.4.4\x01"
                "9=67\x01"
                "35=A\x01"
                "34=1\x01"
                "49=BROKER\x01"
                "56=CLIENT\x01"
                "52=20260318-10:15:23.456\x01"
                "98=0\x01"
                "108=30\x01"
                "10=136\x01";
    
    /*char array[128] = "8=FIX.4.4\x01"
                    "9=55\x01"
                    "35=0\x01"
                    "34=2\x01"
                    "49=BROKER\x01"
                    "56=CLIENT\x01"
                    "52=20260318-10:30:00.000\x01"
                    "10=069\x01";*/

/*char array2[] = "8=FIX.4.4\x01"
"9=0103\x01"
"35=A\x01"
"49=samitha3\x01"
"56=samitha4\x01"
"34=0001\x01"
"52=20260309-05:16:43.134627\x01"
"553=samitha1\x01"
"554=samitha2\x01"
"98=0\x01"
"108=15\x01"
"10=170\x01";   
    TradeSymbols symbols;
    symbols.addSymbol("AAPL");
    std::vector<SymbolID> interestedSymbols;
    interestedSymbols.push_back(symbols.getSymbolID("AAPL"));
    MessageParser parser(symbols, interestedSymbols);
    
    TagValueReader reader(array2, 0, 127, 127);
    auto &st = parser.parseHeader(reader);

    if (st.getType() == ParseStatus::Type::SUCCESS) {
        auto &success = static_cast<const ParseSuccess&>(st);
        auto &msg = success.getMessage();
        auto &fixUpdate = static_cast<const FixMessageHeader&>(msg);
        std::cout << fixUpdate;

        auto &st = parser.parseBody(reader);


        if (st.getType() == ParseStatus::Type::SUCCESS) {
            auto &success = static_cast<const ParseSuccess&>(st);
            auto &msg = success.getMessage();
            auto &logon = static_cast<const FixLogonMessage&>(msg);
            std::cout << logon << std::endl;
        }
    }*/

    /*MessageBuilder builder(2048, 1000,  MessageBuilder::TimeStampAccuracy::NANO);
    FixLogonMessage logon;
    logon.setUserName("samitha1");
    logon.setPassWord("samitha2");
    logon.setHeartBeatInterval(15);
    OutMessage outMessage;
    builder.addDataToOutMsg(logon, outMessage, "samitha3", "samitha4");
    builder.finalizeOutMessage(outMessage, 1);
    std::cout << outMessage;*/



    SessionConfig<MessageParser, MessageBuilder> mdConfig("MDCLIENT", "MDSERVER", "localhost",5001);
    mdConfig.timeAccuracy_ = MessageBuilder::TimeStampAccuracy::NANO;
    mdConfig.heartBeatInterval_ = 15;
    SessionConfig<MessageParser, MessageBuilder> oeConfig("OECLIENT", "OESERVER", "localhost",5002);
    TradeSymbols symbols;
    symbols.addSymbol("AAPL");
    std::vector<SymbolID> interestedSymbols;
    interestedSymbols.push_back(symbols.getSymbolID("AAPL"));
    SessionWorker sw1;
    Broker broker1(mdConfig, oeConfig, symbols, std::move(interestedSymbols));
    auto st = broker1.openSessoins(sw1, sw1);

    if (!st) {
        std::cout << "can not connect to targets";
    }

    sw1.start();

    while(true) {
        sleep(10);
    }
   return 0;
}
