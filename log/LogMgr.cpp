#include <filesystem>
#include "LogMgr.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

bool writeFull(int fd, char *buffer, size_t size) {
    auto totalWrittenBytes = 0;

    while (totalWrittenBytes < size) {
        auto written =  ::write(fd + totalWrittenBytes, buffer , size - totalWrittenBytes);

        if (written < 0 && (written != EINTR)) {
            return false;
        }
        
        totalWrittenBytes += written;
    }
    return true;
}

LogMgr::LogMgr(std::size_t msgQueueSize) : fileInfoList_(static_cast<int>(LogFileType::COUNT)), 
                   fileWriterList_(static_cast<int>(LogFileType::COUNT)),
                   messageReaderList_(static_cast<int>(MessageType::COUNT)),
                   msgQueueSize_(msgQueueSize),
                   messageQueue_(msgQueueSize) {
                   logCount_.store(0);
                   }

bool LogMgr::init()
{
   if (!logDirectory_.empty()) {
        std::filesystem::path dir(logDirectory_);
        bool dirExit = std::filesystem::exists(dir) && std::filesystem::is_directory(dir);

        if (!dirExit) {
            std::cout << "Log directory " << logDirectory_ << " does not exist.";

            try {
                if (!std::filesystem::create_directories(logDirectory_)) {
                    std::cout << "Could not create log directory." << std::endl ;
                    return false;
                } else {
                    std::cout << "Log directory created" << std::endl;
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cout << e.what() << std::endl;
                return false;        
            }
        } else {
            std::cout << "Log directory " << logDirectory_ << " exist. Files may be overwritten.";
        } 
    }
    
    for (std::size_t i = 0; i < fileInfoList_.size(); ++i) {

        if (fileInfoList_[i].getFilePrefix().empty()) {

            return false;
        }

        fileWriterList_[i].reset(new LogFileWriter(fileInfoList_[i]));
        
        if (!fileWriterList_[i]->init()) {
            return false;
        }
    }

    for (std::size_t index = 0; index < messageReaderList_.size(); ++index) {

        if (!messageReaderList_[index]) {
            std::cout << "MessageReader is not registered for type " << index << std::endl;
            return false;
        }
    }

    return true;
}

bool LogMgr::start()
{
    if (!init())
        return false;

    writerThread_ = std::jthread(&LogMgr::read, this);
    std::cout << "Log manager started" << std::endl;
    return true;
}

void LogMgr::read()
{
    
    while (!stop_) {
        auto logCount = logCount_.load();
        auto *slot = messageQueue_.getReadSlot();

        while (!slot) {
            logCount_.wait(logCount);
            slot = messageQueue_.getReadSlot();
        }

        auto data = slot->getData();
        messageQueue_.setReadComplete(slot);
        auto writerIndex = static_cast<std::size_t>(data.first);
        auto &writer = fileWriterList_[writerIndex];

        if (!writer->write(data.second)) {
            return;
        }
    }
    
}

void LogMgr::stop()
{
    stop_ = true;

    if (writerThread_.joinable())
        writerThread_.join();

    std::cout << "Log manager stopped" << std::endl;
    return;
}

std::unique_ptr<LogFileReader> LogMgr::getLogFileReader(LogFileType type, std::string_view fileName)
{
    auto& fileContext = fileInfoList_[static_cast<std::size_t>(type)];
    auto reader = std::unique_ptr<LogFileReader>(new LogFileReader(fileName, fileContext, messageReaderList_));

    if (!reader->openFile())
        return nullptr;

    return reader;

}

LogMgr::~LogMgr()
{

}

LogFileWriter::LogFileWriter(const LogFileContext& config) : totPossibleMsgs_(config.getFileMaxMessageCount()),
                                                                 fileName_(config.getFilePrefix().size()+25), config_(config),
                                                                 buffer_(config.getBatchWriteMessageCount())
{

    
}

bool LogFileWriter::init()
{
    fileNumPos_ = snprintf(fileName_.data(), fileName_.size(),
                            "%s_", config_.getFilePrefix().data());
    if (fileNumPos_ < 0) {
        std::cout << "Constructing LogFileWriter for " << config_.getFilePrefix() << " failed" << std::endl;
        return false;
    }

    auto fileEndMessage = FileEnd1::make();
    convertToLogMessage(fileEndMessage, fileEndMessage_);
    return openFile();
}

std::string_view LogFileWriter::getNextFileName()
{
    char *fileNumberPos = fileName_.data() + fileNumPos_;
    auto written = snprintf(fileNumberPos , fileName_.size() - fileNumPos_,
                            "%zu.bin", ++nextSeqNo_);
    
    if (written < 0) {
        std::cout << "Generating next file name for " << config_.getFilePrefix().data() << " failed." << std::endl;
        return std::string_view();
    }

    return std::string_view(fileName_.data());
    
}

bool LogFileWriter::openFile()
{
    std::string_view nextLogFIle = getNextFileName();

    if (nextLogFIle.empty()) {
        return false;
    }

    fd_ = ::open(nextLogFIle.data(), O_RDWR| O_CREAT | O_TRUNC, 0644);

    if (fd_ < 0) {
        std::cout << "Log file " << nextLogFIle << " can not be opened." << std::endl;
        return false;
    }

    auto status = fallocate(fd_, 0 , 0, config_.getFileMaxMessageCount() * LogMessage::dataSize_);

    if (status < 0) {
        std::cout << "Space allocation for file " << nextLogFIle << " failed." << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

bool LogFileWriter::write(const LogMessage& msg)
{
    auto currentMsgCount = bufferEndIndex_;

    if ((bufferEndIndex_ == buffer_.size()) || (bufferEndIndex_ + 1 == totPossibleMsgs_)) {

        auto written = writeFull(fd_ , reinterpret_cast<char*>(buffer_.data()), (bufferEndIndex_ * LogMessage::dataSize_));
        
        if (!written) {
            std::cout << "Writing to log file " << getFileName() << " Failed";
            return false;
        }

        totPossibleMsgs_ -= bufferEndIndex_;
        bufferEndIndex_ = 0;

        if (totPossibleMsgs_ <= 2) {
            if (!closeFile())
                return false;

            if (!openFile())
                return false;

            totPossibleMsgs_ = config_.getFileMaxMessageCount();
        }

    }
    buffer_[bufferEndIndex_++] = msg;
    return true;
}


bool LogFileWriter::closeFile()
{
    if (fd_ < 0)
        return false;

    auto written = ::write(fd_, fileEndMessage_.data_, LogMessage::dataSize_);

    if (written < 0) {
        std::cout << "Writing end message to log file " << getFileName() << " Failed";
        return false;
    }

    ::close(fd_);
    totPossibleMsgs_ = 0;
    fd_ = -1;
    return true;
}

LogFileReader::LogFileReader(std::string_view fileName, const LogFileContext &context, const std::vector<std::unique_ptr<MessageReader>> &messageReaderList): 
messageReaderList_(messageReaderList.size()), fileName_(fileName), buffer_(context.getBatchWriteMessageCount() * LogMessage::dataSize_)
{
    for (std::size_t i = 0; i < messageReaderList.size(); ++i) {
        messageReaderList_[i] =  messageReaderList[i]->clone();
    }
}

bool LogFileReader::openFile()
{
    fd_ = ::open(fileName_.c_str(), O_RDONLY);

    if (fd_ < 0)
        std::cout << "Could not open log file " << fileName_ << std::endl;

    posix_fadvise(fd_, 0, 0, POSIX_FADV_SEQUENTIAL);

    return true;

}

std::pair<void*,MessageType> LogFileReader::getNextMessage()
{
    if (readComplete_)
        return std::make_pair(nullptr, MessageType::FILE_END_1);

    if (readBufferSize_ == 0) {
        auto readCount = ::read(fd_, buffer_.data(), buffer_.size());

        if (readCount < 0)
            return std::make_pair(nullptr, MessageType::FILE_END_1);;

        readBufferSize_ = readCount;
        bufferOffSet_ = 0;
    }

    MessageType messageType;
    memcpy(&messageType, buffer_.data() +  bufferOffSet_, sizeof(messageType));

    if (messageType == MessageType::FILE_END_1) {
        readComplete_ = true;
        return std::make_pair(nullptr, MessageType::FILE_END_1);;
    }

    auto &reader = messageReaderList_[static_cast<std::size_t>(messageType)];
    auto message = reader->getMesssage(buffer_.data() +  bufferOffSet_);
    bufferOffSet_ += LogMessage::dataSize_;
    readBufferSize_ -= LogMessage::dataSize_;
    return std::make_pair(message,messageType);
}

void LogFileReader::closeFile()
{
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }

}

LogFileReader::~LogFileReader()
{

}