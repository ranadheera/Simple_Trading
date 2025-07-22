#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "CsvParser.h"

CsvParser::CsvParser(const std::string& fileName)
{
    fd_ = open(fileName.c_str(), O_RDONLY);

    if (fd_ == -1) {
        return;
    }

    struct stat st;

    if (fstat(fd_, &st) == -1) {
        return;
    }

    filePtr_ = (char*) mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd_, 0);

    if (filePtr_ == MAP_FAILED) {
        return;
    }

    currentIndex_ = 0;
    fileSize_ = st.st_size;

}

CsvParser::~CsvParser() {
    if (filePtr_)
        munmap(filePtr_, fileSize_);
    if (fd_ != -1)
        close(fd_);
}


bool CsvParser::getNextToken(char*& ptr, std::size_t &length)
{
    length = 0;
    ptr = filePtr_+ currentIndex_;

    while (currentIndex_ < fileSize_) {
        if ((filePtr_[currentIndex_] == ',') || (filePtr_[currentIndex_] == '\n') ) {
            ++currentIndex_;
            break;
        }
        ++length;
        ++currentIndex_;
    }
    return length > 0;
}

