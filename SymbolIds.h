#ifndef SYMBOL_IDS_H
#define SYMBOL_IDS_H

#include <string>
#include <unordered_map>


class SymbolIds
{
public:
    SymbolIds(const std::string &fileName) { ready_ = populateSymbolIds(fileName); }
    inline bool isReady() { return ready_; }
    inline int getId(const std::string &label) { auto iter = cache_.find(label);
                                              if (iter != cache_.end()) {
                                                    return iter->second;
                                              }
                                              return -1;
                                            }
    inline auto getNumSymbols() const { return cache_.size(); }
private:
    bool populateSymbolIds(const std::string &fileName);

private:
    std::unordered_map<std::string,int> cache_;
    bool ready_ = false;
};

#endif