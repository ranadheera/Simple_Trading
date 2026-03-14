#include <vector>
#include "FastRingBuffer.h"
#include "ParsedFixMessage.h"
#include <cstring>

class TagValueReader;

class ParseStatus
{
public:
    enum class Type {SUCCESS, TAG_READ_ERROR, TAG_VALUE_READ_ERROR, MESSAGE_NOT_COMPLETE, INCORRECT_TAG_VALUE};
protected:
    ParseStatus (Type type) : type_(type) {}
public:
    Type getType() const { return type_; }
private:
    Type type_;
};

class MesssageNotComplete : public ParseStatus
{
public:
    MesssageNotComplete() : ParseStatus(Type::MESSAGE_NOT_COMPLETE) {}
};

class TagReadError : public ParseStatus
{
public:
    TagReadError() : ParseStatus(Type::TAG_READ_ERROR) {}
    void setTag(int tag) { tag_ = tag; }
    int getTag() const { return tag_; }
private:
    int tag_ = -1;
};

class TagValueReadError : public ParseStatus
{
public:
    TagValueReadError() : ParseStatus(Type::TAG_VALUE_READ_ERROR) {}
    void setTag(int tag) { tag_ = tag; }
    int getTag() const { return tag_; }
private:
    int tag_ = -1;
};

class IncorrectTagValue : public ParseStatus
{
public:
    IncorrectTagValue() : ParseStatus(Type::INCORRECT_TAG_VALUE) {}
    void setTag(int tag) { tag_ = tag; }
    int getTag() const { return tag_; }
private:
    int tag_ = -1;
};

class ParseSuccess : public ParseStatus
{
public:
    ParseSuccess() : ParseStatus(Type::SUCCESS) {}
    void setMessage(const BaseFixMessage *message) { message_ = message; }
    const BaseFixMessage&  getMessage() const { return *message_; }
private:
    const BaseFixMessage* message_ = nullptr;

};

class MessageParser
{
public:
    MessageParser(const TradeSymbols& symbols, std::size_t bufferSize);
public:
    bool append(char* array, int size);
    const ParseStatus& parse();
private:
    const ParseStatus& parseMarketData(TagValueReader &reader);
    const ParseStatus& parseUpdate(TagValueReader& reader, FixMarketUpdate& marketUpdate);
    const ParseStatus& parseBaseTags(TagValueReader& reader, BaseFixMessage& message);
    const TagReadError& tagReadError(int tag) { tagReadError_.setTag(tag); return tagReadError_; }
    const TagValueReadError& tagValueReadError(int tag) { tagValueReadError_.setTag(tag); return tagValueReadError_; }
    const MesssageNotComplete& msgNotComplete() { return msgNotComplete_; }
    const IncorrectTagValue& incorrectTagValue(int tag) {incorrectTagValue_.setTag(tag); return incorrectTagValue_;}
    const ParseSuccess& success(const BaseFixMessage* msg) { success_.setMessage(msg); return success_; }
private:
    const TradeSymbols &symbols_;
    std::vector<char> buffer_;
    std::size_t bufferSize_;
    std::size_t mask_;
    std::size_t start_ = 0;
    std::size_t parseStart_ = 0;
    std::size_t end_ = 0;
    ParsedFixMarketData parsedMarketData_;
    TagReadError tagReadError_;
    TagValueReadError tagValueReadError_;
    MesssageNotComplete msgNotComplete_;
    IncorrectTagValue incorrectTagValue_;
    ParseSuccess success_;
};

class TagValueReader
{
public:
    TagValueReader(std::vector<char> &buffer, std::size_t start, std::size_t end, std::size_t mask) : buffer_(buffer), start_(start), end_(end), mask_(mask) {}
    bool getTag(int& tag);
    bool getValue(int& value);
    bool getValue(double& value);
    bool getValue(char& value);
    bool getValue(char* value, int &length);
    bool moveReadPosTo(std::size_t offset);
    bool moveToNextTag();
    bool empty() { return start_ == end_; }
    std::size_t getLastTagPos() { return lastTagPos_ & mask_; }
    std::size_t getParseBytes() { return parsePos_ - start_; }
    std::size_t getRemainBytes() { return end_ - parsePos_; }
private:
    std::vector<char> &buffer_;
    std::size_t mask_;
    std::size_t start_;
    std::size_t end_;
    std::size_t parsePos_ = 0;
    std::size_t lastTagPos_ = 0;
};
