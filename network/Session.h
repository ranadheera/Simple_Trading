#ifndef SESSION_H
#define SESSION_H

#include "Connection.h"
#include "RingBuffer.h"
#include "Message.h"

class Session {
public:
    using OutMessageQueue =  MWMRNonOverridableSlotRingBuffer<OutMessage>;
public:
    Session(const std::string host, int port, Connection::IpvType ipvType = Connection::IpvType::IPV4, 
        std::size_t outueSize = 256, std::size_t sentQueueSize = 256);
    void setFixVersion( FixVersion fixVersoin) { fixVersion_ =  fixVersoin;}
    FixVersion getFixVersion() {return fixVersion_;}
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
    OutMessageQueue sentQueue_;
    std::size_t msgSeqNum_ = 0;
    bool run_ = false;

};

#endif