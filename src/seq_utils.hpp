#pragma once
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace SeqUtils
{
    const long CHUNKSIZE = 17L;
    inline int _char2number(char);
    uint64_t pattern2number(const char*, size_t);
    int seqncmp(const char*, const char*, size_t);
}
