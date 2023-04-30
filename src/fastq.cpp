#include "fastq.hpp"

FastQEntry::FastQEntry(FastQEntry&& other)
{
    m_id = std::move(other.m_id);
    m_seq = std::move(other.m_seq);
    m_field3 = std::move(other.m_field3);
    m_qual = std::move(other.m_qual);
}

FastQEntry& FastQEntry::operator=(FastQEntry&& other)
{
    if (this != &other)
    {
        m_id = std::move(other.m_id);
        m_seq = std::move(other.m_seq);
        m_field3 = std::move(other.m_field3);
        m_qual = std::move(other.m_qual);
    }
    return *this;
}

bool operator>(const FastQEntry& left, const FastQEntry& right)
{
    return (left.m_seq > right.m_seq);
}

bool operator<(const FastQEntry& left, const FastQEntry& right)
{
    return (left.m_seq < right.m_seq);
}

std::istream& operator>>(std::istream& is, FastQEntry& fq)
{
    getline(is, fq.m_id);
    getline(is, fq.m_seq);
    getline(is, fq.m_field3);
    getline(is, fq.m_qual);
    return is;
}

std::ostream& operator<<(std::ostream& os, const FastQEntry& fq)
{
    os << fq.m_id << '\n' << fq.m_seq << '\n' \
        << fq.m_field3 << '\n' << fq.m_qual;
    return os;
}
