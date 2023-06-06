#pragma once
#include "boost/format.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>

#include "constants.h"
#include "bufferedinput.hpp"
#include "file_utils.hpp"


template<class T>
struct RecordPair
{
    T left;
    T right;
    RecordPair(const T& first, const T& second) : left(first), right(second) {}
    bool operator<(const RecordPair& other) const
    {
        //return (this->left < other.left) && (this->right < other.right);
        int val = this->left.cmp(other.left);
        if (val < 0) return true;
        if (val > 0) return false;
        return (this->right.cmp(other.right) < 0);
    }
    bool operator>(const RecordPair& other) const
    {
        //return (this->left > other.left) && (this->right > other.right);
        int val = this->left.cmp(other.left);
        if (val > 0) return true;
        if (val < 0) return false;
        return (this->right.cmp(other.right) > 0);
    }
};

template<class T>
struct PairedQueueNode
{
    ssize_t m_index;
    RecordPair<T> m_data;
    PairedQueueNode (ssize_t i, T right, T left) : m_index(i), m_data(right, left) {}
    // invert the sign so that max_heap turns into min_heap
    bool operator<(const PairedQueueNode& other) const {
        return (this->m_data > other.m_data);
    }
    const RecordPair<T>& data() const { return this->m_data; } 
};

template <class T>
class PairedExternalSorter
{
public:
    PairedExternalSorter(ssize_t);
    ~PairedExternalSorter();
    void sort(const char*, const char*, const char*, const char*);
private:
    void sort_buckets(const char*, const char*);
    void mergeHelper(ssize_t, ssize_t, ssize_t);
    void merge(const char*, const char*);
    void reserve(ssize_t);
private:
    ssize_t m_memlimit, m_filesNum;
    char m_tempdir[DIRNAME_LEN + 1];
    std::priority_queue<PairedQueueNode<T>, std::vector<PairedQueueNode<T>>> m_queue;
    std::vector<BufferedInput<T>> m_buffers;
    std::vector<std::ifstream> m_inputs;
};

template <class T>
PairedExternalSorter<T>::PairedExternalSorter(ssize_t memlimit)
{
    m_memlimit = memlimit;
    m_tempdir[DIRNAME_LEN] = '\0';
    create_random_dir(m_tempdir, DIRNAME_LEN);
}

template <class T>
PairedExternalSorter<T>::~PairedExternalSorter() { FS::remove_all(m_tempdir); }

template <class T>
void PairedExternalSorter<T>::sort(const char* infilename1,
                                   const char* infilename2,
                                   const char* outfilename1,
                                   const char* outfilename2)
{
    this->sort_buckets(infilename1, infilename2);
    this->merge(outfilename1, outfilename2);
}

template <class T>
void PairedExternalSorter<T>::reserve(ssize_t count)
{
    // queue
    std::vector<PairedQueueNode<T>> container;
    container.reserve(count);
    m_queue = std::priority_queue<PairedQueueNode<T>, std::vector<PairedQueueNode<T>>>
        (std::less<PairedQueueNode<T>>(), std::move(container));
    // input buffers
    ssize_t mem = std::max(this->m_memlimit / (count*2), constants::ONE_MB);
    this->m_buffers.reserve(count * 2);
    for (ssize_t i = 0; i < count*2; ++i)
        this->m_buffers.emplace_back(mem);
    // input files
    this->m_inputs = std::vector<std::ifstream>(count * 2);
}

template <class T>
void PairedExternalSorter<T>::sort_buckets(const char* infilename1,
                                           const char* infilename2)
{   // maybe be moved outside for gz files support
    std::ifstream input1{infilename1};
    std::ifstream input2{infilename2};
    std::ofstream output1;
    std::ofstream output2;
    check_fstream_ok<std::ifstream>(input1, infilename1);
    check_fstream_ok<std::ifstream>(input2, infilename2);

    m_filesNum = 0;
    std::vector<RecordPair<T>> arr;
    // TODO arr.reserve???
    // TODO memlimit / 3 or some close number
    BufferedInput<T> buffer1(m_memlimit / 2);
    BufferedInput<T> buffer2(m_memlimit / 2);
    buffer1.set_file(&input1);
    buffer2.set_file(&input2);

    while(!buffer1.eof() && !buffer2.eof())
    {
        m_filesNum++;
        // read paired chunks of "view" objects from files
        while (!buffer1.block_end() && !buffer2.block_end())
            arr.emplace_back(buffer1.next(), buffer2.next());
        // sort objects
        std::sort(arr.begin(), arr.end());
        // save sorted chunks to paired files in tmp dir
        boost::format outname1 = boost::format("%1%/%2%_1.tmp") % m_tempdir % (m_filesNum - 1);
        boost::format outname2 = boost::format("%1%/%2%_2.tmp") % m_tempdir % (m_filesNum - 1);
        output1.open(outname1.str());
        output2.open(outname2.str());
        check_fstream_ok<std::ofstream>(output1, outname1.str().c_str());
        check_fstream_ok<std::ofstream>(output2, outname2.str().c_str());
        for (auto& item: arr)
        {
            output1 << item.left;
            output2 << item.right;
        }
        output1.close();
        output2.close();
        // empty array and load new chunks of data
        arr.clear();
        buffer1.refresh();
        buffer2.refresh();
    }
}

template <class T>
void PairedExternalSorter<T>::mergeHelper(ssize_t start,
                                          ssize_t end,
                                          ssize_t location)
{
    ssize_t filesCount = end - start;
    // set up files
    for (ssize_t i = 0; i < filesCount; ++i) {
        boost::format inpname1 = boost::format("%1%/%2%_1.tmp") % m_tempdir % (start+i);
        boost::format inpname2 = boost::format("%1%/%2%_2.tmp") % m_tempdir % (start+i);
        m_inputs[2*i].open(inpname1.str());
        m_inputs[2*i+1].open(inpname2.str());
        check_fstream_ok<std::ifstream>(m_inputs[2*i], inpname1.str().c_str());
        check_fstream_ok<std::ifstream>(m_inputs[2*i+1], inpname2.str().c_str());
        m_buffers[2*i].set_file(&(m_inputs[2*i]));
        m_buffers[2*i+1].set_file(&(m_inputs[2*i+1]));
    }
    // initially fill up queue
    for (ssize_t i = 0; i < filesCount; ++i) 
    {
        if (!m_buffers[2*i].eof() && !m_buffers[2*i+1].eof())
        {
            m_queue.emplace(i, m_buffers[2*i].next(), m_buffers[2*i+1].next());
        }
    }
    // output files
    std::ofstream output1, output2;
    boost::format outname1 = boost::format("%1%/%2%_1.tmp") % m_tempdir % location;
    boost::format outname2 = boost::format("%1%/%2%_2.tmp") % m_tempdir % location;
    output1.open(outname1.str());
    output2.open(outname2.str());
    check_fstream_ok<std::ofstream>(output1, outname1.str().c_str());
    check_fstream_ok<std::ofstream>(output2, outname2.str().c_str());
    // merge files iteratively
    while (!m_queue.empty())
    {
        ssize_t index = m_queue.top().m_index;
        output1 << m_queue.top().data().left;
        output2 << m_queue.top().data().right;
        m_queue.pop();
        // push new item from file to queue if possible
        if (m_buffers[2*index].block_end())
            m_buffers[2*index].refresh();
        if (m_buffers[2*index+1].block_end())
            m_buffers[2*index+1].refresh();
        if (!m_buffers[2*index].eof() && !m_buffers[2*index+1].eof())
            m_queue.emplace(index, m_buffers[2*index].next(), m_buffers[2*index+1].next());
    }
    // cleanup
    for (ssize_t i = 0; i < filesCount; ++i)
    {
        m_inputs[2*i].close();
        m_inputs[2*i+1].close();
        m_buffers[2*i].unset_file();
        m_buffers[2*i+1].unset_file();
    }
    output1.close();
    output2.close();
}

template <class T>
void PairedExternalSorter<T>::merge(const char* outfilename1,
                                    const char* outfilename2)
{
    ssize_t start = 0, end = m_filesNum, step = 50L;
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
    boost::format name1 = boost::format("%1%/%2%_1.tmp") % m_tempdir % start;
    boost::format name2 = boost::format("%1%/%2%_2.tmp") % m_tempdir % start;
    std::rename(name1.str().c_str(), outfilename1);
    std::rename(name2.str().c_str(), outfilename2);
}