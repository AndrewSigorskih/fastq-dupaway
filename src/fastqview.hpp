#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>

class FastqView
{
public:
    FastqView() {};
    FastqView& operator=(FastqView&& other);
    const char* seq() const { return m_seq; }
    ssize_t seq_len() const { return m_seqlen; }
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
    char* m_seq = nullptr;
    char* m_field3 = nullptr;
    char* m_qual = nullptr;
    std::streamsize m_idlen = 0, m_seqlen = 0, m_field3len = 0, m_quallen = 0;
};

class FastqViewWithId : public FastqView
{ // TODO needs move assignment op
public:
    int cmp(const FastqViewWithId& other) const;
    friend bool operator>(const FastqViewWithId& left, const FastqViewWithId& right);
    friend bool operator<(const FastqViewWithId& left, const FastqViewWithId& right);
    std::streamsize read_new(char*, char*);
private:
    char* m_idtag = nullptr;
    std::streamsize m_idtag_len = 0;
};
