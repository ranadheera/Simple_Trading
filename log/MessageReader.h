#ifndef MESSAGE_READER_H
#define MESSAGE_READER_H

#include "LogMessage.h"

class MessageReader 
{
protected:
    MessageReader() {}
    MessageReader(const MessageReader&) = delete;
    MessageReader& operator=(const MessageReader&) = delete;
public: 
    virtual ~MessageReader(){}
public:
    virtual void* getMesssage(char *memory) const = 0;
    virtual std::unique_ptr<MessageReader> clone() const = 0;
};

template<ValidLogMessage T> class MessageReaderImpl : public MessageReader
{
public:
    MessageReaderImpl() : message_(new T()) { }
    MessageReaderImpl(const MessageReaderImpl&) = delete;
    MessageReaderImpl& operator=(const MessageReaderImpl&) = delete;
public:
    virtual ~MessageReaderImpl(){}

    virtual std::unique_ptr<MessageReader> clone() const override 
    {

        return std::unique_ptr<MessageReader>(new  MessageReaderImpl<T>());
    }

    virtual void* getMesssage(char *memory) const override {
        std::memcpy(message_.get(), memory, sizeof(T));
        return message_.get();
    }
private:
    std::unique_ptr<T> message_;
};

#endif