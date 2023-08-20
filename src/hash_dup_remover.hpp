#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <boost/functional/hash.hpp>
// maybe change hash_combine to https://stackoverflow.com/a/72073933
// or https://www.biostars.org/p/184993/#185003

#include "bufferedinput.hpp"
#include "external_sort.hpp"
#include "file_utils.hpp"

using std::string;
using FileUtils::TemporaryDirectory;
struct setRecordHash;
struct setRecordPairHash;
const long ONE_MIL = 1000L * 1000L;

class setRecord
{
public:
    setRecord() {}
    setRecord(const char*, ssize_t);
    bool operator==(const setRecord&) const;
    friend setRecordHash;
private:
    ssize_t m_seq_len;
    std::vector<uint64_t> m_hash;
};

class setRecordPair
{
public:
    setRecordPair() {}
    setRecordPair(const char*, ssize_t, const char*, ssize_t);
    bool operator==(const setRecordPair&) const;
    friend setRecordPairHash;
private:
    ssize_t m_l_len, m_r_len;
    std::vector<uint64_t> m_l_hash, m_r_hash;
};

struct setRecordHash
{
    std::size_t operator()(const setRecord &item) const
    {
        std::size_t seed = item.m_hash.size();
        for (auto& i: item.m_hash)
            boost::hash_combine(seed, i);
        return seed;
    }
};

struct setRecordPairHash
{
    uint64_t operator()(const setRecordPair &item) const
    {
        std::size_t seed = item.m_l_hash.size();
        for (auto& i: item.m_l_hash)
            boost::hash_combine(seed, i);

        boost::hash_combine(seed, item.m_r_hash.size());
        for (auto& i: item.m_r_hash)
            boost::hash_combine(seed, i);

        return seed;
    }
};

using hashed_set = std::unordered_set<setRecord, setRecordHash>;
using paired_hashed_set = std::unordered_set<setRecordPair, setRecordPairHash>;

template<class T>
class HashDupRemover
{
public:
    HashDupRemover(ssize_t, TemporaryDirectory*);
    ~HashDupRemover() {}
    void filterSE(const string&, const string&);
    void filterPE(const string&, const string&,
                  const string&, const string&,
                  bool);
private:
    void impl_filterSE(const char*, const char*);
    void impl_filterPE(const char*, const char*,
                       const char*, const char*);
private:
    ssize_t m_memlimit;
    TemporaryDirectory* m_tempdir;
};

template<class T>
HashDupRemover<T>::HashDupRemover(ssize_t memlimit, TemporaryDirectory* tempdir)
{
    this->m_memlimit = memlimit;
    this->m_tempdir = tempdir;
}

template<class T>
void HashDupRemover<T>::filterSE(const string& infile,
                                 const string& outfile)
{
    // gunzip file if needed
    m_tempdir->set_files(infile);

    // deduplicate file
    this->impl_filterSE(m_tempdir->input1().c_str(),
                        m_tempdir->output1().c_str());

    // save output
    m_tempdir->save_output(outfile);
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
    records.reserve(ONE_MIL);  // TODO optimize this value?
    BufferedInput<T> buffer(5L * constants::HUNDRED_MB);

    buffer.set_file(&input);
    obj = buffer.next();
    output << obj;
    records.insert(std::move(setRecord(obj.seq(), obj.seq_len()-1)));

    while (!buffer.eof())
    {
        while (!buffer.block_end())
        {
            obj = buffer.next();
            setRecord record(obj.seq(), obj.seq_len()-1);
            auto it = records.find(record);
            if (it == records.end())
            {   
                output << obj;
                records.insert(std::move(record));
            }
        }
        buffer.refresh();
    }
}

template<class T>
void HashDupRemover<T>::filterPE(const string& infile1,
                                 const string& infile2,
                                 const string& outfile1,
                                 const string& outfile2,
                                 bool to_sort)
{
    // gunzip inputs if needed
    m_tempdir->set_files(infile1, infile2);
    string infilename1 = m_tempdir->input1();
    string infilename2 = m_tempdir->input2();

    // sort both files by ID if needed
    if (to_sort)
    {
        string tmp1 = (boost::format("%1%/data.sorted1") % m_tempdir->name()).str();
        string tmp2 = (boost::format("%1%/data.sorted2") % m_tempdir->name()).str();

        // sort first file
        {
            ExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
            sorter.sort(infilename1.c_str(), tmp1.c_str());
        }

        // sort second file
        {
            ExternalSorter<T> sorter(m_memlimit, m_tempdir->name());
            sorter.sort(infilename2.c_str(), tmp2.c_str());
        }

        infilename1 = tmp1;
        infilename2 = tmp2;

        // remove ungzipped inputs (or input symlinks) here
        m_tempdir->clear_inputs();
    }

    // deduplicate 2 files
    this->impl_filterPE(infilename1.c_str(), infilename2.c_str(),
                        m_tempdir->output1().c_str(),
                        m_tempdir->output2().c_str());
    // save outputs
    m_tempdir->save_output(outfile1, outfile2);
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
    records.reserve(ONE_MIL);  // TODO optimize this value?
    BufferedInput<T> left_buffer(5L * constants::HUNDRED_MB), right_buffer(5L * constants::HUNDRED_MB);
    left_buffer.set_file(&input1);
    right_buffer.set_file(&input2);
    left = left_buffer.next();
    right = right_buffer.next();

    output1 << left;
    output2 << right;
    records.insert(
        std::move(
            setRecordPair(left.seq(), left.seq_len()-1,
                          right.seq(), right.seq_len()-1)
        )
    );

    while (!left_buffer.eof() && !right_buffer.eof())
    {
        while (!left_buffer.block_end() && !right_buffer.block_end())
        {
            left = left_buffer.next();
            right = right_buffer.next();
            // TODO add AND TEST unmatching ids case!!!
            setRecordPair record(left.seq(), left.seq_len()-1,
                                 right.seq(), right.seq_len()-1);
            auto it = records.find(record);
            if (it == records.end())
            {
                output1 << left;
                output2 << right;
                records.insert(std::move(record));
            }
        }
        left_buffer.refresh();
        right_buffer.refresh();
    }
}