#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>

class FastaView
{
public:
    FastaView() {}
    FastaView(const FastaView&);
    FastaView(FastaView&&);
    FastaView& operator=(FastaView&&);
    void clear();
    bool isEmpty() const;
    inline ssize_t size() const { return m_idlen + m_seqlen; }
    inline ssize_t seq_len() const { return m_seqlen; }
    inline const char* seq() const { return m_id + m_idlen; }
    int cmp(const FastaView&) const;
    friend bool operator>(const FastaView& left, const FastaView& right);
    friend bool operator<(const FastaView& left, const FastaView& right);
    friend std::ostream& operator<<(std::ostream& os, const FastaView& fq);
    std::streamsize read_new(char*, char*);
private:
    void err_invalid_start(char*);
protected:
    char* m_id = nullptr;
    ssize_t m_idlen = 0L, m_seqlen = 0L;
};

class FastaViewWithId : public FastaView
{
public:
    FastaViewWithId() {}
    FastaViewWithId(const FastaViewWithId&);
    FastaViewWithId(FastaViewWithId&&);
    FastaViewWithId& operator=(FastaViewWithId&&);
    int cmp(const FastaViewWithId& other) const;
    friend bool operator>(const FastaViewWithId& left, const FastaViewWithId& right);
    friend bool operator<(const FastaViewWithId& left, const FastaViewWithId& right);
    std::streamsize read_new(char*, char*);
private:
    char* m_idtag = nullptr;
    std::streamsize m_idtag_len = 0;
};