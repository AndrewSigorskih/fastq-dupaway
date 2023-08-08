#pragma once
#include <string>
#include <vector>
#include <unordered_set>

#include "bufferedinput.hpp"
#include "external_sort.hpp"
#include "file_utils.hpp"

using std::string;

class setRecord
{
public:
    setRecord() {}
    setRecord(const char*, ssize_t);
    bool operator==(const setRecord&) const;
    inline uint64_t hash_prefix() const { return m_hash[0]; }
private:
    ssize_t m_seq_len;
    std::vector<uint64_t> m_hash;
};

class setRecordPair
{
public:
    setRecordPair() {}
    setRecordPair(const setRecord&, const setRecord&);
    bool operator==(const setRecordPair&) const;
    inline uint64_t hash_prefix() const { return this->left.hash_prefix(); }
private:
    setRecord left;
    setRecord right;
};

struct setRecordHash
{
    uint64_t operator()(const setRecord &item) const
        { return item.hash_prefix(); }

    uint64_t operator()(const setRecordPair &item) const
        { return item.hash_prefix(); }
};

template<class T>
class HashDupRemover
{
public:
    HashDupRemover(ssize_t, bool);
    ~HashDupRemover();
    void filterSE(const string&, const string&);
    void filterPE(const string&, const string&,
                  const string&, const string&);
private:
    void impl_filterSE(const char*, const char*);
    void impl_filterPE(const char*, const char*,
                       const char*, const char*);
private:
    ssize_t m_memlimit;
    char* m_tempdir;
    bool m_to_sort;
};

template<class T>
HashDupRemover<T>::HashDupRemover(ssize_t memlimit, bool to_sort)
{
    this->m_memlimit = memlimit;
    this->m_to_sort = to_sort;
    m_tempdir = (char*)malloc(sizeof(char)*(constants::DIRNAME_LEN + 1));
    m_tempdir[constants::DIRNAME_LEN] = '\0';
    create_random_dir(m_tempdir, constants::DIRNAME_LEN);
}

template<class T>
HashDupRemover<T>::~HashDupRemover()
{
    FS::remove_all(m_tempdir);
    free(m_tempdir);
}

using hashed_set = std::unordered_set<setRecord, setRecordHash>;
using paired_hashed_set = std::unordered_set<setRecordPair, setRecordHash>;

template<class T>
void HashDupRemover<T>::filterSE(const string& infile,
                                 const string& outfile)
{
    string infilename = infile;
    // TODO deal with gzipped input

    string filtered = (boost::format("%1%/result.out") % m_tempdir).str();
    // deduplicate file
    this->impl_filterSE(infilename.c_str(), filtered.c_str());

    // TODO deal with gzipped output
    std::rename(filtered.c_str(), outfile.c_str());
}

template<class T>
void HashDupRemover<T>::impl_filterSE(const char* infilename,
                                      const char* outfilename)
{
    std::ifstream input{infilename};
    std::ofstream output{outfilename};
    check_fstream_ok<std::ifstream>(input, infilename);
    check_fstream_ok<std::ofstream>(output, outfilename);

    T obj;
    hashed_set records;
    BufferedInput<T> buffer(5L * constants::HUNDRED_MB);

    buffer.set_file(&input);
    obj = buffer.next();
    output << obj;
    records.insert(setRecord(obj.seq(), obj.seq_len()-1));

    size_t cnt_all = 1L, cnt_good = 1L, cnt_collis = 0L, cnt_equal = 0L;

    size_t i = SIZE_MAX;
    uint64_t aboba = ULONG_MAX;
    if (i == aboba)
        std::cout << "size_max == ulong_max";
    else
        std::cout << "size_max != ulong_max";

    while (!buffer.eof())
    {
        while (!buffer.block_end())
        {
            ++cnt_all;
            obj = buffer.next();
            setRecord record(obj.seq(), obj.seq_len()-1);
            auto it = records.find(record);
            if (it == records.end())
            {   
                ++cnt_good;
                output << obj;
                records.insert(record);
            } else {
                ++cnt_collis;
                if (record == *it)
                    ++cnt_equal;
            }
        }
        buffer.refresh();
    }
    std::cout << cnt_all << " reads processed totally\n";
    std::cout << cnt_good << " reads marked as unique\n";
    std::cout << cnt_collis << "hash collisions\n";
    std::cout << cnt_equal << "duplicated by len and hash values\n";
}

template<class T>
void HashDupRemover<T>::filterPE(const string& infile1,
                                 const string& infile2,
                                 const string& outfile1,
                                 const string& outfile2)
{
    string infilename1 = infile1, infilename2 = infile2;
    string filtered1 = (boost::format("%1%/data.out1") % m_tempdir).str();
    string filtered2 = (boost::format("%1%/data.out2") % m_tempdir).str();
    // TODO deal with gzipped input here

    // sort both files by ID
    if (this->m_to_sort)
    {
        string tmp1 = (boost::format("%1%/data.sorted1") % m_tempdir).str();
        string tmp2 = (boost::format("%1%/data.sorted2") % m_tempdir).str();

        // sort first file
        {
            ExternalSorter<T> sorter(m_memlimit, m_tempdir);
            sorter.sort(infilename1.c_str(), tmp1.c_str());
        }

        // sort second file
        {
            ExternalSorter<T> sorter(m_memlimit, m_tempdir);
            sorter.sort(infilename2.c_str(), tmp2.c_str());
        }

        infilename1 = tmp1;
        infilename2 = tmp2;
    }

    // deduplicate 2 files
    this->impl_filterPE(infilename1.c_str(), infilename1.c_str(),
                        filtered1.c_str(), filtered2.c_str());
    // TODO gzip if output is gz else
    std::rename(filtered1.c_str(), outfile1.c_str());
    std::rename(filtered2.c_str(), outfile2.c_str());
}

template<class T>
void HashDupRemover<T>::impl_filterPE(const char* infile1,
                                      const char* infile2,
                                      const char* outfile1,
                                      const char* outfile2)
{
    std::ifstream input1{infile1};
    std::ifstream input2{infile2};
    std::ofstream output1{outfile1};
    std::ofstream output2{outfile2};
    check_fstream_ok<std::ifstream>(input1, infile1);
    check_fstream_ok<std::ifstream>(input2, infile2);
    check_fstream_ok<std::ofstream>(output1, outfile1);
    check_fstream_ok<std::ofstream>(output2, outfile2);

    T left, right;
    paired_hashed_set records;
    BufferedInput<T> left_buffer(5L * constants::HUNDRED_MB), right_buffer(5L * constants::HUNDRED_MB);
    left_buffer.set_file(&input1);
    right_buffer.set_file(&input2);
    left = left_buffer.next();
    right = right_buffer.next();

    output1 << left;
    output2 << right;
    records.insert(
        setRecordPair(
            setRecord(left.seq(), left.seq_len()-1),
            setRecord(right.seq(), right.seq_len()-1)
        )
    );

    while (!left_buffer.eof() && !right_buffer.eof())
    {
        while (!left_buffer.block_end() && !right_buffer.block_end())
        {
            left = left_buffer.next();
            right = right_buffer.next();
            // TODO add AND TEST unmatching ids case!!!
            setRecordPair record(
                setRecord(left.seq(), left.seq_len()-1),
                setRecord(right.seq(), right.seq_len()-1)
            );
            auto it = records.find(record);
            if (it == records.end())
            {
                output1 << left;
                output2 << right;
                records.insert(record);
            }
        }
        left_buffer.refresh();
        right_buffer.refresh();
    }
}