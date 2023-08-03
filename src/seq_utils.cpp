#include "seq_utils.hpp"

inline int SeqUtils::_char2number(char c)
{
    switch (c)
    {
        case 'A':
        case 'N':
            return 0;
        case 'C':
            return 1;
        case 'G':
            return 2;
        case 'T':
            return 3;
        default:
            std::cerr << "Error: unknown character in DNA sequence: " << c << '\n';
            throw std::runtime_error("Supported sequences character set: {A, N, C, G, T}!");
    }
}

uint64_t SeqUtils::pattern2number(const char* seq, size_t len)
{
    uint64_t result = 0UL;

    for (size_t i = 0; i < len; ++i)
    {
        result = 4 * result + SeqUtils::_char2number(seq[i]);
    }

    return result;
}

int SeqUtils::seqncmp(const char* s1, const char* s2, size_t len)
{
    while ((len > 0) && (SeqUtils::_char2number(*s1) == SeqUtils::_char2number(*s2)))
    {
        ++s1;
        ++s2;
        --len;
    }
    if (len == 0)
        return 0;
    return SeqUtils::_char2number(*s1) - SeqUtils::_char2number(*s2);
}
