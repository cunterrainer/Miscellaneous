#include <string_view>
#include <functional>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <format>
#include <utility>
#include <fstream>

#include "Hash.h"
#include "Profiler.h"

#define WARMUP_RUNS 2  // To warm up the cache and branch predictor

std::vector<std::string> ReadStrings(std::size_t* total_bytes)
{
    std::ifstream in("hash_benchmark_strings.txt");
    if (!in.is_open())
    {
        std::cerr << "Failed to open hash_benchmark_strings.txt" << std::endl;
        return {};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
    {
        *total_bytes += line.size();
        lines.push_back(std::move(line));
    }
    return lines;
}

int main()
{
    std::ios_base::sync_with_stdio(false);
    Profiler::Start();
    size_t total_bytes = 0;
    std::vector<std::pair<double, std::string>> times;
    const std::vector<std::string> strings = ReadStrings(&total_bytes);

    auto Measure = [&](std::string (*hashfunc)(std::string_view str), std::string name)
    {
        for (int i = 0; i < WARMUP_RUNS; ++i)
        {
            for (const std::string& s : strings)
            {
                hashfunc(s);
            }
        }

        Profiler::Reset();
        const int strings_size = strings.size();
        
        Profiler::Start();
        for (int i = 0; i < strings_size; ++i)
        {
            hashfunc(strings[i]);
        }
        Profiler::End();

        const double seconds = Profiler::Total(Profiler::Conversion::Seconds);
        const double gb_per_second = (total_bytes / 1e9) / seconds;
        const double average_time = Profiler::Total(Profiler::Conversion::Nanoseconds) / strings_size;

        times.push_back({average_time, name});
        std::cout << std::format("{:<14}: {:>8.8f} GB/s  Nanoseconds per hash: {:>8.1f}  Execution time: {:>8.3f} ms  Hashes calculated: {}", name, gb_per_second, average_time, Profiler::Total(Profiler::Conversion::Milliseconds), strings_size) << std::endl;
    };

    auto MeasureChar = [&](const char* (*hashfunc)(const char* str), std::string name)
    {
        for (int i = 0; i < WARMUP_RUNS; ++i)
        {
            for (const std::string& s : strings)
            {
                hashfunc(s.c_str());
            }
        }

        Profiler::Reset();
        const int strings_size = strings.size();

        Profiler::Start();
        for (int i = 0; i < strings_size; ++i)
        {
            hashfunc(strings[i].c_str());
        }
        Profiler::End();

        const double seconds = Profiler::Total(Profiler::Conversion::Seconds);
        const double gb_per_second = (total_bytes / 1e9) / seconds;
        const double average_time = Profiler::Total(Profiler::Conversion::Nanoseconds) / strings_size;

        times.push_back({average_time, name});
        std::cout << std::format("{:<14}: {:>8.8f} GB/s  Nanoseconds per hash: {:>8.1f}  Execution time: {:>8.3f} ms  Hashes calculated: {}", name, gb_per_second, average_time, Profiler::Total(Profiler::Conversion::Milliseconds), strings_size) << std::endl;
    };

    Profiler::End();
    std::cout << "Finished generating strings, starting ";
    Profiler::Log(Profiler::Conversion::Seconds);

    Measure(Hash::sha1, "C++ Sha1");
    //Measure(Hash::sha224, "C++ Sha244");
    //Measure(Hash::sha256, "C++ Sha256");
    //Measure(Hash::sha384, "C++ Sha384");
    //Measure(Hash::sha512, "C++ Sha512");
    //Measure(Hash::sha512_224, "C++ Sha512/224");
    //Measure(Hash::sha512_256, "C++ Sha512/256");
    //
    //Measure(Hash::sha3_224, "C++ Sha3-244");
    //Measure(Hash::sha3_256, "C++ Sha3-256");
    //Measure(Hash::sha3_384, "C++ Sha3-384");
    //Measure(Hash::sha3_512, "C++ Sha3-512");
//
    //Measure(Hash::md5, "C++ MD5");
//
//
    MeasureChar(hash_sha1_easy, "C Sha1");
    //MeasureChar(hash_sha224_easy, "C Sha244");
    //MeasureChar(hash_sha256_easy, "C Sha256");
    //MeasureChar(hash_sha384_easy, "C Sha384");
    //MeasureChar(hash_sha512_easy, "C Sha512");
//
    //MeasureChar(hash_sha3_224_easy, "C Sha3-244");
    //MeasureChar(hash_sha3_256_easy, "C Sha3-256");
    //MeasureChar(hash_sha3_384_easy, "C Sha3-384");
    //MeasureChar(hash_sha3_512_easy, "C Sha3-512");
//
    //MeasureChar(hash_md5_easy, "C MD5");

    std::cout << "\nStrings: " << strings.size() << std::endl;

    std::pair<double, std::string> fastest = times[0];
    std::pair<double, std::string> slowest = times[0];
    for (const auto& p : times)
    {
        if (p.first < fastest.first) fastest = p;
        if (p.first > slowest.first) slowest = p;
    }
    
    std::cout << "Fastest: " << fastest.second << " " << fastest.first << std::endl;
    std::cout << "Slowest: " << slowest.second << " " << slowest.first << std::endl;
}