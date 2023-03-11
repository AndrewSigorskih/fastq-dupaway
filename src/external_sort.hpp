#pragma once
#include "boost/format.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>

#include "file_utils.hpp"

template<class T>
struct QueueNode
{
    uint64_t m_index;
    T m_data;
    QueueNode (uint64_t i, T item) : m_index(i), m_data(std::move(item)) {}
    // invert the sign so that max_heap turns into min_heap
    bool operator<(const QueueNode& other) const {
        return (this->m_data > other.m_data);
    }

    const T& data() const { return this->m_data; } 
};

template <class T, int LinesPerObj = 1>
class ExternalSorter
{
private:
    uint64_t m_batchsize, m_linesCount, m_filesNum;
    char m_tempdir[DIRNAME_LEN + 1];
    std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>> m_queue;
public:
    ExternalSorter(uint64_t batch, uint64_t linesCount) : 
                m_batchsize(batch), m_linesCount(linesCount)
    {
        m_tempdir[DIRNAME_LEN] = '\0';
        std::vector<QueueNode<T>> container;
        container.reserve(m_batchsize);
        m_queue = std::priority_queue<QueueNode<T>, std::vector<QueueNode<T>>>
            (std::less<QueueNode<T>>(), std::move(container));
    }
    void sort(const char*);
    void mergeHelper(uint64_t, uint64_t, uint64_t);
    void merge(const char*);
    void operator()(const char* infilename, const char* outfilename);
};

template <class T, int LinesPerObj>
void ExternalSorter<T, LinesPerObj>::operator()
    (const char* infilename, const char* outfilename)
{
    create_random_dir(m_tempdir, DIRNAME_LEN);
    //printf("Created dir: %s\n", m_tempdir);  // DEBUG
    this->sort(infilename);
    this->merge(outfilename);
    FS::remove_all(m_tempdir);
}

template <class T, int LinesPerObj>
void ExternalSorter<T, LinesPerObj>::sort(const char* infilename)
{
    std::cout << "lines in file: " << m_linesCount << std::endl;
    if ((m_linesCount % LinesPerObj) != 0)
    {  // change to exception throw?
        std::cerr << "Error: wrong file format!\n";
        std::cerr << "Cannot infer non-fractional number of objects from file, exiting.\n";
        exit(1);
    }

    std::ifstream input{infilename};
    check_fstream_ok<std::ifstream>(input, infilename);

    uint64_t currCount, linesPerBatch, remainder;
    linesPerBatch = m_batchsize * LinesPerObj;
    remainder = (m_linesCount % linesPerBatch) / LinesPerObj;
    m_filesNum = (m_linesCount / linesPerBatch);
    if (remainder) m_filesNum += 1;

    T* arr = new T[m_batchsize];
    std::ofstream output;

    std::cout <<"n runs: "<< m_filesNum << " lines pb: " << linesPerBatch<< std::endl;

    for (uint64_t i = 0; i < m_filesNum; ++i)
    {
        // read input file in batches
        if (!remainder)
            currCount = m_batchsize;
        else
            currCount = i < (m_filesNum-1) ? m_batchsize : remainder;
        for (uint64_t j = 0; j < currCount; ++j)
            input >> *(arr+j);
        // sort batch
        std::sort(arr, arr+currCount);
        // write to temporal files in test dir
        boost::format fmt = boost::format("%1%/%2%.tmp") % m_tempdir % i;
        output.open(fmt.str());
        check_fstream_ok<std::ofstream>(output, fmt.str().c_str());
        for (uint64_t j = 0; j < currCount; ++j)
            output << *(arr+j) << '\n';
        output.close();
    }   
    delete[] arr;
}

template <class T, int LinesPerObj>
void ExternalSorter<T, LinesPerObj>::mergeHelper(uint64_t start,
                                                 uint64_t end,
                                                 uint64_t location)
{
    uint64_t filesCount = end - start ;
    std::ifstream input[filesCount];
    for (uint64_t i = 0; i < filesCount; ++i) {
       boost::format fmt = boost::format("%1%/%2%.tmp") % m_tempdir % (start+i);
       input[i].open(fmt.str());
       check_fstream_ok<std::ifstream>(input[i], fmt.str().c_str());
    }

    for (uint64_t i = 0; i < filesCount; ++i) 
    {
        if (!input[i].eof()) 
        {
            T item;
            input[i] >> item;
            m_queue.emplace(i, std::move(item));
        }
    }

    std::ofstream output;
    boost::format fmt = boost::format("%1%/%2%.tmp") % m_tempdir % location;
    output.open(fmt.str());
    check_fstream_ok<std::ofstream>(output, fmt.str().c_str());

    while (!m_queue.empty())
    {
        uint64_t index = m_queue.top().m_index;
        output << m_queue.top().data() << '\n';
        m_queue.pop();
        // in case of 1-line objects like int
        // eat trailing \n so we dont go into infinite loop
        input[index] >> std::ws;
        if (!input[index].eof()) {
            T item;
            input[index] >> item;
            m_queue.emplace(index, std::move(item));
        }
    }
    for (uint64_t i = 0; i < filesCount; ++i)
        { input[i].close(); }
    output.close();
}

template <class T, int LinesPerObj>
void ExternalSorter<T, LinesPerObj>::merge(const char* outfilename)
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
    boost::format fmt = boost::format("%1%/%2%.tmp") % m_tempdir % start;
    std::rename(fmt.str().c_str(), outfilename);
}
