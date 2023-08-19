#include "hash_dup_remover.hpp"
#include "seq_utils.hpp"

setRecord::setRecord(const char* seq, ssize_t len)
{
    this->m_seq_len = len;
    SeqUtils::seq2hash(this->m_hash, seq, len);
}

bool setRecord::operator==(const setRecord& other) const
{
    if (this->m_seq_len != other.m_seq_len) return false;
    return (this->m_hash == other.m_hash);
}

setRecordPair::setRecordPair(const char* l_seq, ssize_t l_len, 
                             const char* r_seq, ssize_t r_len)
{
    this->m_l_len = l_len;
    this->m_r_len = r_len;

    SeqUtils::seq2hash(this->m_l_hash, l_seq, l_len);
    SeqUtils::seq2hash(this->m_r_hash, r_seq, r_len);
}

bool setRecordPair::operator==(const setRecordPair& other) const
{
    if ((this->m_l_len != other.m_l_len) || (this->m_r_len != other.m_r_len))
        return false;
    if (this->m_l_hash != other.m_l_hash)
        return false;
    return (this->m_r_hash == other.m_r_hash);
}
