#include "SymbolIds.h"
#include "CsvParser.h"
#include <charconv>


bool SymbolIds::populateSymbolIds(const std::string &fileName)
{
    CsvParser parser(fileName);

    if (!parser.isReady())
        return false;

    char *ptr;
    std::size_t length;
    int id;

    while(parser.getNextToken(ptr, length)) {
        std::string label = std::string(ptr,length);
        parser.getNextToken(ptr, length);
        std::from_chars(ptr, ptr+length, id);
        cache_.emplace(label, id);
    }

    return true;

}
