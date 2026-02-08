#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <cstdint>

inline std::size_t getNextPowerOfTwo(std::size_t number)
{
    if (number == 0)
        return 1;

    --number;
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    number |= number >> 32;
    return number + 1;
}

#endif