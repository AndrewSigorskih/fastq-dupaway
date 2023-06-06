#include "fastqview.hpp"

FastqView::FastqView(const FastqView& other)
{
    this->m_id = other.m_id; this->m_idlen = other.m_idlen;
    this->m_seq = other.m_seq; this->m_seqlen = other.m_seqlen;
    this->m_field3 = other.m_field3; this->m_field3len = other.m_field3len; 
    this->m_qual = other.m_qual; this->m_quallen = other.m_quallen;
}

FastqView::FastqView(FastqView&& other)
{
    this->m_id = other.m_id; this->m_idlen = other.m_idlen; other.m_id=nullptr;
    this->m_seq = other.m_seq; this->m_seqlen = other.m_seqlen; other.m_seq = nullptr;
    this->m_field3 = other.m_field3; this->m_field3len = other.m_field3len; other.m_field3 = nullptr;
    this->m_qual = other.m_qual; this->m_quallen = other.m_quallen; other.m_qual = nullptr;
}

FastqView& FastqView::operator=(FastqView&& other)
{
    if (this != &other)
    {
        this->m_id = other.m_id; this->m_idlen = other.m_idlen; other.m_id=nullptr;
        this->m_seq = other.m_seq; this->m_seqlen = other.m_seqlen; other.m_seq = nullptr;
        this->m_field3 = other.m_field3; this->m_field3len = other.m_field3len; other.m_field3 = nullptr;
        this->m_qual = other.m_qual; this->m_quallen = other.m_quallen; other.m_qual = nullptr;
    }
    return *this;
}

bool FastqView::isEmpty() const
{
    return ((this->m_id == nullptr) || (this->m_seq == nullptr) || (this->m_field3 == nullptr) || (this->m_qual == nullptr));
}

int FastqView::cmp(const FastqView& other) const
{
    /*return strncmp(this->m_seq, other.m_seq,
                   std::min(this->m_seqlen, other.m_seqlen));*/
    //if ((this->isEmpty()) || (other.isEmpty()))
        //throw std::runtime_error("Trying to compare an empty Fastq object!");
    int res = strncmp(this->m_seq, other.m_seq,
                      std::min(this->m_seqlen, other.m_seqlen));
    if ((res==0) && (this->m_seqlen < other.m_seqlen))
        return -1;
    if ((res==0) && (this->m_seqlen > other.m_seqlen))
        return 1;
    return res;
}

bool operator>(const FastqView& left, const FastqView& right)
{
    return (left.cmp(right) > 0);
}

bool operator<(const FastqView& left, const FastqView& right)
{
    return (left.cmp(right) < 0);
}

std::ostream& operator<<(std::ostream& os, const FastqView& fq)
{
    // safety plug
    //if (fq.isEmpty())
        //throw std::runtime_error("Trying to write an empty Fastq object!");
    os.write(fq.m_id, fq.m_idlen+fq.m_seqlen+fq.m_field3len+fq.m_quallen);
    // TODO skip third field and only write "+\n"?
    return os;
}

std::streamsize FastqView::read_new(char* start, char* stop)
{   // try to map char* buffer to self, return -1 if buffer end is encountered prematurely
    if (start >= stop) { return -1; }
    if (*start != '@') { this->err_invalid_start(start); }
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
    if (ptr == stop) { m_qual = nullptr; return -1; }
    m_quallen = ptr - m_qual + 1;
    if (m_quallen != m_seqlen) { this->err_len_not_match(); }
    return m_seqlen + m_idlen + m_field3len + m_quallen;
}

void FastqView::err_invalid_start(char* ptr)
{
    std::cerr << "Invalid record start character: ";
    std::cerr << *ptr << std::endl;
    throw std::runtime_error("Fastq record should start with @ symbol!");
}

void FastqView::err_len_not_match()
{
    std::cerr << "Found sequence ";
    std::cerr.write(m_seq, m_seqlen-1);
    std::cerr <<" of length " << m_seqlen << " and quality string ";
    std::cerr.write(m_qual, m_quallen-1);
    std::cerr << " of length " << m_quallen << std::endl;
    throw std::runtime_error("Sequence and Quality fields of Fastq record should have the same length!"); 
}

int FastqViewWithId::cmp(const FastqViewWithId& other) const
{
    // TODO change this cmp as well
    return strncmp(this->m_idtag, other.m_idtag,
                   std::min(this->m_idtag_len, other.m_idtag_len));
}

bool operator>(const FastqViewWithId& left, const FastqViewWithId& right)
{
    return (left.cmp(right) > 0);
}

bool operator<(const FastqViewWithId& left, const FastqViewWithId& right)
{
    return (left.cmp(right) < 0);
}

std::streamsize FastqViewWithId::read_new(char* start, char* stop)
{
    std::streamsize size = FastqView::read_new(start, stop);
    char* ptr;
    ptr = std::find(m_id, m_id+m_idlen, '.');
    if (ptr == m_id+m_idlen)  // id in a form of "@XXXXX some_text"
        m_idtag = m_id + 1;  // dont use @ in comparison
    else                    // id in a form of "@XXXX.NNNNN some_text"
        m_idtag = ptr + 1; // we need NNNNN part
    ptr = std::find(m_idtag, m_id+m_idlen, ' ');
    m_idtag_len = ptr - m_idtag;
    return size;
}
