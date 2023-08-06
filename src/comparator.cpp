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
    if (!m_buf_2) 
    { throw std::runtime_error("Comparator was initialized in single-sequence mode, however paired-sequence comparison was requested."); }
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
    if (!m_buf_2) 
    { throw std::runtime_error("Comparator was initialized in single-sequence mode, however paired-sequence comparison was requested."); }
    bool first_cmp = this->compare(seq_1, len_1);
    if (!first_cmp) return false;
    return (strncmp(seq_2, m_buf_2, std::min(len_2-1, m_len_2-1)) == 0);
}

BaseComparator* makeComparator(ComparatorType ctype,
                               bool paired)
{
    switch (ctype)
    {
        case ComparatorType::CT_TIGHT:
            return new TightComparator(paired);
        case ComparatorType::CT_LOOSE:
            return new LooseComparator(paired);
        default:
            throw std::runtime_error("Unknown ComparatorType requested!");
    }
}