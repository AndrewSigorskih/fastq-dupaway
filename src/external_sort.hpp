#pragma once
#include "boost/format.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>

#include "constants.hpp"
#include "bufferedinput.hpp"
#include "file_utils.hpp"


template<class T>
struct QueueNode
{
    ssize_t m_index;
    T m_data;
    QueueNode (ssize_t i, T item) : m_index(i), m_data(std::move(item)) {}
    // invert the sign so that max_heap turns into min_heap
    bool operator<(const QueueNode& other) const {
        return (this->m_data > other.m_data);
    }
    const T& data() const { return this->m_data; } 
};

template <class T>
class ExternalSorter
{
public:
    ExternalSorter(ssize_t, char*);
    ~ExternalSorter();
    void sort(const char*, const char*);
private:
    void sort_buckets(const char*);
    void mergeHelper(ssize_t, ssize_t, ssize_t);
    void merge(const char*);
    void reserve(ssize_t);
private:
    ssize_t m_memlimit, m_filesNum;
    char* m_tempdir;
    char* m_workdir;
    std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>> m_queue;
    std::vector<BufferedInput<T>> m_buffers;
    std::vector<std::ifstream> m_inputs;
};

template <class T>
ExternalSorter<T>::ExternalSorter(ssize_t memlimit,
                                  char* workdir)
{
    m_memlimit = memlimit;
    m_workdir = workdir;
    m_tempdir = (char*)malloc(sizeof(char)*(constants::DIRNAME_LEN + 1));
    m_tempdir[constants::DIRNAME_LEN] = '\0';
    std::ignore = chdir(m_workdir);
    create_random_dir(m_tempdir, constants::DIRNAME_LEN);
    std::ignore = chdir("..");
}

template <class T>
ExternalSorter<T>::~ExternalSorter() 
{
    std::ignore = chdir(m_workdir);
    FS::remove_all(m_tempdir);
    std::ignore = chdir("..");
    free(m_tempdir);
}

template <class T>
void ExternalSorter<T>::sort(const char* infilename,  
                             const char* outfilename)
{
    this->sort_buckets(infilename);
    this->merge(outfilename);
}

template <class T>
void ExternalSorter<T>::reserve(ssize_t count)
{
    // queue
    std::vector<QueueNode<T>> container;
    container.reserve(count);
    m_queue = std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>>
        (std::less<QueueNode<T>>(), std::move(container));
    // input buffers
    ssize_t mem = std::max(this->m_memlimit / count, constants::ONE_MB);
    this->m_buffers.reserve(count);
    for (ssize_t i = 0; i < count; ++i)
        this->m_buffers.emplace_back(mem);
    // input files
    this->m_inputs = std::vector<std::ifstream>(count);
}

template <class T>
void ExternalSorter<T>::sort_buckets(const char* infilename)
{   // maybe be moved outside for gz files support
    std::ifstream input{infilename};
    std::ofstream output;
    check_fstream_ok<std::ifstream>(input, infilename);

    m_filesNum = 0;
    std::vector<T> arr;
    // "view" objects take up to 1/3 of corresponding memory chunk
    BufferedInput<T> buffer((m_memlimit / 3) * 2);
    buffer.set_file(&input);
    
    while(!buffer.eof())
    {
        m_filesNum++;
        // read chunk of "view" objects from file
        while (!buffer.block_end())
            arr.push_back(buffer.next());
        // sort objects
        std::sort(arr.begin(), arr.end());
        // save sorted chunk to file in tmp dir
        boost::format fmt = boost::format("%1%/%2%/%3%.tmp") % m_workdir % m_tempdir % (m_filesNum - 1);
        output.open(fmt.str());
        check_fstream_ok<std::ofstream>(output, fmt.str().c_str());
        for (auto& item: arr)
            output << item;
        output.close();
        // empty array and load new chunk of data
        arr.clear();
        buffer.refresh();
    }
}

template <class T>
void ExternalSorter<T>::mergeHelper(ssize_t start,
                                    ssize_t end,
                                    ssize_t location)
{
    ssize_t filesCount = end - start;
    std::vector<std::string> filenames;
    filenames.reserve(filesCount);
    // set up files
    for (ssize_t i = 0; i < filesCount; ++i) {
        boost::format fmt = boost::format("%1%/%2%/%3%.tmp") % m_workdir % m_tempdir % (start+i);
        filenames.push_back(fmt.str());
        m_inputs[i].open(filenames[i]);
        check_fstream_ok<std::ifstream>(m_inputs[i], filenames[i].c_str());
        m_buffers[i].set_file(&(m_inputs[i]));
    }
    // initially fill up queue
    for (ssize_t i = 0; i < filesCount; ++i) 
    {
        if (!m_buffers[i].eof()) 
        {
            m_queue.emplace(i, m_buffers[i].next());
        }
    }
    // output file
    std::ofstream output;
    boost::format fmt = boost::format("%1%/%2%/%3%.tmp") % m_workdir % m_tempdir % location;
    output.open(fmt.str());
    check_fstream_ok<std::ofstream>(output, fmt.str().c_str());
    // merge files iteratively
    while (!m_queue.empty())
    {
        ssize_t index = m_queue.top().m_index;
        output << m_queue.top().data();
        m_queue.pop();
        // push new item from file to queue if possible
        if (m_buffers[index].block_end())
            m_buffers[index].refresh();
        if (!m_buffers[index].eof())
            m_queue.emplace(index, m_buffers[index].next());
    }
    // cleanup
    for (ssize_t i = 0; i < filesCount; ++i)
    {
        m_inputs[i].close();
        m_buffers[i].unset_file();
    }
    output.close();
    for (auto& name: filenames)
        FS::remove(name.c_str());
}

template <class T>
void ExternalSorter<T>::merge(const char* outfilename)
{
    ssize_t start = 0, end = m_filesNum, step = 100L;
    if (step > end-start)
        step = (end - start + 1) / 2 + 1;
    // reserve needed memory
    this->reserve(step);
    // actual file merging loop
    while (true)
    {
        ssize_t location = end, dist = std::min(step, end-start+1);
        ssize_t mid = start + dist;
        if (mid > end) 
            break;
        mergeHelper(start, mid, location);
        ++end;
        start = mid;
    }
    if (start < end-1)
    { // one last run
        mergeHelper(start, end, end+1);
        start = end + 1;
    }
    boost::format fmt = boost::format("%1%/%2%/%3%.tmp") % m_workdir % m_tempdir % start;
    std::rename(fmt.str().c_str(), outfilename);
}
