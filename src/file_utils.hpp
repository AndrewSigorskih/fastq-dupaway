#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

namespace FS = std::filesystem;

const int DIRNAME_LEN = 10;

void generate_random_name(char* buf, int len);
void create_random_dir(char* buf, int len, uint n_tries = 100);
void create_random_file(char* buf, int len, uint n_tries = 100);
uint64_t countLines(const char* infilename);
bool fileHasExt(const char* filename, const char* ext=".gz");

template<class filehandle>
void check_fstream_ok(filehandle& stream, const char* filename)
{
    if (!stream)
    {
        // TODO change to exception
        std::cerr << "Error: cannot open file " << filename <<", exiting.\n";
        exit(1);
    }
}
