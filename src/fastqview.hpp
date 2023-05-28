#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>

class FastqView
{
public:
    FastqView() {};
    //FastqView& operator=(FastqView&& other);
    int cmp(const FastqView& other) const;
    friend bool operator>(const FastqView& left, const FastqView& right);
    friend bool operator<(const FastqView& left, const FastqView& right);
    friend std::ostream& operator<<(std::ostream& os, const FastqView& fq);
    std::streamsize read_new(char*, char*);
private:
    char* m_id = nullptr;
    char* m_seq = nullptr;
    char* m_field3 = nullptr;
    char* m_qual = nullptr;
    std::streamsize m_idlen = 0, m_seqlen = 0, m_field3len = 0, m_quallen = 0;
};

class FastqViewWithId : FastqView
{
    // TODO
    // extract Illumina-style ID and store it in extra fields
    // new comparison operators: compare by ID instead of seq
};
