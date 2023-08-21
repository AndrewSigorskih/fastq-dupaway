#include "fastaview.hpp"

/*
----------------------------
>>> Base FastaView class <<<
----------------------------
*/

FastaView::FastaView(const FastaView& other)
{
    this->m_id = other.m_id; 
    this->m_idlen = other.m_idlen;
    this->m_seqlen = other.m_seqlen;
}

FastaView::FastaView(FastaView&& other)
{
    this->m_id = other.m_id;
    this->m_idlen = other.m_idlen;
    this->m_seqlen = other.m_seqlen;
    other.clear();
}

FastaView& FastaView::operator=(FastaView&& other)
{
    if (this != &other)
    {
        this->m_id = other.m_id;
        this->m_idlen = other.m_idlen;
        this->m_seqlen = other.m_seqlen;
        other.clear();
    }
    return *this;
}

void FastaView::clear()
{
    this->m_id = nullptr;
    this->m_idlen = 0L;
    this->m_seqlen = 0L;
}

bool FastaView::isEmpty() const
{
    return (this->m_id == nullptr);
}

int FastaView::cmp(const FastaView& other) const
{
    int res = strncmp(this->seq(), other.seq(),
                      std::min(this->m_seqlen, other.m_seqlen));
    if ((res==0) && (this->m_seqlen < other.m_seqlen))
        return -1;
    if ((res==0) && (this->m_seqlen > other.m_seqlen))
        return 1;
    return res;
}

bool operator>(const FastaView& left, const FastaView& right)
{
    return (left.cmp(right) > 0);
}

bool operator<(const FastaView& left, const FastaView& right)
{
    return (left.cmp(right) < 0);
}

std::ostream& operator<<(std::ostream& os, const FastaView& fq)
{
    os.write(fq.m_id, fq.m_idlen+fq.m_seqlen);
    return os;
}

std::streamsize FastaView::read_new(char* start, char* stop)
{   // try to map char* buffer to self, return -1 if buffer end is encountered prematurely
    if (start >= stop) { return -1; }
    if (*start != '>') { this->err_invalid_start(start); }
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
    return len_so_far;
}

void FastaView::err_invalid_start(char* ptr)
{
    std::cerr << "Invalid record start character: ";
    std::cerr << *ptr << std::endl;
    throw std::runtime_error("Fasta record should start with > symbol!");
}

/*
----------------------------------------------
            >>> FastaViewWithId <<<
----------------------------------------------
*/

FastaViewWithId::FastaViewWithId(const FastaViewWithId& other) : FastaView(other)
{
    this->m_idtag = other.m_idtag;
    this->m_idtag_len = other.m_idtag_len;
}

FastaViewWithId::FastaViewWithId(FastaViewWithId&& other) : FastaView(std::move(other))
{
    this->m_idtag = other.m_idtag;
    this->m_idtag_len = other.m_idtag_len;
}

FastaViewWithId& FastaViewWithId::operator=(FastaViewWithId&& other)
{
    FastaView::operator=(std::move(other));
    if (this != &other)
    {
        this->m_idtag = other.m_idtag;
        this->m_idtag_len = other.m_idtag_len;
    }
    return *this;
}

int FastaViewWithId::cmp(const FastaViewWithId& other) const
{
    int res = strncmp(this->m_idtag, other.m_idtag,
                      std::min(this->m_idtag_len, other.m_idtag_len));

    if ((res==0) && (this->m_idtag_len < other.m_idtag_len))
        return -1;
    if ((res==0) && (this->m_idtag_len > other.m_idtag_len))
        return 1;
    return res;
}

bool operator>(const FastaViewWithId& left, const FastaViewWithId& right)
{
    return (left.cmp(right) > 0);
}

bool operator<(const FastaViewWithId& left, const FastaViewWithId& right)
{
    return (left.cmp(right) < 0);
}

std::streamsize FastaViewWithId::read_new(char* start, char* stop)
{
    std::streamsize size = FastaView::read_new(start, stop);
    if (size <= 0)
        return -1;
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
