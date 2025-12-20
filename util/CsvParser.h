#ifndef CVS_PARSER_H
#define CVS_PARSER_H

#include <string>

class CsvParser
{
public:
    CsvParser(const std::string& fileName);
    ~CsvParser();
    bool getNextToken(char*& ptr, std::size_t &length);
    bool isReady() { return filePtr_ != nullptr; }
private:
    int fd_ = -1;
    char* filePtr_ = nullptr;
    std::size_t currentIndex_ = 0;
    std::size_t fileSize_ = 0;
};

#endif