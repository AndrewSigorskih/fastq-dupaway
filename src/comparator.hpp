#pragma once
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "seq_utils.hpp"

const ssize_t STARTING_SEQ_SIZE = 150L;

enum ComparatorType
{
    CT_NONE,
    CT_TIGHT,
    CT_LOOSE,
    CT_HAMMING
};

class BaseComparator
{
public:
    BaseComparator(bool);
    virtual ~BaseComparator();
    void set_seq(const char*, ssize_t);
    void set_seq(const char*, ssize_t, const char*, ssize_t);
    virtual bool compare(const char*, ssize_t) = 0;
    virtual bool compare(const char*, ssize_t, const char*, ssize_t) = 0;
protected:
    char* m_buf_1 = nullptr;
    char* m_buf_2 = nullptr;
    ssize_t m_len_1 = 0, m_len_2 = 0;
    ssize_t m_capacity_1 = 0, m_capacity_2 = 0;
};

class TightComparator : public BaseComparator
{
public:
    using BaseComparator::BaseComparator; // inherit constructor(s)
    bool compare(const char*, ssize_t);
    bool compare(const char*, ssize_t, const char*, ssize_t);
};

class LooseComparator : public BaseComparator
{
public:
    using BaseComparator::BaseComparator;
    bool compare(const char*, ssize_t);
    bool compare(const char*, ssize_t, const char*, ssize_t);
};

class HammingComparator : public BaseComparator
{
public:
    HammingComparator(bool, uint);
    bool compare(const char*, ssize_t);
    bool compare(const char*, ssize_t, const char*, ssize_t);
private:
    uint m_dist;
};

// comparator factory
BaseComparator* makeComparator(ComparatorType, bool, uint);
