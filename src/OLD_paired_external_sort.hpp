#pragma once
#include "boost/format.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>

#include "file_utils.hpp"
#include "bufferedinput.hpp"

template<class T>
struct RecordPair
{
    T left;
    T right;
    bool operator<(const RecordPair& other) const {
        return (this->left < other.left) && (this->right < other.right);
    }
    bool operator>(const RecordPair& other) const {
        return (this->left > other.left) && (this->right > other.right);
    }
    uint64_t mem_usage() const { return (this->left.mem_usage() + this->right.mem_usage()); }
};

template<class T>
struct QueueNode
{
    uint64_t m_index;
    T m_data; // TODO actually store 2 datas
    QueueNode (uint64_t i, T item) : m_index(i), m_data(std::move(item)) {}
    // invert the sign so that max_heap turns into min_heap
    bool operator<(const QueueNode& other) const {
        return (this->m_data > other.m_data);
    }

    const T& data() const { return this->m_data; } 
};

template <class T>
class PairedExternalSorter
{
public:
    PairedExternalSorter(uint64_t);
    void sort(const char*, const char*, const char*, const char*);
private:
    void sort_buckets(const char*, const char*);
    void mergeHelper(uint64_t, uint64_t, uint64_t);
    void merge(const char*, const char*);
private:
    uint64_t m_memlimit, m_filesNum;
    char m_tempdir[DIRNAME_LEN + 1];
    std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>> m_queue;
};

template <class T>
PairedExternalSorter<T>::PairedExternalSorter(uint64_t memlimit)
{
    m_memlimit = memlimit;
    m_tempdir[DIRNAME_LEN] = '\0';
    /* TODO estimate BATCHSIZE???????
    std::vector<QueueNode<T>> container;
    container.reserve(BATCHSIZE);
    m_queue = std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>>
        (std::less<QueueNode<T>>(), std::move(container));
    */
}

template <class T>
void PairedExternalSorter<T>::sort(const char* infilename1, 
                                   const char* infilename2,
                                   const char* outfilename1, 
                                   const char* outfilename2)
{
    create_random_dir(m_tempdir, DIRNAME_LEN);
    this->sort_buckets(infilename1, infilename2);
    this->merge(outfilename1, outfilename2);
    FS::remove_all(m_tempdir);
}

template <class T>
void PairedExternalSorter<T>::sort_buckets(const char* infilename1,
                                           const char* infilename2)
{   // maybe be moved outside fir gz files support
    std::ifstream input1{infilename1};
    std::ifstream input2{infilename2};
    check_fstream_ok<std::ifstream>(input1, infilename1);
    check_fstream_ok<std::ifstream>(input2, infilename2);
    
    std::ofstream output1, output2;
    std::vector<RecordPair<T>> arr;
    //TODO arr.reserve(????);

    m_filesNum = 0;
    RecordPair<T> item;
    
    while (!input1.eof() && !input2.eof())
    {   // read unknown amount of chunks from inputs, sort and store
        m_filesNum++;
        uint64_t mem_used = 0;
        while (mem_used < m_memlimit)
        {
            input1 >> item.left;
            input2 >> item.right;
            mem_used += item.mem_usage();
            arr.emplace_back(std::move(item));
            if (input1.eof() || input2.eof()) break;
        }
        std::sort(arr.begin(), arr.end());
        // save chunks to files
        boost::format outfilename1 = boost::format("%1%/%2%_1.tmp") % m_tempdir % i;
        boost::format outfilename2 = boost::format("%1%/%2%_2.tmp") % m_tempdir % i;
        output1.open(outfilename1.str());
        output2.open(outfilename2.str());
        check_fstream_ok<std::ofstream>(output1, outfilename1.str().c_str());
        check_fstream_ok<std::ofstream>(output2, outfilename2.str().c_str());
        for (auto& pair: arr)
        {
            output1 << pair.left;
            output2 << pair.right;
        }
        output1.close();
        output2.close();
        arr.clear();
    }
}

template <class T>
void PairedExternalSorter<T>::mergeHelper(uint64_t start,
                                          uint64_t end,
                                          uint64_t location)
{
    ;
}

template <class T>
void PairedExternalSorter<T>::merge(const char* outfilename1,
                                    const char* outfilename2)
{
    uint64_t start = 0, end = m_filesNum, step = 100uL;
    if (step > end-start)
        step = (end - start + 1) / 2 + 1;
    std::cout << step << std::endl;
    while (true)
    {
        uint64_t location = end, dist = std::min(step, end-start+1);
        uint64_t mid = start + dist;
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
    boost::format fmt1 = boost::format("%1%/%2%_1.tmp") % m_tempdir % start;
    boost::format fmt2 = boost::format("%1%/%2%_2.tmp") % m_tempdir % start;
    std::rename(fmt1.str().c_str(), outfilename1);
    std::rename(fmt2.str().c_str(), outfilename1);
}