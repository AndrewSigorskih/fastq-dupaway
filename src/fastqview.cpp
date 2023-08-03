#include "fastqview.hpp"

/*
----------------------------
>>> Base FastqView class <<<
----------------------------
*/

FastqView::FastqView(const FastqView& other)
{
    this->m_id = other.m_id; 
    this->m_idlen = other.m_idlen;
    this->m_seqlen = other.m_seqlen;
    this->m_field3len = other.m_field3len; 
    this->m_quallen = other.m_quallen;
}

FastqView::FastqView(FastqView&& other)
{
    this->m_id = other.m_id;
    this->m_idlen = other.m_idlen;
    this->m_seqlen = other.m_seqlen;
    this->m_field3len = other.m_field3len;
    this->m_quallen = other.m_quallen;
    other.clear();
}

FastqView& FastqView::operator=(FastqView&& other)
{
    if (this != &other)
    {
        this->m_id = other.m_id;
        this->m_idlen = other.m_idlen;
        this->m_seqlen = other.m_seqlen;
        this->m_field3len = other.m_field3len;
        this->m_quallen = other.m_quallen;
        other.clear();
    }
    return *this;
}

void FastqView::clear()
{
    this->m_id = nullptr;
    this->m_idlen = 0L;
    this->m_seqlen = 0L;
    this->m_field3len = 0L;
    this->m_quallen = 0L;
}

bool FastqView::isEmpty() const
{
    return ((this->m_id == nullptr));
}

int FastqView::cmp(const FastqView& other) const
{
    //if ((this->isEmpty()) || (other.isEmpty()))
        //throw std::runtime_error("Trying to compare an empty Fastq object!");
    int res = strncmp(this->seq(), other.seq(),
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
    if (ptr == stop) { this->clear(); return -1; }
    m_idlen = ptr - start + 1;
    ssize_t len_so_far = m_idlen;
    // search SEQ
    ++ptr;
    ptr = std::find(ptr, stop, '\n');
    if (ptr == stop) { this->clear(); return -1; }
    m_seqlen = ptr - (start+len_so_far) + 1;
    len_so_far += m_seqlen;
    // junk third string
    ++ptr;
    ptr = std::find(ptr, stop, '\n');
    if (ptr == stop) { this->clear(); return -1; }
    m_field3len = ptr - (start+len_so_far) + 1;
    len_so_far += m_field3len;
    // quality
    ++ptr;
    ptr = std::find(ptr, stop, '\n');
    if (ptr == stop) { this->clear(); return -1; }
    m_quallen = ptr - (start+len_so_far) + 1;
    if (m_quallen != m_seqlen) { this->err_len_not_match(); }
    return len_so_far + m_quallen;
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
    char* seq = this->m_id + this->m_idlen;
    std::cerr.write(seq, m_seqlen-1);
    std::cerr <<" of length " << m_seqlen << " and quality string ";
    char* qual = this->m_id + this->m_idlen + this->m_seqlen + this->m_field3len;
    std::cerr.write(qual, m_quallen-1);
    std::cerr << " of length " << m_quallen << std::endl;
    throw std::runtime_error("Sequence and Quality fields of Fastq record should have the same length!"); 
}

/*
---------------------------------------------------
            >>> FastqViewWithPreHash <<<
---------------------------------------------------
*/
FastqViewWithPreHash::FastqViewWithPreHash(const FastqViewWithPreHash& other) : FastqView(std::move(other))
{
    this->m_hash = other.m_hash;
}

FastqViewWithPreHash::FastqViewWithPreHash(FastqViewWithPreHash&& other) : FastqView(std::move(other))
{
    this->m_hash = other.m_hash;
    other.m_hash = 0L;
}

FastqViewWithPreHash& FastqViewWithPreHash::operator=(FastqViewWithPreHash&& other)
{
    FastqView::operator=(std::move(other));
    if (this != &other)
    {
        this->m_hash = other.m_hash;
        other.m_hash = 0L;
    }
    return *this;
}

int FastqViewWithPreHash::cmp(const FastqViewWithPreHash& other) const
{
    if (this->m_hash != other.m_hash)
        return this->m_hash < other.m_hash ? -1 : 1;
    // hashes are equal, needs checking
    if ((this->m_seqlen > params::PREFIX_LEN) || (other.m_seqlen > params::PREFIX_LEN))
    {
        int res = SeqUtils::seqncmp(this->seq(),
                                    other.seq(),
                                    std::min(this->m_seqlen-1, other.m_seqlen-1));
        if ((res == 0) && (this->m_seqlen < other.m_seqlen))
            return -1;
        if ((res == 0) && (this->m_seqlen > other.m_seqlen))
            return 1;
        return res;
    }
    // sequences are both of size PREFIX_LEN and equal
    return 0;
}

bool operator>(const FastqViewWithPreHash& left, const FastqViewWithPreHash& right)
{
    return (left.cmp(right) > 0);
}

bool operator<(const FastqViewWithPreHash& left, const FastqViewWithPreHash& right)
{
    return (left.cmp(right) < 0);
}

std::streamsize FastqViewWithPreHash::read_new(char* start, char* stop)
{
    std::streamsize size = FastqView::read_new(start, stop);
    if (size > 0)
        this->m_hash = SeqUtils::pattern2number(this->seq(), params::PREFIX_LEN);
    return size;
}

/*
----------------------------------------------
            >>> FastqViewWithId <<<
----------------------------------------------
*/

int FastqViewWithId::cmp(const FastqViewWithId& other) const
{
    // TODO change this cmp as well
    return strncmp(this->m_idtag, other.m_idtag,
                   std::min(this->m_idtag_len, other.m_idtag_len));
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
