#include "fastqview.hpp"

// is it actually needed??
/*
FastqView& FastqView::operator=(FastqView&& other)
{
    if (this != &other)
    {
        this->m_id = other.m_id; this->m_idlen = other.m_idlen;
        this->m_seq = other.m_seq; this->m_seqlen = other.m_seqlen;
        this->m_field3 = other.m_field3; this->m_field3len = other.m_field3len;
        this->m_qual = other.m_qual; this->m_quallen = other.m_quallen;
    }
    return *this;
}
*/

bool operator>(const FastqView& left, const FastqView& right)
{
    return (strncmp(left.m_seq, right.m_seq, 
                    std::min(left.m_seqlen, right.m_seqlen)) > 0);
}

bool operator<(const FastqView& left, const FastqView& right)
{
    return (strncmp(left.m_seq, right.m_seq, 
                    std::min(left.m_seqlen, right.m_seqlen)) < 0);
}

std::ostream& operator<<(std::ostream& os, const FastqView& fq)
{
    os.write(fq.m_id, fq.m_idlen+fq.m_seqlen+fq.m_field3len+fq.m_quallen);
    // TODO skip third field and only write "+\n"?
    return os;
}

// TODO make errors more informative!
std::streamsize FastqView::read_new(char* start, char* stop)
{   // try to map char* buffer to self, return -1 if buffer end is encountered prematurely
    if (start >= stop) { return -1; }
    if (*start != '@') { throw std::runtime_error("Fastq record should start with @ symbol!"); }
    char* ptr;
    // search ID
    m_id = start;
    ptr = std::find(start, stop, '\n');
    if (ptr == stop) { return -1; }
    m_idlen = ptr - start + 1;
    // search SEQ
    m_seq = ++ptr;
    ptr = std::find(m_seq, stop, '\n');
    if (ptr == stop) { return -1; }
    m_seqlen = ptr - m_seq + 1;
    // junk third string
    m_field3 = ++ptr;
    ptr = std::find(m_field3, stop, '\n');
    if (ptr == stop) { return -1; }
    m_field3len = ptr - m_field3 + 1;
    // quality
    m_qual = ++ptr;
    ptr = std::find(m_qual, stop, '\n');
    if (ptr == stop) { return -1; }
    m_quallen = ptr - m_qual + 1;
    if (m_quallen != m_seqlen)
    {
        throw std::runtime_error("Sequence and Quality fields have different lengths!"); 
    }
    return m_seqlen + m_idlen + m_field3len + m_quallen;
}