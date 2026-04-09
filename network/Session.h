#ifndef SESSION_H
#define SESSION_H

#include <chrono>
#include "Connection.h"
#include "FixTags.h"
#include "MWMRNoOverWriteSlotRingBuffer.h"
#include "MarketData.h"
#include "FixMessage.h"
#include "Events.h"
#include "CommonUtils.h"
#include "MessageBuilder.h"

class NewFixMessageEvent : public EventBase
{
public:
    NewFixMessageEvent(const FixMsgType& message) : EventBase(EventType::NEW_FIX_MESSAGE), message_(&message) {}
    const FixMsgType& getMessage() { return *message_; }
private:
    const FixMsgType* message_ = nullptr;
};

class ConnectionCloseEvent : public EventBase
{
public:
    enum Reason { RING_BUFFER_OVERFLOW, MESSAGE_OVER_SIZE, CONNECTION_ERROR, MESSAGE_PARSE_ERROR, MANY_GAPS};
public:
    ConnectionCloseEvent(Reason reason) : EventBase(EventType::BROKER_CONNECTION_CLOSED) , reason_(reason) { }
    Reason getReason() { return reason_; }
    Reason reason_;
};

template<std::size_t u> requires PowerOfTwo<u> class RawFixMessage
{
public:
    enum Status {VALID, RECIEVING, NOT_VALID};
public:
    std::size_t getArraySize() { return u; }
    char* getMessage() { return message_; }
    const char* getMessage() const { return message_; }
    void setStatus(Status st) { st_ = st; }
    Status getSatus() const { return st_; }
    void setMessageSize(std::size_t size) { msgSize_ = size; }
    std::size_t getMessageSize() const { return msgSize_; }
    void setMessageSeqNum(std::size_t seqNum) { msgSeqNum_ = seqNum; }
    std::size_t getMessageSeqNum() const { return msgSeqNum_; }
private:
    char message_[u];
    Status st_ = NOT_VALID;
    std::size_t msgSize_;
    std::size_t msgSeqNum_ = 0;
};

template<typename MsgParser, typename MsgBuilder, typename RawFixMsg> class Session {
public:
    enum status {OPEN, CLOSING, CLOSED};
    using OutMessageQueue =  MWMRNoOverWriteSlotRingBuffer<OutMessage>;
public:
    Session(std::unique_ptr<MsgParser> messageParser, std::unique_ptr<MsgBuilder> messageBuilder, std::string_view id, std::string_view targetId,  std::string_view host, int port,
        Connection::IpvType ipvType = Connection::IpvType::IPV4, std::size_t outueSize = 256, std::size_t sentQueueSize = 256, std::size_t bufferInSize =  4096, std::size_t ringBufferSize = 8192,
        std::size_t gapMsgBufferSize = 128);
    const ConnectionID& getID() const { return conn_.getID(); }
    template<typename T> bool addMessageToSend(const T& message);
    bool openSession();
    void closeImidietely(ConnectionCloseEvent::Reason reason);
    void readMessages();
    void sendMessages();
    void registerForSessionEvents(Subscriber subscriber) const  {  subscribers_.push_back(subscriber); }

private:
    void handleNewMessage(const FixMsgType& msg);  
    bool appendToRingBuffer(char* array, int size);
    std::size_t getNextMsgSeqNum() { return ++msgSeqNum_; }
    void notifySubscribers(const EventBase& event) const;
    bool copyFromRingBuffer(char *array, std::size_t length) const;
    void updateRecieveTimePoint() { lastRecieved_ = std::chrono::steady_clock::now();}
    void resendReuest(std::size_t startIndex, std::size_t endIndex);

private:
    std::string id_;
    std::string targetId_; 
    std::unique_ptr<MsgParser> messageParser_;
    std::unique_ptr<MsgBuilder> messageBuilder_;    
    Connection conn_;
    OutMessageQueue outQueue_;
    std::vector<OutMessageQueue> sentQueue_;
    std::vector<char> bufferIn_;
    mutable std::vector<Subscriber> subscribers_;
    std::vector<char> ringBuffer_;
    std::size_t ringBufferSize_;
    std::size_t mask_;
    std::size_t start_ = 0;
    std::size_t parseStart_ = 0;
    std::size_t end_ = 0;
    std::size_t msgSeqNum_ = 0;
    std::size_t expIncomingMsgSeqNum_ = 1;
    std::size_t latestIncomingMsgSeqNum_ = 0;
    std::size_t gapMsgBufferSize_;
    std::chrono::seconds heatBeat_ = std::chrono::seconds(30);
    std::chrono::seconds destinationHeartBeat_ = std::chrono::seconds(30);
    std::chrono::steady_clock::time_point lastSentTime_;
    std::chrono::steady_clock::time_point lastRecieved_ ;    
    std::vector<RawFixMsg> gapMsgBuffer_;
    OutMessageQueue::Slot* currentOutMessageSlot_ = nullptr;
    status started_ =  CLOSED;
};

#endif