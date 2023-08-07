#include "hash_dup_remover.hpp"
#include "seq_utils.hpp"

setRecord::setRecord(const char* seq, ssize_t len)
{
    this->m_seq_len = len;
    long num_chunks = (len / SeqUtils::CHUNKSIZE) + 1;
    this->m_hash.reserve(num_chunks);
    for (long i = 0; i < num_chunks; ++i)
    {
        this->m_hash.push_back(
            SeqUtils::pattern2number(
                seq+(i*SeqUtils::CHUNKSIZE), 
                std::min(SeqUtils::CHUNKSIZE, len-(i*SeqUtils::CHUNKSIZE))
            )
        );
    }
}

bool setRecord::operator==(const setRecord& other) const
{
    if (this->m_seq_len != other.m_seq_len) return false;
    for (size_t i = 0; i < this->m_hash.size(); ++i)
        if (this->m_hash[i] != other.m_hash[i])
            return false;
    return true;
}

setRecordPair::setRecordPair(const setRecord& first, const setRecord& second)
{
    this->left = first;
    this->right = second;
}

bool setRecordPair::operator==(const setRecordPair& other) const
{
    if (this->left == other.left)
        return (this->right == other.right);
    return false;
}
