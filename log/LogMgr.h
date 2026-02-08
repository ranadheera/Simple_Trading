#ifndef LOG_MGR_H
#define LOG_MGR_H

#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <memory>
#include <type_traits>
#include "MWMRNoOverWriteSlotRingBuffer.h"
#include "LogMessage.h"
#include "MessageReader.h"

enum class LogFileType
{
    SYSTEM = 0,
    COUNT
};

class LogFileContext
{
public:
    LogFileContext() = default;
    LogFileContext(std::string_view filePrefix) : filePrefix_(filePrefix) {}
    void setFileMaxMessageCount(std::size_t maxMsgCount) { maxMsgCount_ = maxMsgCount; }
    void setBatchWriteMessageCount(std::size_t count) {batchWriteCount_= count; }

    std::string_view getFilePrefix() const { return filePrefix_;  }
    std::size_t getFileMaxMessageCount() const { return maxMsgCount_; }
    std::size_t getBatchWriteMessageCount() const { return batchWriteCount_; }
private:
    std::string filePrefix_;
    std::size_t maxMsgCount_ = 1638400; //100MB
    std::size_t batchWriteCount_ = 16; //1KB
};

class LogFileWriter
{
public:
    LogFileWriter(const LogFileWriter& config) = delete;
    LogFileWriter& operator=(const LogFileWriter& config) = delete;
    LogFileWriter(const LogFileContext& config);
public:
    bool write(const LogMessage& msg);
    bool init();
private:
    std::string_view getNextFileName();
    std::string_view getFileName() { return std::string_view(fileName_.data()); }
    bool openFile();
    bool closeFile();
    bool write();

private:
    int fd_ = -1;
    int fileNumPos_ = 0;
    std::size_t totPossibleMsgs_;
    std::size_t nextSeqNo_ = 0;
    std::size_t bufferEndIndex_ = 0;
    std::vector<char> fileName_;
    LogFileContext config_;
    std::vector<LogMessage> buffer_;
    LogMessage fileEndMessage_;
};

class LogFileReader
{
    friend class LogMgr;
public:
    ~LogFileReader();
    std::pair<void*, MessageType> getNextMessage();
    bool openFile();
    void closeFile();
private:
    LogFileReader(std::string_view fileName, const LogFileContext &context, const std::vector<std::unique_ptr<MessageReader>> &fileReaderList);
    LogFileReader(const LogFileReader&) = delete;
    LogFileReader& operator=(const LogFileReader&) = delete;
private:
    std::vector<std::unique_ptr<MessageReader>> messageReaderList_;
    std::string fileName_;
    int fd_ = -1;
    bool readComplete_ = false;
    std::size_t readBufferSize_ = 0;
    std::size_t bufferOffSet_ = 0;
    std::vector<char> buffer_;
};

class LogMgr
{
public:
    LogMgr(std::size_t msgQueueSize);
    ~LogMgr();
    void addLogFileContext(LogFileType type, const LogFileContext &fileInfo) { fileInfoList_[static_cast<int>(type)] = fileInfo;}
    template<ValidLogMessage T> void registerLogMessage(MessageType type);
    void setLogDiectory(std::string_view logDirectory) { logDirectory_ = logDirectory; }
    template<ValidLogMessage T> bool logMessage(LogFileType fileType, const T& message);
    bool start();
    void stop();
    std::unique_ptr<LogFileReader> getLogFileReader(LogFileType type, std::string_view fileName);
private:
    bool init();
    void read();
private:
    std::vector<LogFileContext> fileInfoList_;
    std::vector<std::unique_ptr<LogFileWriter>> fileWriterList_;
    std::vector<std::unique_ptr<MessageReader>> messageReaderList_;
    std::string logDirectory_;
    std::size_t msgQueueSize_ = 320;
    MWMRNoOverWriteSlotRingBuffer<std::pair<LogFileType,LogMessage>> messageQueue_;
    std::jthread writerThread_;
    std::atomic<std::size_t> logCount_;               
    bool stop_ = false;
};

template<ValidLogMessage T> void  LogMgr::registerLogMessage(MessageType type)
{
     messageReaderList_[static_cast<int>(type)] = std::make_unique<MessageReaderImpl<T>>();
}

template<ValidLogMessage T> bool LogMgr::logMessage(LogFileType fileType, const T& message)
{
    auto slot = messageQueue_.getWriteSlot();

    if (!slot)
        return false;

    auto &data = slot->getData();
    data.first = fileType;
    convertToLogMessage(message, data.second);
    messageQueue_.setWriteComplete(slot);
    logCount_.fetch_add(1, std::memory_order_acq_rel);
    logCount_.notify_one();
    return true;
}

#endif