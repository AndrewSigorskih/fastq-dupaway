#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>

class FastaView
{
public:
    FastaView() {}
    
private:
    void err_invalid_start(char*);
protected:
    char* m_id = nullptr;
    ssize_t m_idlen = 0L, m_seqlen = 0L;
};

class FastaViewWithId : public FastaView
{
public:

private:
};