#include "MessageParser.h"
#include "Message.h"
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

MessageParser::MessageParser(const TradeSymbols& symbols, std::size_t bufferSize) :
    symbols_(symbols), buffer_(bufferSize), bufferSize_(bufferSize), mask_(bufferSize -1), parsedMarketData_(symbols.getNumSymbols())
{

}

bool MessageParser::append(char* array, int size)
{
   auto endIndex_ = end_ & mask_;
   std::size_t remain = bufferSize_ - endIndex_;

   if (remain >= size) {
        memcpy(&buffer_[endIndex_], array, size);
        end_ += size;
   } else {
        memcpy(&buffer_[endIndex_], array, remain);
        auto remainBytesToWrite = size  - remain;
        memcpy(&buffer_[0], array + remain, remainBytesToWrite);
        end_ += size;

        if (end_ - start_ > bufferSize_)
            return false;

        start_ += remainBytesToWrite;
   }

   return true;
}


const ParseStatus& MessageParser::parse()
{
    if (end_ - start_ < 16)
        return msgNotComplete_;

    TagValueReader reader(buffer_, start_, end_, mask_);

    int tag;
    char beginstring[20];

    if (!reader.getTag(tag) || (tag != FixTags::BeginString))
        return tagReadError(FixTags::BeginString);

    int length;

    if (!reader.getValue(beginstring, length))
        return tagValueReadError(FixTags::BeginString);;


    if (!reader.getTag(tag) || (tag != FixTags::BodyLength))
        return tagReadError(FixTags::BodyLength);

    int bodyLegth;

    if (!reader.getValue(bodyLegth))
        return tagValueReadError(FixTags::BodyLength);

    if (bodyLegth + 6 > reader.getRemainBytes())
        return msgNotComplete(); 

    int msgTypeBeginPos = reader.getParseBytes();

    reader.moveReadPosTo( msgTypeBeginPos + bodyLegth);

    auto checksumTagPos = start_ + reader.getParseBytes();

    if (!reader.getTag(tag) || tag != FixTags::CheckSum)
        return tagReadError(FixTags::CheckSum);
    
    int checksum;

    if (!reader.getValue(checksum))
        return tagValueReadError(FixTags::CheckSum);

    int checkSumCal = 0;

    for (auto i = start_; i < checksumTagPos; ++i) {
        unsigned char val = buffer_[i & mask_];
        checkSumCal += val;
    }

    checkSumCal = checkSumCal % 256;

    if (checkSumCal != checksum) {
        return incorrectTagValue(FixTags::CheckSum);
    }

    reader.moveReadPosTo(msgTypeBeginPos);
    
    if (!reader.getTag(tag)) {
        return tagReadError(FixTags::MsgType);
    }

    char messageType;

    if (!reader.getValue(messageType))
        return tagValueReadError(FixTags::MsgType);

    switch (messageType) {

        case 'X':
            start_ = checksumTagPos + 7;
            return parseMarketData(reader);
            break;
        default:
            return incorrectTagValue(FixTags::MsgType);
    }

}

const ParseStatus& MessageParser::parseBaseTags(TagValueReader& reader, BaseFixMessage& message)
{
    int tag;

    while(true) {

        if (!reader.getTag(tag))
            return tagReadError(tag);

        switch (tag) {

            case FixTags::SendingTime: {
                char time[40]; int length;

                if (reader.getValue(time, length)) {
                    time[length] = '\0';
                    message.setTimeStamp(fixTimestampToNs(time));
                } else {
                    return tagValueReadError(tag);
                }
                break;
            }
            case FixTags::SenderCompID: {
                char senderCompID[30]; int length;

                if (reader.getValue(senderCompID, length)) {
                    message.setSenderCompID(senderCompID, length);
                } else {
                    return tagValueReadError(tag);
                }

                break;

            }
            case FixTags::TargetCompID: {
                char targetCompID[30]; int length;
                
                if (reader.getValue(targetCompID, length)) {
                    message.setTargetCompID(targetCompID, length);
                } else {
                    return tagValueReadError(tag);
                }
                break;

            }
            case FixTags::MsgSeqNum: {
                int msgSeqNum;

                if (reader.getValue(msgSeqNum)) {
                    message.setMessageSeqNum(msgSeqNum);
                } else {
                    return tagValueReadError(tag);
                }
                break;
            }
            case FixTags::PossDupFlag:
            case FixTags::PossResend: {
                char value;
                if (!reader.getValue(value)) {
                    return tagValueReadError(tag);
                }
                break;
            }
            case FixTags::LastMsgSeqNumProcessed: {
                int value;
                if (!reader.getValue(value)) {
                    return tagValueReadError(tag);
                }
                break;
            }
            case FixTags::SenderSubID:
            case FixTags::TargetSubID:
            case FixTags::OrigSendingTime: {
                char value[40]; int length;
                if (!reader.getValue(value, length)) {
                    return tagValueReadError(tag);
                }
                break;

            }
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
    auto& status = parseBaseTags(reader,parsedMarketData_);

    if (status.getType() != ParseStatus::Type::SUCCESS)
        return status;

    int tag;
    int numEntries;

    while(true) {

        if (!reader.getTag(tag))
            return tagValueReadError(tag);

        switch (tag) {
            case FixTags::NoEntries:
                if (!reader.getValue(numEntries)) {
                    return tagValueReadError(tag);
                }
                break;
            case FixTags::UpdateAction: {
                int updateAction;

                if (!reader.getValue(updateAction)) {
                    return tagValueReadError(tag);
                }

                FixMarketUpdate update;
                update.setUpdateAction(static_cast<UpdateAction>(updateAction));  
                auto &status = parseUpdate(reader, update);

                if (status.getType() != ParseStatus::Type::SUCCESS)
                    return status;

                if (update.getTimeStamp() == 0)
                    update.setTimestamp(parsedMarketData_.getTimeStamp());
                
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
            case FixTags::EntryType: {
                int entryType;
                if (!reader.getValue(entryType)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setEntryType(static_cast<EntryType>(entryType));
                break;
            }
            case FixTags::Symbol: {
                char symbol[20]; int length;
                if (!reader.getValue(symbol, length)) {
                    return tagValueReadError(tag);
                }
                symbol[length] = '\0';
                SymbolID id = symbols_.getSymbolID(symbol);
                marketUpdate.setSymbolID(id);
                break;
            }
            case FixTags::LastPrice:
            case FixTags::EntryPrice: {
                double price;
                if (!reader.getValue(price)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setPrice(price);
                break;
            }
            case FixTags::EntryPosition: {
                int position;
                if (!reader.getValue(position)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setPosition(position);
                break;
            }

            case FixTags::EntryID: {
                char value[20]; int length;
                if (!reader.getValue(value, length)) {
                    return tagValueReadError(tag);
                }
                break;
            }
            
            case FixTags::RptSeq: {
                int value;
                if (!reader.getValue(value)) {
                    return tagValueReadError(tag);
                }
                break;
            }
            case FixTags::NumberOfOrders: {
                int value;
                if (!reader.getValue(value)) {
                    return tagValueReadError(tag);
                }
                break;
            }
            case FixTags::LastQuantity: 
            case FixTags::EntrySize: {
                int value;
                if (!reader.getValue(value)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setVolume(value);
                break;
            }
            case FixTags::TransactionTime: {
                char value[40]; int length;
                if (!reader.getValue(value, length)) {
                    return tagValueReadError(tag);
                }
                marketUpdate.setTimestamp(fixTimestampToNs(value));
                break;
            }
            default:
                reader.moveReadPosTo(reader.getLastTagPos());
                return success(nullptr);

        }
    }
}

bool TagValueReader::getTag(int &tag)
{
    tag = 0;
    bool retval = false;

    for (auto i = parsePos_; i != end_; ++i) {
        auto index = i & mask_;
        char val = buffer_[index];

        if (val != '=' ) {
            tag = tag * 10 + (val - '0');
        } else {
            retval = true;
            lastTagPos_ = parsePos_ - start_;
            parsePos_ = i + 1;
            break;
        }
    }
    return retval;
}

bool TagValueReader::getValue(int& value)
{
    value = 0;
    bool retval = false;

    for (auto i = parsePos_ ; i != end_; ++i) {
        auto index = i & mask_;
        char val = buffer_[index];

        if (val != OutMessage::SOH ) {
            value = value * 10 + (val - '0');
        } else {
            retval = true;
            parsePos_ = i + 1;
            break;
        }
    }
    return retval;
}

bool TagValueReader::getValue(double& value)
{
    value = 0;
    bool retval = false;
    int decimals = 0;
    bool decimal = false;

    for (auto i = parsePos_ ; i != end_; ++i) {
        auto index = i & mask_;
        char val = buffer_[index];

        if (val != OutMessage::SOH ) {

            if ( val == '.') {
                decimal = true;
                continue;
            }

            value = value * 10 + (val - '0');

            if (decimal)
                ++decimals;

        } else {
            retval = true;
            parsePos_ = i + 1;
            break;
        }
    }

    while (decimals--)
        value = value / 10;

    return retval;
}

bool TagValueReader::getValue(char& value)
{
    bool retval = false;

    for (auto i = parsePos_ ; i != end_; ++i) {
        auto index = i & mask_;
        char val = buffer_[index];

        if (val != OutMessage::SOH ) {
            value = val;
        } else {
            retval = true;
            parsePos_ = i + 1;
            break;
        }
    }
    return retval;
}

bool TagValueReader::getValue(char* value, int &length)
{
    bool retval = false;
    length = 0;

    for (std::size_t i = parsePos_; i != end_; ++i) {
        auto index = i & mask_;
        char val = buffer_[index];

        if (val != OutMessage::SOH ) {
            value[length] = val;
            ++length;
        } else {
            retval = true;
            parsePos_ = i + 1;
            break;
        }
    }
    return retval;
}

bool TagValueReader::moveReadPosTo(std::size_t offset)
{
    auto newReadPos = start_ + offset;

    if (offset >= buffer_.size()) {
        return false;
    }

    parsePos_ = newReadPos;
    return true;
}

bool TagValueReader::moveToNextTag()
{

    for (std::size_t i = parsePos_; i != end_; ++i) {
        auto index = i & mask_;
        char val = buffer_[index];

        if (val == OutMessage::SOH ) {
            parsePos_ = i + 1;
            return true;
        } 
    }
    return false;;
}
