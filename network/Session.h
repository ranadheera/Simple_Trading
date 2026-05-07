#ifndef SESSION_H
#define SESSION_H

#include <cstring>
#include <chrono>
#include <type_traits>
#include "Connection.h"
#include "FixTags.h"
#include "MWMRNoOverWriteSlotRingBuffer.h"
#include "MarketData.h"
#include "FixMessage.h"
#include "Events.h"
#include "CommonUtils.h"
#include "MessageBuilder.h"
#include "MessageParser.h"

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
    enum Reason { RING_BUFFER_OVERFLOW, MESSAGE_OVER_SIZE, CONNECTION_ERROR, MESSAGE_PARSE_ERROR, MANY_GAPS, BROKER_LOGOUT, FAILED_TO_ADD_OUT_QUEUE};
public:
    ConnectionCloseEvent(Reason reason) : EventBase(EventType::BROKER_CONNECTION_CLOSED) , reason_(reason) { }
    Reason getReason() { return reason_; }
    Reason reason_;
};

class RawFixMessage
{
public:
    enum Status {VALID, RECIEVING, NOT_VALID};
public:
    RawFixMessage(std::size_t arrSize = 1024) {
        if (!isPowerOfTwo(arrSize)) {
            throw std::invalid_argument("Size ahould be power of two");
        }
        message_.reserve(arrSize);
    }
public:
    std::size_t getArraySize() { return message_.capacity(); }
    char* getMessage() { return message_.data(); }
    const char* getMessage() const { return message_.data(); }
    void setStatus(Status st) { st_ = st; }
    Status getSatus() const { return st_; }
    void setMessageSize(std::size_t size) { msgSize_ = size; }
    std::size_t getMessageSize() const { return msgSize_; }
    void setMessageSeqNum(std::size_t seqNum) { msgSeqNum_ = seqNum; }
    std::size_t getMessageSeqNum() const { return msgSeqNum_; }
private:
    std::vector<char> message_;
    Status st_ = NOT_VALID;
    std::size_t msgSize_;
    std::size_t msgSeqNum_ = 0;
};

template<typename MsgParserT, typename MsgBuilderT> struct SessionConfig
{
public:
    using MsgParser = MsgParserT;
    using MsgBuilder = MsgBuilderT;
public:
    SessionConfig(std::string_view id, std::string_view targetId, std::string_view host, int port) : id_(id), targetId_(targetId), host_(host), port_(port) {}
    std::string id_;
    std::string targetId_;
    std::string host_;
    int port_;
    std::string userName_;
    std::string passWord_;
    TimeStamp heartBeatInterval_ = 30;
    Connection::IpvType ipvType_ = Connection::IpvType::IPV4;
    std::size_t outMessageBufferSize_ = 2048;
    std::size_t outQueueSize_ = 256;
    std::size_t sentQueueSize_ = 256;
    std::size_t socketReadBufferSize_ = 4096;
    std::size_t dataInRingBufferSize_ = 8192;
    std::size_t gapMsgQueueSize_ = 128;
    std::size_t gapMsgBufferSize_ = 1024;
    MessageBuilder::TimeStampAccuracy timeAccuracy_ = MessageBuilder::TimeStampAccuracy::MICRO;
    std::size_t outMessageMaxBodyLength_ = 2048;
    std::size_t maxOutMessageSeqNo_ = 1000000000;
};

template<typename T> struct isSessionConFig : std::false_type{};

template<typename T1, typename T2> struct isSessionConFig<SessionConfig<T1, T2>> :std::true_type {};

template<typename ConfigType> class Session
{
    static_assert(isSessionConFig<ConfigType>::value);
private:
    using MsgParser = typename ConfigType::MsgParser;
    using MsgBuilder = typename ConfigType::MsgBuilder;
public:
    enum class status {CONNECTING, CONNECTED, LOGGEDIN, LOGGEDOUT, DISCONNECTED};
    using OutMessageQueue =  MWMRNoOverWriteSlotRingBuffer<OutMessage>;
public:
    Session(std::unique_ptr<MsgParser> messageParser, std::unique_ptr<MsgBuilder> messageBuilder, const ConfigType& config);
    const ConnectionID& getID() const { return conn_.getID(); }
    template<typename T> bool addMessageToSend(T& message);
    bool openSession();
    void closeImidietely(ConnectionCloseEvent::Reason reason);
    status checkStatus();
    bool isSessionReady();
    void readMessages();
    void sendMessages();
    void sendHeartBeat();
    void registerForSessionEvents(Subscriber subscriber) const  {  subscribers_.push_back(subscriber); }

private:
    void handleNewMessage(const FixMsgType& msg);  
    bool appendToRingBuffer(char* array, int size);
    std::size_t getNextMsgSeqNum() { return ++msgSeqNum_; }
    void notifySubscribers(const EventBase& event) const;
    bool copyFromRingBuffer(char *array, std::size_t length) const;
    void updateRecieveTimePoint() { lastRecieved_ = std::chrono::steady_clock::now();}
    void resendReuest(std::size_t startIndex, std::size_t endIndex);
    void printFromRingBuffer(std::ostream& os, char *array, std::size_t length ) const;
private:
    std::unique_ptr<MsgParser> messageParser_;
    std::unique_ptr<MsgBuilder> messageBuilder_;  
    std::string id_;
    std::string targetId_;   
    Connection conn_;
    std::string userName_;
    std::string passWord_;
    OutMessageQueue outQueue_;
    std::vector<OutMessage> sentQueue_;
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
    std::size_t gapMsgQueueSize_;
    std::chrono::seconds heartBeat_;
    std::chrono::seconds destinationHeartBeat_ = std::chrono::seconds(30);
    std::chrono::steady_clock::time_point lastSentTime_;
    std::chrono::steady_clock::time_point lastRecieved_;
    char testString_[50];
    char testStringLength_ = 0;
    std::vector<RawFixMessage> gapMsgQueue_;
    OutMessageQueue::Slot* currentOutMessageSlot_ = nullptr;
    status sessionStatus_ =  status::DISCONNECTED;
};

template<typename ConfigType> Session<ConfigType>::Session(std::unique_ptr<MsgParser> messageParser, std::unique_ptr<MsgBuilder> messageBuilder, const ConfigType& config):
                                         messageParser_(std::move(messageParser)), messageBuilder_(std::move(messageBuilder)), id_(config.id_), targetId_(config.targetId_),
                                         conn_(config.host_, config.port_, config.ipvType_), userName_(config.userName_), passWord_(config.passWord_),
                                         outQueue_(config.outQueueSize_, OutMessage(config.outMessageBufferSize_)), sentQueue_(config.sentQueueSize_, OutMessage(config.outMessageBufferSize_)),
                                         bufferIn_(config.socketReadBufferSize_), ringBuffer_(config.dataInRingBufferSize_), ringBufferSize_(config.dataInRingBufferSize_),
                                         mask_(ringBufferSize_ -1), gapMsgQueueSize_(config.gapMsgQueueSize_), heartBeat_(config.heartBeatInterval_),
                                         gapMsgQueue_(config.gapMsgQueueSize_, RawFixMessage(config.gapMsgQueueSize_))
{
    subscribers_.reserve(1);
}

template<typename ConfigType> template<typename T> 
bool Session<ConfigType>::addMessageToSend(T& message)
{
    auto slotPtr = outQueue_.getWriteSlot();

    if (!slotPtr)
        return false;

    auto &outMessage = slotPtr->getData();
    outMessage.reset();

    bool dataAdded = messageBuilder_->addDataToOutMsg(message, outMessage, id_, targetId_);

    if (!dataAdded)
        return false;

    outQueue_.setWriteComplete(slotPtr);
    return true;

}

template<typename ConfigType> void Session<ConfigType>::sendMessages()
{
    while (isSessionReady()) {

        if (!currentOutMessageSlot_) {
            currentOutMessageSlot_ =  outQueue_.getReadSlot();

            if (!currentOutMessageSlot_)
                return;

            auto &outMessage = currentOutMessageSlot_->getData();
            messageBuilder_->finalizeOutMessage(outMessage, getNextMsgSeqNum());

        }

        auto &outMessage = currentOutMessageSlot_->getData();
        std::cout << "sending message " << outMessage << std::endl;
        int st = conn_.send(outMessage.buffer_.data() + outMessage.sentCount_, outMessage.dataSize_ - outMessage.sentCount_);

        if (st > 0) {
            outMessage.sentCount_ += st;
            lastSentTime_ = std::chrono::steady_clock::now();

            if (outMessage.sentCount_ == outMessage.dataSize_) {
                outQueue_.setReadComplete(currentOutMessageSlot_);
                currentOutMessageSlot_ = nullptr;
            }

        } else if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
            closeImidietely(ConnectionCloseEvent::Reason::MESSAGE_OVER_SIZE);
        }

    }

}

template<typename ConfigType> bool Session<ConfigType>::openSession()
{
    auto st = conn_.connect();

    if (st == Connection::Status::CONNECTING) {
        sessionStatus_ = status::CONNECTING;
    } else if (st == Connection::Status::CONNECTED) {
        sessionStatus_ = status::CONNECTED;
    } else {
        return false;
    }

    auto& msg = messageBuilder_->getLogonMessage();
    msg.setResetSeqNumFlag(ResetSeqNumFlag::Types::YES);
    msg.setHeartBeatInterval(heartBeat_.count());
    msg.setUserName(userName_);
    msg.setPassWord(passWord_);

    if (!addMessageToSend(msg)) {
        std::cout << "Failed add message to outgoing  Queue failed.";
        return false;
    }

    return true;
}

template<typename ConfigType> Session<ConfigType>::status Session<ConfigType>::checkStatus()
{
    if (sessionStatus_ == status::CONNECTING) {
        auto st = conn_.checkStatus();

        if (st == Connection::Status::CONNECTING) {
            sessionStatus_ = status::CONNECTING;
        } else if (st == Connection::Status::CONNECTED) {
            sessionStatus_ = status::CONNECTED;
        }
    }
    return sessionStatus_;
}

template<typename ConfigType> bool Session<ConfigType>::isSessionReady()
{
    auto status = checkStatus();
    return (status == status::CONNECTED || status == status::LOGGEDIN);
}

template<typename ConfigType> void Session<ConfigType>::readMessages()
{

    while (isSessionReady()) {
        auto numBytes = conn_.receive(bufferIn_.data(), bufferIn_.size());

        if (numBytes > 0) {

            if (!appendToRingBuffer(bufferIn_.data(), numBytes)) {
                std::cout << "Failed append incoming data to ring buffer" << std::endl;
                closeImidietely(ConnectionCloseEvent::Reason::RING_BUFFER_OVERFLOW);
                return;
            }

            TagValueReader reader(ringBuffer_.data(), parseStart_, end_, mask_);
            const ParseStatus &sth = messageParser_->parseHeader(reader);
            auto typeh = sth.getType();

            if (typeh == ParseStatus::Type::SUCCESS) {
                updateRecieveTimePoint();
                auto& successH = static_cast<const ParseSuccess&>(sth);
                auto &msgH = static_cast<const FixMessageHeader&>(successH.getMessage());
                auto seqNum = msgH.getMessageSeqNum();
                auto msgLength = msgH.getTotalMsgSize();
                
                if (seqNum > latestIncomingMsgSeqNum_)
                    latestIncomingMsgSeqNum_ = seqNum;

                if (seqNum == expIncomingMsgSeqNum_) {

                    const ParseStatus &stb = messageParser_->parseBody(reader);
                    auto typeb = stb.getType();

                    if (typeb == ParseStatus::Type::SUCCESS) {
                        auto& successB = static_cast<const ParseSuccess&>(stb);
                        auto &msgH = successH.getMessage();
                        handleNewMessage(msgH);
                        ++expIncomingMsgSeqNum_;
                    } else if (typeb == ParseStatus::Type::TAG_READ_ERROR) {
                        closeImidietely(ConnectionCloseEvent::Reason::MESSAGE_PARSE_ERROR);
                        return;
                    }

                } else if (seqNum > expIncomingMsgSeqNum_) {

                    auto gapBufferIndex = seqNum % gapMsgQueueSize_;
                    auto& gapBufferEntry = gapMsgQueue_[gapBufferIndex];
                    auto msgArraySize = gapBufferEntry.getArraySize();
                    auto gspSeqNum = gapBufferEntry.getMessageSeqNum();
                    auto status= gapBufferEntry.getSatus();

                    if ((status != RawFixMessage::Status::NOT_VALID)) {
                        closeImidietely(ConnectionCloseEvent::Reason::MANY_GAPS);
                        return;
                    }

                    if (msgLength > msgArraySize) {
                        closeImidietely(ConnectionCloseEvent::Reason::MESSAGE_OVER_SIZE);
                        return;
                    } 

                    char* msgArray = gapBufferEntry.getMessage();
                    copyFromRingBuffer(msgArray, msgH.getTotalMsgSize());
                    gapBufferEntry.setMessageSeqNum(seqNum);
                    gapBufferEntry.setStatus(RawFixMessage::Status::VALID);
                }

                start_ += msgLength;
                parseStart_ += msgLength;

            } else if (typeh == ParseStatus::Type::MESSAGE_NOT_COMPLETE) {
                continue;
            } else {
                closeImidietely(ConnectionCloseEvent::Reason::MESSAGE_PARSE_ERROR);
                return;
            }

        } else if ((numBytes == 0) || ((numBytes < 0) && (errno != EWOULDBLOCK) && (errno != EAGAIN))) {
            closeImidietely(ConnectionCloseEvent::Reason::CONNECTION_ERROR);
            return;
        } else {
            break;
        }
    }

    std::size_t count = 0;

    for (auto i = expIncomingMsgSeqNum_ ; i <= latestIncomingMsgSeqNum_; ++i) {
        auto gapBufferIndex = i % gapMsgQueueSize_;
        auto& gapBufferEntry = gapMsgQueue_[gapBufferIndex];
        auto seqNum = gapBufferEntry.getMessageSeqNum();
        auto status = gapBufferEntry.getSatus();

        if (status != RawFixMessage::Status::NOT_VALID) {
            break;
        }
        gapBufferEntry.setMessageSeqNum(i);
        gapBufferEntry.setStatus(RawFixMessage::Status::RECIEVING);
        ++count;
    }

    for (auto i = expIncomingMsgSeqNum_ ; i <= latestIncomingMsgSeqNum_; ++i) {
        auto gapBufferIndex = i % gapMsgQueueSize_;
        auto& gapBufferEntry = gapMsgQueue_[gapBufferIndex];
        auto seqNum = gapBufferEntry.getMessageSeqNum();
        auto status = gapBufferEntry.getSatus();

        if ((seqNum != i ) || (status != RawFixMessage::Status::VALID)) {
            break;
        }
        
        TagValueReader reader(gapBufferEntry.getMessage(), 0, gapBufferEntry.getMessageSize(), gapBufferEntry.getMessageSize() - 1);
        messageParser_->parseHeader(reader);
        const ParseStatus &stb = messageParser_->parseBody(reader);
        auto typeb = stb.getType();

        if (typeb == ParseStatus::Type::SUCCESS) {
            auto& successB = static_cast<const ParseSuccess&>(stb);
            auto &msgB = successB.getMessage();
            handleNewMessage(msgB);
            ++expIncomingMsgSeqNum_;
        } else if (typeb == ParseStatus::Type::TAG_READ_ERROR) {
            closeImidietely(ConnectionCloseEvent::Reason::MESSAGE_PARSE_ERROR);
            return;
        }

        gapBufferEntry.setStatus(RawFixMessage::Status::NOT_VALID);
    }

}

template<typename ConfigType> void Session<ConfigType>::handleNewMessage(const FixMsgType& msg)
{
    auto type = msg.getMessageType();

    if (type == FixMessageType::LOGON) {
        auto &loginMsg = static_cast<const FixLogonMessage&>(msg);
        destinationHeartBeat_ = std::chrono::seconds(loginMsg.getHeartBeatInterval());
        sessionStatus_ = status::LOGGEDIN;
    } else if (type == FixMessageType::LOGOUT) {
        sessionStatus_ = status::LOGGEDOUT;
        closeImidietely(ConnectionCloseEvent::BROKER_LOGOUT);
        return;
    } else if (type == FixMessageType ::HEART_BEAT) {
        auto& htBtmsg = static_cast<const FixHeartBeatMessage&>(msg);
        auto testId = htBtmsg.getTestID();
        strcpy(testString_, testId); 
    } else if (type == FixMessageType::MARKET_DATA) {

    }
}

template<typename ConfigType> void Session<ConfigType>::closeImidietely(ConnectionCloseEvent::Reason reason)
{
    conn_.disconnect();
    sessionStatus_ =  status::DISCONNECTED;
    notifySubscribers(ConnectionCloseEvent(reason));
}

template<typename ConfigType> void Session<ConfigType>::notifySubscribers(const EventBase& event) const
{
    for (auto& subscriber : subscribers_) {
        subscriber.notify(event);
    }
}

template<typename ConfigType> bool Session<ConfigType>::appendToRingBuffer(char* array, int size) 
{
    auto endIndex_ = end_ & mask_;
   std::size_t remain = ringBufferSize_ - endIndex_;

   if (remain >= size) {
        memcpy(&ringBuffer_[endIndex_], array, size);
        end_ += size;
   } else {
        memcpy(&ringBuffer_[endIndex_], array, remain);
        auto remainBytesToWrite = size  - remain;
        memcpy(&ringBuffer_[0], array + remain, remainBytesToWrite);
        end_ += size;

        if (end_ - start_ > ringBufferSize_)
            return false;

        start_ += remainBytesToWrite;
   }

   return true;
}

template<typename ConfigType> bool Session<ConfigType>::copyFromRingBuffer(char *array, std::size_t length) const
{
    auto startIndex_ = start_ & mask_;

   if (startIndex_ + length <= ringBufferSize_) {
        memcpy(array, &ringBuffer_[startIndex_], length);
   } else {
        auto l1 = ringBufferSize_ - startIndex_;
        memcpy(array, &ringBuffer_[startIndex_], l1);
        memcpy(array + l1, &ringBuffer_[0], length - l1);
   }

   return true;
};

template<typename ConfigType> void Session<ConfigType>::sendHeartBeat()
{
    auto duration = std::chrono::steady_clock::now() - lastSentTime_;
    auto durationS = std::chrono::duration_cast<std::chrono::seconds>(duration);

    if (durationS < destinationHeartBeat_)
        return;

    auto &heartBtMsg = messageBuilder_->getHeartBeatMessage();
    auto testId = heartBtMsg.getTestID();
    strcpy(testId, testString_);

    if (!addMessageToSend(heartBtMsg)) {
        std::cout << "Failed add message to outgoing Queue.";
        closeImidietely(ConnectionCloseEvent::Reason::FAILED_TO_ADD_OUT_QUEUE);
        return;
    }

    sendMessages();
}

template<typename ConfigType> void Session<ConfigType>::printFromRingBuffer(std::ostream& os, char *array, std::size_t length ) const
{
    for (auto i = 0; i <= length; ++i) {
        char c = ringBuffer_[(i + start_) & mask_];

        if (c == SOH)
            c = '|';

        os << c;
    }
}

template<typename ConfigType> void  Session<ConfigType>::resendReuest(std::size_t startIndex, std::size_t endIndex)
{

}

#endif