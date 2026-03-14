#ifndef SESSION_H
#define SESSION_H

#include "Connection.h"
#include "Message.h"
#include "MWMRNoOverWriteSlotRingBuffer.h"
#include "MarketData.h"
#include "ParsedFixMessage.h"

class Session {
public:
    using OutMessageQueue =  MWMRNoOverWriteSlotRingBuffer<OutMessage>;
public:
    Session(std::string_view host, int port, Connection::IpvType ipvType = Connection::IpvType::IPV4, 
        std::size_t outueSize = 256, std::size_t sentQueueSize = 256, std::size_t bufferInSize = 8192);
    void setFixVersion( FixVersion fixVersoin) { fixVersion_ =  fixVersoin;}
    FixVersion getFixVersion() const {return fixVersion_;}
    const ConnectionID& getID() const { return conn_.getID(); }
    template<typename T> bool sendMessage(const T& messageBuilder);
    bool startSession();
    bool stopSession();

private:
    bool initializeMessage(OutMessage &message);
    bool finalizeMessage(OutMessage &message);
    std::size_t getNextMsgSeqNum() { return ++msgSeqNum_; }

private:
    Connection conn_;
    FixVersion fixVersion_ = FixVersion::FIX44;
    OutMessageQueue outQueue_;
    std::vector<OutMessageQueue> sentQueue_;
    std::vector<char> bufferIn_;
    std::size_t msgSeqNum_ = 0;
    bool started_ =  false;
    bool run_ = false;


};

#endif