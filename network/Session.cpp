#include "Session.h"
#include "Message.h"
#include <chrono>
#include <cstring>

Session::Session(const std::string host, int port, Connection::IpvType ipvType, std::size_t outueSize, std::size_t sentQueueSize):
    conn_(host, port, ipvType), outQueue_(outueSize), sentQueue_(sentQueueSize)
{

}

bool Session::initializeMessage(OutMessage &message)
{
    message.reset();
    auto retVal =  message.addTagValue(FixTags::BeginString, to_string(fixVersion_));

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    message.msgLenPos_ = message.dataSize_;

    retVal =  message.addTagValue(FixTags::BodyLength, "0000");

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    message.bodyStartPos_ = message.dataSize_;
    return true;
}

bool Session::finalizeMessage(OutMessage &message)
{
    auto retVal =  message.addTagValue(FixTags::MsgSeqNum, getNextMsgSeqNum());

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm utc_tm;
    gmtime_r(&now_time, &utc_tm); // convert to UTC
    auto available = message.buffer_.size() - message.dataSize_;

    auto written = snprintf(message.buffer_.data(), available, "52=%04d%02d%02d-%02d:%02d:%02d.%03d%c",
            utc_tm.tm_year + 1900,
            utc_tm.tm_mon + 1,
            utc_tm.tm_mday,
            utc_tm.tm_hour,
            utc_tm.tm_min,
            utc_tm.tm_sec,
            (int)millis.count(),
            OutMessage::SOH);

    if (written < 0)
        return false;

    if (static_cast<size_t>(written) >= available) {
        message.status_ =  OutMessage::MessageStatus::INVALID;
        return false;
    }

    message.dataSize_ += written;

    char msgLength[5];
    written = snprintf(msgLength, 5,
                            "%04zu", (message.dataSize_ - message.bodyStartPos_));

    if (written < 0)
        return false;

    auto msgLenPtr = message.buffer_.data() + message.msgLenPos_+ 2;
    memcpy(msgLenPtr,msgLength,4);

    auto checksum = message.dataSize_ % 256;

    retVal = message.addTagValue(FixTags::CheckSum, checksum);

    if (retVal != OutMessage::UpdateStatus::SUCCESS)
        return false;

    message.status_ = OutMessage::MessageStatus::FINALIZED;
    return true;
}

template<typename T> bool Session::sendMessage(const T& messageBuilder)
{
    auto slotPtr = outQueue_.getWriteSlot();

    if (!slotPtr)
        return false;

    auto message = slotPtr->getData();

    auto initialized = initializeMessage(message);

    if (initialized)
        return false;

    auto buildComplete = messageBuilder.buildMessage(message);

    if (!buildComplete)
        return false;

    auto finalized = finalizeMessage(message);

    if (!finalized)
        return false;

}

bool Session::startSession()
{
    run_ = true;

    while (run_) {

    }
}