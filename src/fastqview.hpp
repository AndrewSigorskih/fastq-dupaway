#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>

#include "seq_utils.hpp"

class FastqView
{
public:
    FastqView() {};
    FastqView(const FastqView&);
    FastqView(FastqView&&);
    FastqView& operator=(FastqView&& other);
    void clear();
    bool isEmpty() const;
    ssize_t size() const { return m_idlen+m_seqlen+m_field3len+m_quallen; }
    ssize_t seq_len() const { return m_seqlen; }
    const char* seq() const { return m_id + m_idlen; }
    int cmp(const FastqView& other) const;
    friend bool operator>(const FastqView& left, const FastqView& right);
    friend bool operator<(const FastqView& left, const FastqView& right);
    friend std::ostream& operator<<(std::ostream& os, const FastqView& fq);
    std::streamsize read_new(char*, char*);
private:
    void err_invalid_start(char*);
    void err_len_not_match();
protected:
    char* m_id = nullptr;
    ssize_t m_idlen = 0L, m_seqlen = 0L, m_field3len = 0L, m_quallen = 0L;
};

class FastqViewWithPreHash : public FastqView
{
public:
    FastqViewWithPreHash() {};
    FastqViewWithPreHash(const FastqViewWithPreHash&);
    FastqViewWithPreHash(FastqViewWithPreHash&&);
    FastqViewWithPreHash& operator=(FastqViewWithPreHash&& other);
    int cmp(const FastqViewWithPreHash& other) const;
    std::streamsize read_new(char*, char*);
private:
    uint64_t m_hash = 0UL;
};

class FastqViewWithId : public FastqView
{ // TODO needs copy and move semantics added!
public:
    int cmp(const FastqViewWithId& other) const;
    std::streamsize read_new(char*, char*);
private:
    char* m_idtag = nullptr;
    std::streamsize m_idtag_len = 0;
};
