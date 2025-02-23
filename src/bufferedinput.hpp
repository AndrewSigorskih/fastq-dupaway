#pragma once
#include <fstream>
#include "constants.hpp"
#include "file_utils.hpp"

using FileUtils::I_InputFile;

template <class T>
class BufferedInput
{
public:
    BufferedInput(std::streamsize);
    ~BufferedInput()                    { free(m_buffer); this->unset_file(); }
    bool eof()                          { return m_infile->eof() && m_block_end; }
    bool block_end()                    { return m_block_end; }
    void set_file(const char* infilename);
    void unset_file();
    void refresh();
    T next();

private:
    T m_curobj;
    std::streamsize m_maxsize, m_cursize, m_curpos = 0;
    I_InputFile* m_infile = nullptr;
    char* m_buffer = nullptr;
    bool m_block_end = false;
};

template <class T>
BufferedInput<T>::BufferedInput(std::streamsize size)
{
    m_maxsize = size;
    m_cursize = size;
    m_buffer = (char*)malloc(sizeof(char) * size);
}

template <class T>
void BufferedInput<T>::set_file(const char* infilename)
{
    m_infile = FileUtils::openInputFile(infilename);
    this->refresh();
}

template <class T>
void BufferedInput<T>::unset_file()
{
    if (m_infile != nullptr)
    {
        delete m_infile;
        m_infile = nullptr;
    }
    m_block_end = false;
    m_cursize = m_maxsize;
    m_curpos = 0;
}

template <class T>
void BufferedInput<T>::refresh()
{
    if (m_infile->eof())
    {
        m_block_end = true;
        return;
    }
    m_block_end = false;
    if (m_curpos > 0)
    {
        if (!m_curobj.isEmpty())
            m_curpos -= m_curobj.size();
        std::streamsize num_trailing = m_cursize - m_curpos;
        memmove(m_buffer, m_buffer+m_curpos, num_trailing);
        //memcpy(m_buffer, m_buffer+m_curpos, num_trailing);
        this->m_infile->read(m_buffer+num_trailing, m_curpos);
        m_cursize = m_infile->gcount() + num_trailing;
    } else {
        this->m_infile->read(m_buffer, m_cursize);
        m_cursize = m_infile->gcount();
    }
    
    m_curpos = 0;
    std::streamsize new_size = m_curobj.read_new(m_buffer, m_buffer+m_cursize);
    if (new_size < 0)
    { // could not read object from block start -> not enough memory in buffer
        throw std::runtime_error("Not enough memory to read a single object!");
    } else {
        m_curpos += new_size;
    }
}

template <class T>
T BufferedInput<T>::next()
{
    T to_return = std::move(m_curobj);
    //if (!m_curobj.isEmpty()) { throw std::runtime_error("object not cleared"); } // TODO remove
    std::streamsize new_size = m_curobj.read_new(m_buffer+m_curpos, m_buffer+m_cursize);
    if (new_size < 0)
    { // could not read another object
        m_block_end = true;
    } else {
        m_curpos += new_size;
    }
    return to_return;
}
