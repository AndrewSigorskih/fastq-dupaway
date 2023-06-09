#pragma once
#include <stdlib.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

const ssize_t STARTING_SEQ_SIZE = 150L;

class Comparator
{
public:
    Comparator(bool);
    ~Comparator();
    void set_seq(const char*, ssize_t);
    void set_seq(const char*, ssize_t, const char*, ssize_t);
    bool compare(const char*, ssize_t);
    bool compare(const char*, ssize_t, const char*, ssize_t);
private:
    char* m_buf_1 = nullptr;
    char* m_buf_2 = nullptr;
    ssize_t m_len_1 = 0, m_len_2 = 0;
    ssize_t m_capacity_1 = 0, m_capacity_2 = 0;
};