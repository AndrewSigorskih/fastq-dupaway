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
    ExternalSorter(ssize_t, const char*);
    ~ExternalSorter();
    void sort(const char*, const char*);
private:
    void sort_buckets(const char*);
    void mergeHelper(ssize_t, ssize_t, ssize_t);
    void merge(const char*);
    void reserve(ssize_t);
    void saveOutput(ssize_t, const char*);
private:
    ssize_t m_memlimit, m_filesNum;
    char* m_tempdir;
    const char* m_workdir;
    std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>> m_queue;
    std::vector<BufferedInput<T>> m_buffers;
};

template <class T>
ExternalSorter<T>::ExternalSorter(ssize_t memlimit,
                                  const char* workdir) : m_workdir(workdir)
{
    m_memlimit = memlimit;
    m_tempdir = (char*)malloc(sizeof(char)*(constants::DIRNAME_LEN + 1));
    m_tempdir[constants::DIRNAME_LEN] = '\0';
    std::ignore = chdir(m_workdir);
    FileUtils::create_random_dir(m_tempdir, constants::DIRNAME_LEN);
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
}

template <class T>
void ExternalSorter<T>::sort_buckets(const char* infilename)
{
    std::ofstream output;
    m_filesNum = 0;
    std::vector<T> arr;
    // "view" objects take up to 1/3 of corresponding memory chunk
    BufferedInput<T> buffer((m_memlimit / 3) * 2);
    buffer.set_file(infilename);
    
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
        m_buffers[i].set_file(filenames[i].c_str());
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
        m_buffers[i].unset_file();
    }
    output.close();
    for (auto& name: filenames)
        FS::remove(name.c_str());
}

template <class T>
void ExternalSorter<T>::merge(const char* outfilename)
{
    if (m_filesNum == 1)
    {// whole file was processed in a single chunk
        this->saveOutput(0, outfilename);
        return;
    }

    ssize_t start = 0, end = m_filesNum, step = 100L;

    if (step > end-start)
    {// can merge all files in one cycle
        this->reserve(m_filesNum);
        this->mergeHelper(start, end, end);
        this->saveOutput(end, outfilename);
        return;
    }

    // unlucky case: several cycles needed
    // reserve needed memory
    this->reserve(step);
    // actual file merging loop
    while (true)
    {
        ssize_t location = end, dist = std::min(step, end-start+1);
        ssize_t mid = start + dist;
        if (mid > end) 
            break;
        this->mergeHelper(start, mid, location);
        ++end;
        start = mid;
    }
    if (start < end-1)
    { // one last run
        this->mergeHelper(start, end, end+1);
        start = end + 1;
    }
    this->saveOutput(start, outfilename);
}

template <class T>
void ExternalSorter<T>::saveOutput(ssize_t idx,
                                   const char* outfilename)
{
    boost::format fmt = boost::format("%1%/%2%/%3%.tmp") % m_workdir % m_tempdir % idx;
    std::rename(fmt.str().c_str(), outfilename);
}
