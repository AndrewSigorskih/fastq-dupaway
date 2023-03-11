#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

#include "file_utils.hpp"
#include "external_sort.hpp"

using std::string;

struct setRecord
{
    size_t m_hashval;
    long long m_pos;

    setRecord(const string& seq, long long pos)
    {
        static std::hash<string> Hasher = std::hash<string>();
        m_hashval = Hasher(seq);
        m_pos = pos;
    }
    bool operator==(const setRecord &other) const {
        return (this->m_hashval == other.m_hashval); 
    }
};

struct setRecordHash 
{
    size_t operator()(const setRecord &item) const
        { return item.m_hashval; }
};

using hashed_set = std::unordered_set<setRecord, setRecordHash>;

template<class T, int LinesPerObj>
class DupRemover
{
    bool m_no_sort, m_synched;
    char m_tempdir[DIRNAME_LEN + 1];
    uint64_t m_objCount, m_memLimit;

    void hashFilterSE(const char* infile,
                      const char* outfile);
    void hashFilterPE(const char* infile1,
                      const char* infile2,
                      const char* outfile1,
                      const char* outfile2);
public:
    DupRemover(bool no_sort, bool synched, uint64_t memLimit) : 
               m_no_sort(no_sort), m_synched(synched), m_memLimit(memLimit)
    {
        m_tempdir[DIRNAME_LEN] = '\0';
        create_random_dir(m_tempdir, DIRNAME_LEN);
    }
    void FilterSE(const string& infile,
                  const string& outfile);
    void FilterPE(const string& infile1,
                  const string& infile2,
                  const string& outfile1,
                  const string& outfile2);
    ~DupRemover()
    {
        FS::remove_all(m_tempdir);
        std::cout << "destructor\n";
    }
};

template<class T, int LinesPerObj>
void DupRemover<T, LinesPerObj>::FilterSE(const string& infile,
                                          const string& outfile)
{
    string infilename = infile;
    if (fileHasExt(infile.c_str(), ".gz"))
    {
        std::cerr << "gzipped files are not supported yet!\n";
        return;
        //infilename = m_tempdir / file1.fastq
        // gunzipfile (infile, infilename)
    }
    uint64_t linesNum = countLines(infilename.c_str());
    uint64_t objNum = (linesNum / LinesPerObj);
    m_objCount = objNum;
    // check no extra lines in file?
    string outfilename = (boost::format("%1%/result.out") % m_tempdir).str();
    if (m_synched)
    {
        // sort file by sequence
        // and then dedup
        std:: cerr << "Error: sequence-based approach not implemented yet\n";
        return;
    } else { // actual hash-based approach
        hashFilterSE(infilename.c_str(),
                     outfilename.c_str());
    }
    // gzip outputs if needed else rename
    if (fileHasExt(outfile.c_str(), ".gz"))
    {
        std::cerr << "gzipped files are not supported yet!\n";
        // gzipfile (outfilename, outfile)
        return;
    } else {
        std::rename(outfilename.c_str(), outfile.c_str());
    }
}

template<class T, int LinesPerObj>
void DupRemover<T, LinesPerObj>::FilterPE(const string& infile1,
                                          const string& infile2,
                                          const string& outfile1,
                                          const string& outfile2)
{
    string infilename1 = infile1;
    string infilename2 = infile2;
    if (fileHasExt(infile1.c_str(), ".gz"))
    {
        std::cerr << "gzipped files are not supported yet!\n";
        // infilename1 = m_tempdir / file1.fastq
        // gunzipfile (infile1, infilename1)
        return;
    } 

    if (fileHasExt(infile2.c_str(), ".gz"))
    {
        std::cerr << "gzipped files are not supported yet!\n";
        // infilename2 = m_tempdir / file2.fastq
        // gunzipfile (infile2, infilename2)
        return;
    } 
    
    // actual code starts here
    uint64_t linesNum1, linesNum2, objNum1, objNum2;
    linesNum1 = countLines(infilename1.c_str());
    linesNum2 = countLines(infilename2.c_str());
    objNum1 = (linesNum1 / LinesPerObj);
    objNum2 = (linesNum2 / LinesPerObj);
    m_objCount = std::min(linesNum1, linesNum2) / LinesPerObj;
    if (!m_no_sort) // we need to sort the files
    {
        // file 1
        string tmp = (boost::format("%1%/1.sorted") % m_tempdir).str();
        uint64_t fileSize = static_cast<uint64_t>(
                                FS::file_size(infilename1.c_str()));
        
        uint64_t bucketSize = (fileSize > m_memLimit) \
                                ? (objNum1 / (fileSize / m_memLimit + 1)) \
                                : objNum1;
        //std::cout << (boost::format("File 1: %1% bytes, %2% lines, %3% objects,  %4% obj per bucket\n") % fileSize % linesNum1 % objNum1 % bucketSize).str();
        ExternalSorter<T, LinesPerObj> sorter(bucketSize, linesNum1);
        sorter(infilename1.c_str(), tmp.c_str());
        infilename1 = std::move(tmp);
        // file 2
        tmp = (boost::format("%1%/2.sorted") % m_tempdir).str();
        fileSize = static_cast<uint64_t>(
                                FS::file_size(infilename2.c_str()));
        bucketSize = (fileSize > m_memLimit) \
                                ? (objNum2 / (fileSize / m_memLimit + 1)) \
                                : objNum2;
        //std::cout <<( boost::format("File 1: %1% bytes, %2% lines, %3% objects,  %4% obj per bucket\n") % fileSize % linesNum2 % objNum2 % bucketSize).str();
        sorter = ExternalSorter<T, LinesPerObj>(bucketSize, linesNum2);
        sorter(infilename2.c_str(), tmp.c_str());
        infilename2 = std::move(tmp);
    }
    
    // filter files
    string outfilename1 = (boost::format("%1%/1.out") % m_tempdir).str();
    string outfilename2 = (boost::format("%1%/2.out") % m_tempdir).str();
    if (m_synched)
    {  // need to write new paired sequence sorter and move default sorter in else scope here
        std:: cerr << "Error: sequence-based approach not implemented yet\n";
        exit(1);
    } else { // default behaviour: hash-based approach
        hashFilterPE(infilename1.c_str(),
                     infilename2.c_str(),
                     outfilename1.c_str(),
                     outfilename2.c_str());
    }

    // gzip outputs if needed else rename
    if (fileHasExt(outfile1.c_str(), ".gz"))
    {
        std::cerr << "gzipped files are not supported yet!\n";
        // gzipfile (outfilename1, outfile1)
        return;
    } else {
        std::rename(outfilename1.c_str(), outfile1.c_str());
    }

    if (fileHasExt(outfile2.c_str(), ".gz"))
    {
        std::cerr << "gzipped files are not supported yet!\n";
        // gzipfile (outfilename2, outfile2) 
        return;
    } else {
        std::rename(outfilename2.c_str(), outfile2.c_str());
    }
}


template<class T, int LinesPerObj>
void DupRemover<T, LinesPerObj>::hashFilterSE(const char* infile,
                                              const char* outfile)
{
    std::ifstream if1{infile};
    std::ofstream of1{outfile};
    check_fstream_ok<std::ifstream>(if1, infile);
    check_fstream_ok<std::ofstream>(of1, outfile);

    T obj;
    hashed_set hashes;
    hashes.reserve(m_objCount);

    std::fstream database;
    string database_name = (boost::format("%1%/database.txt") % m_tempdir).str();
    database.open(database_name, std::ios::in | std::ios::out | std::ios::app);

    while (!if1.eof())
    {
        if1 >> obj >> std::ws;
        setRecord record(obj.seq(),
                         static_cast<long long>(database.tellp()));
        auto it = hashes.find(record);
        if (it == hashes.end())
        {
            of1 << obj << '\n';
            hashes.insert(record);
            database << obj.seq() << " " << -1 << std::endl; // sadly have to flush buffer every time. 
        } else {
            string prevSeq;
            bool found = false;
            long long prevPos = (*it).m_pos;
            while ((prevPos != -1) && (!found))
            {
                database.seekg(static_cast<std::streampos>(prevPos));
                database >> prevSeq >> prevPos;
                if (obj.seq().compare(prevSeq) == 0)
                { found = true; }
            }
            if (!found)
            {
                hashes.erase(it);
                hashes.insert(record);
                database << obj.seq() << " " << prevPos << std::endl;
            }
        }
    }
}

template<class T, int LinesPerObj>
void DupRemover<T, LinesPerObj>::hashFilterPE(const char* infile1,
                                              const char* infile2,
                                              const char* outfile1,
                                              const char* outfile2)
{
    std::ifstream if1{infile1};
    std::ifstream if2{infile2};
    std::ofstream of1{outfile1};
    std::ofstream of2{outfile2};
    check_fstream_ok<std::ifstream>(if1, infile1);
    check_fstream_ok<std::ifstream>(if2, infile2);
    check_fstream_ok<std::ofstream>(of1, outfile1);
    check_fstream_ok<std::ofstream>(of2, outfile2);

    T right, left;
    hashed_set hashes;
    hashes.reserve(m_objCount);

    std::fstream database;
    string database_name = (boost::format("%1%/database.txt") % m_tempdir).str();
    database.open(database_name, std::ios::in | std::ios::out | std::ios::app);

    uint count_pairs = 0, count_unique = 0, count_bad = 0, count_coll = 0;
    while ((!if1.eof()) && (!if2.eof()))
    {
        if1 >> left >> std::ws;
        if2 >> right >> std::ws;
        // while ids dont match -- read from file with lesser id
        // not implemented before I find universal id-defining method
        // TODO!!
        ++count_pairs;
        string curSeq = left.seq() + "|" + right.seq();
        setRecord record(curSeq, 
                         static_cast<long long>(database.tellp()));

        auto it = hashes.find(record);
        if (it == hashes.end())
        {
            of1 << left << '\n';
            of2 << right << '\n';
            hashes.insert(record);
            database << curSeq << " " << -1 << std::endl; // sadly have to flush buffer every time. 
            ++count_unique;
        } else {
            ++count_coll;
            string prevSeq;
            bool found = false;
            long long prevPos = (*it).m_pos;
            while ((prevPos != -1) && (!found))
            {
                database.seekg(static_cast<std::streampos>(prevPos));
                database >> prevSeq >> prevPos;
                if (curSeq.compare(prevSeq) == 0)
                { found = true; }
            }
            if (!found)
            {
                hashes.erase(it);
                hashes.insert(record);
                database << curSeq << " " << prevPos << std::endl;
            } else {
                ++count_bad;
            }
        }
    }
    std::cout << count_pairs << " pairs were read\n";
    std::cout << count_unique << " unique pairs\n";
    std::cout << count_bad << " bad pairs\n";
    std::cout << count_coll << " hash collisions\n";
}