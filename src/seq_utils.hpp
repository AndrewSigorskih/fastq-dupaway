#pragma once
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace SeqUtils
{
    const long CHUNKSIZE = 17L;

    inline int _char2number(char);
    uint64_t pattern2number(const char*, size_t);
    void seq2hash(std::vector<uint64_t>&, const char*, ssize_t);

    int seqncmp(const char*, const char*, size_t);
    uint hammingDistance(const char*, const char*, ssize_t);
}
