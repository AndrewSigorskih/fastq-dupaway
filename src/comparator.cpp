#include "comparator.hpp"

BaseComparator::BaseComparator(bool paired)
{
    m_capacity_1 = STARTING_SEQ_SIZE;
    m_buf_1 = (char*)malloc(sizeof(char)*m_capacity_1);
    if (paired)
    {
        m_capacity_2 = STARTING_SEQ_SIZE;
        m_buf_2 = (char*)malloc(sizeof(char)*m_capacity_2);
    }
}

BaseComparator::~BaseComparator()
{
    free(m_buf_1);
    if (m_buf_2)
        free(m_buf_2);
}

void BaseComparator::set_seq(const char* seq, ssize_t len)
{
    if (len > m_capacity_1)
    {
        m_buf_1 = (char*)realloc(m_buf_1, len);
        m_capacity_1 = len;
    }
    m_len_1 = len;
    memcpy(m_buf_1, seq, len);
}

void BaseComparator::set_seq(const char* seq_1, ssize_t len_1,
                             const char* seq_2, ssize_t len_2)
{
    this->set_seq(seq_1, len_1);
    if (len_2 > m_capacity_2)
    {
        m_buf_2 = (char*)realloc(m_buf_2, len_2);
        m_capacity_2 = len_2;
    }
    m_len_2 = len_2;
    memcpy(m_buf_2, seq_2, len_2);
}

bool TightComparator::compare(const char* seq, ssize_t len)
{  // similar to fastuniq's "compare_tight"
    if (len != m_len_1) return false;
    return (strncmp(seq, m_buf_1, len) == 0);
}

bool TightComparator::compare(const char* seq_1, ssize_t len_1,
                              const char* seq_2, ssize_t len_2)
{  // similar to fastuniq's "compare_tight"
    bool first_cmp = this->compare(seq_1, len_1);
    if (!first_cmp) return false;
    if (len_2 != m_len_2) return false;
    return (strncmp(seq_2, m_buf_2, len_2) == 0);
}

bool LooseComparator::compare(const char* seq, ssize_t len)
{  // similar to fastuniq's "compare_loose"
    return (strncmp(seq, m_buf_1, std::min(len-1, m_len_1-1)) == 0);
}

bool LooseComparator::compare(const char* seq_1, ssize_t len_1,
                              const char* seq_2, ssize_t len_2)
{  // similar to fastuniq's "compare_loose"
    bool first_cmp = this->compare(seq_1, len_1);
    if (!first_cmp) return false;
    bool second_cmp = (strncmp(seq_2, m_buf_2, std::min(len_2-1, m_len_2-1)) == 0);
    if (!second_cmp) return false;
    // only return true if both overlaps are same-sided
    return ((m_len_1 <= len_1) && (m_len_2 <= len_2)) || ((m_len_1 > len_1) && (m_len_2 > len_2));
}

HammingComparator::HammingComparator(bool paired, uint dist) : BaseComparator(paired), m_dist(dist) { }

bool HammingComparator::compare(const char* seq, ssize_t len)
{
    if (len != m_len_1) return false;
    return (SeqUtils::hammingDistance(m_buf_1, seq, len) <= m_dist);
}

bool HammingComparator::compare(const char* seq_1, ssize_t len_1,
                                const char* seq_2, ssize_t len_2)
{
    bool first_cmp = this->compare(seq_1, len_1);
    if (!first_cmp) return false;
    if (len_2 != m_len_2) return false;
    return (SeqUtils::hammingDistance(m_buf_2, seq_2, len_2) <= m_dist);
}

BaseComparator* makeComparator(ComparatorType ctype,
                               bool paired,
                               uint hammdist)
{
    switch (ctype)
    {
        case ComparatorType::CT_NONE:
            return nullptr;
        case ComparatorType::CT_TIGHT:
            return new TightComparator(paired);
        case ComparatorType::CT_LOOSE:
            return new LooseComparator(paired);
        case ComparatorType::CT_HAMMING:
            return new HammingComparator(paired, hammdist);
        default:
            throw std::runtime_error("Unknown ComparatorType requested!");
    }
}