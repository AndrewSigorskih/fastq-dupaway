#pragma once
#include <iostream>

class BufferSizeExceeded : public std::exception 
{
public:
    const char * what () const throw() 
    { return "Not enough memory to read a single object!"; }
};

template <class T>
class BufferedInput
{
public:
    BufferedInput(std::streamsize);
    ~BufferedInput();
    bool eof() { return this->m_infile->eof() && this->m_block_end; }
    bool block_end() { return this->m_block_end; }
    void set_file(std::ifstream* ist) { m_infile = ist; this->refresh();}
    void unset_file();
    void refresh();
    T next();

private:
    std::streamsize m_maxsize, m_cursize, m_curpos = 0;
    std::ifstream* m_infile = nullptr;
    char* m_buffer = nullptr;
    bool m_block_end = false;
    T m_curobj;
};

template <class T>
BufferedInput<T>::BufferedInput(std::streamsize size)
{
    m_maxsize = size;
    m_cursize = size;
    m_buffer = (char*)malloc(sizeof(char) * size);
}

template <class T>
BufferedInput<T>::~BufferedInput()
{
    free(m_buffer);
}

template <class T>
void BufferedInput<T>::unset_file()
{
    this->m_infile = nullptr;
    this->m_block_end = false;
    this->cursize = this->m_maxsize;
    this->m_curpos = 0;
}

template <class T>
void BufferedInput<T>::refresh()
{
    if (m_infile->eof())
    {
        this->m_block_end = true;
        return;
    }
    this->m_block_end = false;
    if (m_curpos > 0)
    {
        std::streamsize num_trailing = m_cursize-m_curpos;
        memmove(m_buffer, m_buffer+m_curpos, num_trailing);
        //memcpy(m_buffer, m_buffer+m_curpos, num_trailing);
        this->m_infile->read(m_buffer+num_trailing, m_curpos);
        m_cursize = this->m_infile->gcount() + num_trailing;
    } else {
        this->m_infile->read(m_buffer, m_cursize);
        m_cursize = this->m_infile->gcount();
    }
    /*
    std::cout << this->m_infile->gcount() << " bytes read, current size is " << m_cursize << '\n';
    std::cout << "\n>>START OF CHUNK<<\n|";
    for (int i = 0; i < m_cursize; ++i) {std::cout << *(m_buffer+i);}
    std::cout << "|\n>>END OF CHUNK<<\n\n";
    */
    m_curpos = 0;
    std::streamsize new_size = m_curobj.read_new(m_buffer, m_buffer+m_cursize);
    std::cout << new_size << " bytes increment" <<"\n";
    if (new_size < 0)
    { // could not read object from block start -> not enough memory in buffer
        throw BufferSizeExceeded();
    } else {
        m_curpos += new_size;
    }
}

template <class T>
T BufferedInput<T>::next()
{
    T to_return = std::move(m_curobj);
    std::streamsize new_size = m_curobj.read_new(m_buffer+m_curpos, m_buffer+m_cursize);
    if (new_size < 0)
    { // could not read another object
        m_block_end = true;
    } else {
        m_curpos += new_size;
    }
    return to_return;
}
