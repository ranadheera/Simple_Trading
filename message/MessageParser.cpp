#include "MessageParser.h"
#include "FixTags.h"
#include <iostream>

using u64 = uint64_t;

// Fast ASCII to int (fixed width)
inline int toInt(const char* p, int len)
{
    int v = 0;
    for (int i = 0; i < len; ++i)
        v = v * 10 + (p[i] - '0');
    return v;
}

inline int64_t daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2)/5 + d - 1;
    const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

TimeStamp fixTimestampToNs(const char* ts)
{
    // YYYYMMDD-HH:MM:SS[.fraction]

    int year  = toInt(ts + 0, 4);
    int month = toInt(ts + 4, 2);
    int day   = toInt(ts + 6, 2);

    int hour  = toInt(ts + 9, 2);
    int min   = toInt(ts + 12, 2);
    int sec   = toInt(ts + 15, 2);

    int64_t days = daysFromCivil(year, month, day);

    auto total_ns = static_cast<TimeStamp>(days) * 86400ull;
    total_ns += hour * 3600ull;
    total_ns += min * 60ull;
        total_ns += sec;

    total_ns *= 1'000'000'000ull;

    // Fractional part
    const char* frac = ts + 17;

    if (*frac == '.')
    {
        ++frac;
        u64 frac_ns = 0;
        int digits = 0;

        while (*frac >= '0' && *frac <= '9' && digits < 9)
        {
            frac_ns = frac_ns * 10 + (*frac - '0');
            ++frac;
            ++digits;
        }

        // scale if fewer than 9 digits
        while (digits < 9)
        {
            frac_ns *= 10;
            ++digits;
        }

        total_ns += frac_ns;
    }

    return total_ns;
}

MessageParser::MessageParser(const TradeSymbols& symbols, const std::vector<SymbolID> &interestedSymbols) :
    symbols_(symbols), parsedMarketData_(symbols.getNumSymbols(), interestedSymbols)
{

}

const ParseStatus& MessageParser::parseHeader(TagValueReader &reader)
{

    if (reader.getRemainBytes() < 16)
        return msgNotComplete_;

    
    int tag;

    if (!reader.getTag(tag) || (tag != BeginString::id_))
        return tagReadError(BeginString::id_);

    char version[20];

    if (!reader.getValue(version, 20))
         return tagValueReadError(BeginString::id_);
    
    FixVersion versionEnum = toEnum(version);

    if (versionEnum != version_)
        return incorrectTagValue(BeginString::id_);
    
    if (!reader.getTag(tag) || (tag != BodyLength::id_))
        return tagReadError(BodyLength::id_);

    int bodyLegth;

    if (!reader.getValue(bodyLegth))
        return tagValueReadError(BodyLength::id_);

    if (bodyLegth + 6 > reader.getRemainBytes())
        return msgNotComplete(); 

    int msgTypeBeginPos = reader.getParseBytes();

    reader.moveReadPosTo( msgTypeBeginPos + bodyLegth);

    auto checksumTagPos =  reader.getParseBytes();

    if (!reader.getTag(tag) || tag != CheckSum::id_)
        return tagReadError(CheckSum::id_);
    
    int checksum;

    if (!reader.getValue(checksum))
        return tagValueReadError(CheckSum::id_);

    auto totalMsgSize = reader.getParseBytes();

    int checkSumCal = 0;

    for (std::size_t i = 0;  i < checksumTagPos; ++i) {
        unsigned char val = reader[i];
        checkSumCal += val;
    }

    checkSumCal = checkSumCal % 256;

    if (checkSumCal != checksum) {
        return incorrectTagValue(CheckSum::id_);
    }

    reader.moveReadPosTo(msgTypeBeginPos);

    headerMessage_.reset();
    headerMessage_.setTotalMsgSize(totalMsgSize);
    return parseHeaderTags(reader, headerMessage_);
}

const ParseStatus& MessageParser::parseBody(TagValueReader &reader)
{
    auto msgTYpe = headerMessage_.getBodyType();


    switch (msgTYpe) {
        case FixMessageType::MARKET_DATA: 
            return parseMarketData(reader);
        case FixMessageType::LOGON:
            return parseLoginSuccess(reader);
            break;
        case FixMessageType::HEART_BEAT:
            return parseHeartBeat(reader);
            break;
        default:
            return incorrectTagValue(MsgType::id_);
    }

}

const ParseStatus& MessageParser::parseHeaderTags(TagValueReader& reader, FixMessageHeader& message)
{
    int tag;

    while(true) {

        if (!reader.getTag(tag))
            return tagReadError(tag);

        switch (tag) {
            case MsgType::id_: {
                char value;

                if (!reader.getValue(value))
                    return tagValueReadError(tag);
                message.setBodyType(static_cast<FixMessageType>(value));
                break;
            }
            case SendingTime::id_: {
                char time[40]; 
                int length = reader.getValue(time, 40);

                if (length > 0) {
                    message.setTimeStamp(fixTimestampToNs(time));
                } else {
                    return tagValueReadError(tag);
                }
                break;
            }
            case SenderCompID::id_: {

                if (!reader.getValue(message.getSenderCompID(), message.getSenderCompIDArrLength())) {
                    return tagValueReadError(tag);
                }
                break;
            }
            case TargetCompID::id_: {
                
                if (!reader.getValue(message.getTargetCompID(),  message.getTargetCompIDArrLength())) {
                    return tagValueReadError(tag);
                }
                break;

            }
            case MsgSeqNum::id_: {
                int msgSeqNum;

                if (reader.getValue(msgSeqNum)) {
                    message.setMessageSeqNum(msgSeqNum);
                } else {
                    return tagValueReadError(tag);
                }
                break;
            }
            case PossDupFlag::id_:
            case PossResend::id_:
            case LastMsgSeqNumProcessed::id_:
            case SenderSubID::id_:
            case TargetSubID::id_:
            case OrigSendingTime::id_:
                reader.moveToNextTag();
                break;
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(&message);  
        }
    }
}

const ParseStatus& MessageParser::parseMarketData(TagValueReader& reader)
{
    parsedMarketData_.reset();

    /*auto& status = parseBaseTags(reader,parsedMarketData_);

    if (status.getType() != ParseStatus::Type::SUCCESS)
        return status;*/

    int tag;
    int numEntries;

    while(true) {

        if (!reader.getTag(tag))
            return tagValueReadError(tag);

        switch (tag) {
            case NoEntries::id_:
                if (!reader.getValue(numEntries)) {
                    return tagValueReadError(tag);
                }
                break;
            case UpdateAction::id_: {
                int updateAction;

                if (!reader.getValue(updateAction)) {
                    return tagValueReadError(tag);
                }

                FixMarketUpdate update;
                update.setUpdateAction(static_cast<UpdateAction::Types>(updateAction));  
                auto &status = parseUpdate(reader, update);

                if (status.getType() != ParseStatus::Type::SUCCESS)
                    return status;

                if (update.getTimeStamp() == 0)
                    update.setTimestamp(headerMessage_.getTimeStamp());
                
                update.setUpdateSeqNum(headerMessage_.getMessageSeqNum());
                
                parsedMarketData_.addMarketData(update);
                break;
            }
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(&parsedMarketData_);
        }
    }

}

const ParseStatus& MessageParser::parseUpdate(TagValueReader& reader, FixMarketUpdate& marketUpdate)
{
    while(true) {
        int tag;

        if (!reader.getTag(tag))
            return tagReadError(tag);

        switch (tag) {
            case EntryType::id_: {
                int entryType;
                if (!reader.getValue(entryType)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setEntryType(static_cast<EntryType::Types>(entryType));
                break;
            }
            case Symbol::id_:{
                char symbol[20]; 
                int length = reader.getValue(symbol, 20);

                if (!length)
                    return tagValueReadError(tag);

                SymbolID id = symbols_.getSymbolID(symbol);
                marketUpdate.setSymbolID(id);
                break;
            }
            case LastPrice::id_:
            case EntryPrice::id_: {
                double price;
                if (!reader.getValue(price)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setPrice(price);
                break;
            }
            case EntryPosition::id_: {
                int position;
                if (!reader.getValue(position)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setPosition(position);
                break;
            }

            case EntryID::id_:
            case RptSeq::id_:
            case NumberOfOrders::id_:
                reader.moveToNextTag();
                break;

            case LastQuantity::id_: 
            case EntrySize::id_: {
                int value;
                if (!reader.getValue(value)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setVolume(value);
                break;
            }
            case TransactionTime::id_: {
                char value[40];
                int length = reader.getValue(value, 40);

                if (!length)
                    return tagValueReadError(tag);

                marketUpdate.setTimestamp(fixTimestampToNs(value));
                break;
            }
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(nullptr);

        }
    }
}

const ParseStatus& MessageParser::parseLoginSuccess(TagValueReader &reader)
{
    loginSuccessMessage_.reset();

    int tag;

    while(true) {

        if (!reader.getTag(tag))
            return tagValueReadError(tag);

        switch (tag) {
            case ResetSeqNumFlag::id_: {
                char val;

                if (!reader.getValue(val)) {
                    return tagValueReadError(tag);
                }

                loginSuccessMessage_.setResetSeqNumFlag(static_cast<ResetSeqNumFlag::Types>(val));
                break;
            }
            case HeartBtInt::id_: {
                int val;

                if (!reader.getValue(val)) {
                    return tagValueReadError(tag);
                }

                loginSuccessMessage_.setHeartBeatInterval(val);
                break;
            }
            case NextExpectedSeqNum::id_: {
                int val;

                if (!reader.getValue(val)) {
                    return tagValueReadError(tag);
                }

                loginSuccessMessage_.setNextExpectedSeqNum(val);
                break;
            }
            case EncryptMethod::id_: {
                int val;

                if (!reader.getValue(val)) {
                    return tagValueReadError(tag);
                }

                loginSuccessMessage_.setEncryptMethod(val);
                break;
            }
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(&loginSuccessMessage_);
        }
    }
        
}

const ParseStatus& MessageParser::parseLogout(TagValueReader &reader)
{
    logoutMessage_.reset();

    int tag;

    while(true) {

        if (!reader.getTag(tag))
            return tagValueReadError(tag);

        switch (tag) {
            case Text::id_: {

                if (!reader.getValue(logoutMessage_.getReason(), logoutMessage_.getReasonArrLength())) {
                    return tagValueReadError(tag);
                }
                break;
            }
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(&logoutMessage_);
        }
    }
}

const ParseStatus& MessageParser::parseHeartBeat(TagValueReader &reader)
{
    heartBeatMessage_.reset();
    
    int tag;

    while(true) {

        if (!reader.getTag(tag))
            return tagValueReadError(tag);

        switch (tag) {
            case TestReqID::id_: {

                auto length = reader.getValue(heartBeatMessage_.getTestID(), heartBeatMessage_.getTestIdArrLength());

                if (!length) {
                    return tagValueReadError(tag);
                }
                break;
            }
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(&heartBeatMessage_);
        }
    }
}
