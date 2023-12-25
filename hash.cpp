#define _CRT_SECURE_NO_WARNINGS // strerror, fopen
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <string>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <unordered_map>

#include "Hash.h"


inline void lower_string(std::string& str) noexcept
{
    for (char& c : str)
    {
        c = std::tolower(c);
    }
}


constexpr const char* upper_string(char* str) noexcept
{
    while(*str)
    {
        *str = std::toupper(*str);
        str++;
    }
    return str;
}


void print_help(const char* path)
{
    printf("Usage: %s [input] [function] [options]\nOptions:\n", path);
    printf("        -i  | --input           Force the next argument to be the input (e.g. to get the hash of '-h')\n");
    printf("        -s  | --search          Print files with the same hash value\n");
    printf("        -m  | --multiple        Print files that are the same in directory\n");
    printf("        -h  | --help            Show this info message\n");
    printf("        -t  | --text            Treat input as text\n");
    printf("        -f  | --file            Treat input as file\n");
    printf("        -d  | --directory       Treat input as directory\n");
    printf("        -u  | --upper           Print hash upper case\n");
    printf("        -c  | --conceal         Don't show the current file when matching/searching in directories\n");
    printf("        -dh | --directory-hash  Print a hash value of all hash values of all files in the directory combined\n");
    printf("        -nt | --no-threads      Don't use multiple threads when hashing folders\n");
    printf("        -nd | --no-decorator    Just print the hash without information and new line (e.g. for piping)\n");
    printf("              --threads         Next argument is the max number of threads allowed to use (actuall size can be less)\n");
    printf("Supported functions are: md5, sha1, sha224, sha256, sha384, sha512, sha512-224, sha512-256, sha3-224, sha3-256, sha3-384, sha3-512\n");
    printf("If neither the '-f', '-t' nor '-d' options are specified, the program will try to hash a directory, then a file; otherwise, it will be treated as a string.\n");
}


struct Settings
{
    enum class HashFunction
    {
        MD5,
        Sha1,
        Sha224,
        Sha256,
        Sha384,
        Sha512,
        Sha512_224,
        Sha512_256,
        Sha3_224,
        Sha3_256,
        Sha3_384,
        Sha3_512
    };

    std::string input;
    std::string search_hash;
    bool valid = true;
    bool conceal = false;
    bool decorator = true;
    bool force_dir = false;
    bool force_file = false;
    bool force_text = false;
    bool upper_case = false;
    bool print_same = false;
    bool directory_hash = false;
    bool disable_threads = false;
    size_t max_num_of_threads = 0;
    HashFunction hash_func = HashFunction::Sha256;
};


inline std::unordered_map<std::string, Settings::HashFunction> generate_hash_map()
{
    return {
        { "md5", Settings::HashFunction::MD5 },
        { "sha1", Settings::HashFunction::Sha1 },
        { "sha224", Settings::HashFunction::Sha224 },
        { "sha256", Settings::HashFunction::Sha256 },
        { "sha384", Settings::HashFunction::Sha384 },
        { "sha512", Settings::HashFunction::Sha512 },
        { "sha512-224", Settings::HashFunction::Sha512_224 },
        { "sha512-256", Settings::HashFunction::Sha512_256 },
        { "sha3-224", Settings::HashFunction::Sha3_224 },
        { "sha3-256", Settings::HashFunction::Sha3_256 },
        { "sha3-384", Settings::HashFunction::Sha3_384 },
        { "sha3-512", Settings::HashFunction::Sha3_512 }
    };
}


constexpr const char* get_hash_function_name(Settings::HashFunction func)
{
    switch (func)
    {
        case Settings::HashFunction::MD5:        return "MD5";
        case Settings::HashFunction::Sha1:       return "Sha1";   
        case Settings::HashFunction::Sha224:     return "Sha224";
        case Settings::HashFunction::Sha256:     return "Sha256";
        case Settings::HashFunction::Sha384:     return "Sha384";
        case Settings::HashFunction::Sha512:     return "Sha512";
        case Settings::HashFunction::Sha512_224: return "Sha512-224";
        case Settings::HashFunction::Sha512_256: return "Sha512-256";
        case Settings::HashFunction::Sha3_224:   return "Sha3-224";
        case Settings::HashFunction::Sha3_256:   return "Sha3-256";
        case Settings::HashFunction::Sha3_384:   return "Sha3-384";
        case Settings::HashFunction::Sha3_512:   return "Sha3-512";
    }
    return "";
}


constexpr size_t get_hash_size(Settings::HashFunction func) noexcept
{
    switch (func)
    {
        case Settings::HashFunction::MD5:        return HASH_MD5_SIZE;
        case Settings::HashFunction::Sha1:       return HASH_SHA1_SIZE;
        case Settings::HashFunction::Sha224:     return HASH_SHA224_SIZE;
        case Settings::HashFunction::Sha256:     return HASH_SHA256_SIZE;
        case Settings::HashFunction::Sha384:     return HASH_SHA384_SIZE;
        case Settings::HashFunction::Sha512:     return HASH_SHA512_SIZE;
        case Settings::HashFunction::Sha512_224: return HASH_SHA224_SIZE;
        case Settings::HashFunction::Sha512_256: return HASH_SHA256_SIZE;
        case Settings::HashFunction::Sha3_224:   return HASH_SHA3_224_SIZE;
        case Settings::HashFunction::Sha3_256:   return HASH_SHA3_256_SIZE;
        case Settings::HashFunction::Sha3_384:   return HASH_SHA3_384_SIZE;
        case Settings::HashFunction::Sha3_512:   return HASH_SHA3_512_SIZE;
    }
    return 0;
}


Settings parse_args(int argc, const char** argv)
{
    Settings settings;
    const std::unordered_map<std::string, Settings::HashFunction> hash_func_map = generate_hash_map();

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        lower_string(arg);

        const std::unordered_map<std::string, Settings::HashFunction>::const_iterator it = hash_func_map.find(arg);

        if (it != hash_func_map.end())
        {
            settings.hash_func = it->second;
        }
        else if (arg == "-u" || arg == "--upper")
        {
            settings.upper_case = true;
        }
        else if (arg == "-f" || arg == "--file")
        {
            settings.force_file = true;
            settings.force_text = false;
            settings.force_dir = false;
        }
        else if (arg == "-t" || arg == "--text")
        {
            settings.force_text = true;
            settings.force_file = false;
            settings.force_dir = false;
        }
        else if (arg == "-d" || arg == "--directory")
        {
            settings.force_dir = true;
            settings.force_file = false;
            settings.force_text = false;
        }
        else if (arg == "-h" || arg == "--help")
        {
            settings.valid = false;
            print_help(argv[0]);
            return settings;
        }
        else if (arg == "-m" || arg == "--multiple")
        {
            settings.print_same = true;
        }
        else if (arg == "-c" || arg == "--conceal")
        {
            settings.conceal = true;
        }
        else if (arg == "-nd" || arg == " --no-decorator")
        {
            settings.decorator = false;
        }
        else if (arg == "-nt" || arg == " --no-threads")
        {
            settings.disable_threads = true;
        }
        else if (arg == "-dh" || arg == "--directory-hash")
        {
            settings.force_dir = true;
            settings.print_same = true;
            settings.directory_hash = true;
        }
        else if (arg == "--threads")
        {
            if (i + 1 == argc)
            {
                settings.valid = false;
                fprintf(stderr, "Missing number of threads after '%s'\nTry '--help' for additional information\n", argv[i]);
                return settings;
            }

            char* end;
            const long num = strtol(argv[i+1], &end, 10);
            if (end == argv[i+1] || num <= 0)
            {
                fprintf(stderr, "Not a valid, positiv integer number for threads '%s'\nTry '--help' for additional information\n", argv[i+1]);
                settings.valid = false;
                return settings;
            }
            settings.max_num_of_threads = (size_t)num;
            ++i;
        }
        else if (arg == "-s" || arg == "--search")
        {
            if (i + 1 == argc)
            {
                settings.valid = false;
                fprintf(stderr, "Missing hash after '%s'\nTry '--help' for additional information\n", argv[i]);
                return settings;
            }
            settings.search_hash = argv[i+1];
            lower_string(settings.search_hash);
            i++;
        }
        else if (arg == "-i" || arg == "--input")
        {
            if (i + 1 == argc)
            {
                settings.valid = false;
                fprintf(stderr, "Missing input after '%s'\nTry '--help' for additional information\n", argv[i]);
                return settings;
            }
            settings.input = argv[i+1];
            i++;
        }
        else
        {
            if (!settings.input.empty())
            {
                fprintf(stderr, "Previous input: %s\nNew input: %s\nWas this change intentional?\nTry '--help' for additional information\n\n", settings.input.c_str(), argv[i]);
            }
            settings.input = argv[i];
        }
    }

    if (!settings.search_hash.empty() || settings.print_same)
    {
        settings.force_dir = true;
        settings.force_file = false;
        settings.force_text = false;
    }

    if (!settings.search_hash.empty() && settings.search_hash.size() != get_hash_size(settings.hash_func))
    {
        settings.valid = false;
        fprintf(stderr, "Search hash is not a valid %s hash, expected size: %zu, hash size: %zu\n", get_hash_function_name(settings.hash_func), get_hash_size(settings.hash_func), settings.search_hash.size());
    }

    settings.force_text = settings.force_text || settings.input.empty();
    return settings;
}


std::mutex g_MutexPrintThread;
void print_thread(FILE* f, const char* const fmt, ...)
{
    std::scoped_lock lock{g_MutexPrintThread};
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
}


int print_hash(const char* dec1, const char* dec2, const char* hash, bool decorator, bool upper_case, bool folder = false)
{
    if (upper_case)
    {
        // const cast is allowed because the internal buffer is not created using const
        upper_string(const_cast<char*>(hash));
    }

    std::scoped_lock lock{g_MutexPrintThread};
    if (decorator)
    {
        if (folder)
            printf("%s%s%s\n", dec1, hash, dec2);
        else
            printf("%s%s%s\n", dec1, dec2, hash);
    }
    else
    {
        printf("%s", hash);
    }
    return 0;
}


int hash_string(std::string_view str, Settings::HashFunction func, bool decorator, bool upper_case)
{
    switch (func)
    {
        case Settings::HashFunction::MD5:        return print_hash("[Text] ", "MD5: ", hash_md5_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha1:       return print_hash("[Text] ", "Sha1: ", hash_sha1_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha224:     return print_hash("[Text] ", "Sha224: ", hash_sha224_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha256:     return print_hash("[Text] ", "Sha256: ", hash_sha256_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha384:     return print_hash("[Text] ", "Sha384: ", hash_sha384_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha512:     return print_hash("[Text] ", "Sha512: ", hash_sha512_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha512_224: return print_hash("[Text] ", "Sha512-224: ", hash_sha512t_binary(224, str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha512_256: return print_hash("[Text] ", "Sha512-256: ", hash_sha512t_binary(256, str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha3_224:   return print_hash("[Text] ", "Sha3-224: ", hash_sha3_224_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha3_256:   return print_hash("[Text] ", "Sha3-256: ", hash_sha3_256_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha3_384:   return print_hash("[Text] ", "Sha3-384: ", hash_sha3_384_binary(str.data(), str.size(), NULL), decorator, upper_case);
        case Settings::HashFunction::Sha3_512:   return print_hash("[Text] ", "Sha3-512: ", hash_sha3_512_binary(str.data(), str.size(), NULL), decorator, upper_case);
    }
    return 0;
}


std::string hash_file(const char* path, Settings::HashFunction func)
{
    switch (func)
    {
        case Settings::HashFunction::MD5:        return Hash::File::HashFile<Hash::MD5>(path);
        case Settings::HashFunction::Sha1:       return Hash::File::HashFile<Hash::Sha1>(path);
        case Settings::HashFunction::Sha224:     return Hash::File::HashFile<Hash::Sha224>(path);
        case Settings::HashFunction::Sha256:     return Hash::File::HashFile<Hash::Sha256>(path);
        case Settings::HashFunction::Sha384:     return Hash::File::HashFile<Hash::Sha384>(path);
        case Settings::HashFunction::Sha512:     return Hash::File::HashFile<Hash::Sha512>(path);
        case Settings::HashFunction::Sha512_224: return Hash::File::HashFile<Hash::Sha512_T<224>>(path);
        case Settings::HashFunction::Sha512_256: return Hash::File::HashFile<Hash::Sha512_T<256>>(path);
        case Settings::HashFunction::Sha3_224:   return Hash::File::HashFile<Hash::Sha3_224>(path);
        case Settings::HashFunction::Sha3_256:   return Hash::File::HashFile<Hash::Sha3_256>(path);
        case Settings::HashFunction::Sha3_384:   return Hash::File::HashFile<Hash::Sha3_384>(path);
        case Settings::HashFunction::Sha3_512:   return Hash::File::HashFile<Hash::Sha3_512>(path);
    }
    return "";
}


size_t g_PrevPathLength = 0;
std::map<std::string, std::vector<std::string>> g_HashFilesMap;
std::unordered_map<std::string, std::vector<std::string>> g_HashFilesUnorderedMap;
template <typename T> void hash_directory(const std::string& path, Settings::HashFunction func, bool decorator, bool upper_case, const std::string& search, bool multiple, bool conceal, bool directory_hash)
{
    for (const auto& entry : T(path, std::filesystem::directory_options::skip_permission_denied))
    {
        try
        {
            if (std::filesystem::is_regular_file(entry.path()))
            {
                std::string hash = hash_file(entry.path().string().c_str(), func);
                if (hash.empty())
                {
                    throw std::filesystem::filesystem_error(strerror(errno), entry.path(), std::error_code());
                }

                if (hash == search || (multiple && search.empty()))
                {
                    static std::mutex mutex;
                    std::scoped_lock lock{mutex};
                    if (decorator && !conceal)
                    {
                        print_thread(stdout, "File: %-*s\r", static_cast<int>(g_PrevPathLength), entry.path().string().c_str());
                        g_PrevPathLength = entry.path().string().size();
                    }

                    if (directory_hash)
                    {
                        auto it = g_HashFilesMap.find(hash);
                        if (it != g_HashFilesMap.end())
                        {
                            it->second.push_back(entry.path().string());
                        }
                        else
                        {
                            g_HashFilesMap[hash] = std::vector<std::string>({ entry.path().string() });
                        }
                    }
                    else
                    {
                        auto it = g_HashFilesUnorderedMap.find(hash);
                        if (it != g_HashFilesUnorderedMap.end())
                        {
                            it->second.push_back(entry.path().string());
                        }
                        else
                        {
                            g_HashFilesUnorderedMap[hash] = std::vector<std::string>({ entry.path().string() });
                        }
                    }
                }
                else if (search.empty())
                {
                    print_hash(std::string(std::string(get_hash_function_name(func)) + ": ").c_str(), std::string(" [" + entry.path().string() + "]").c_str(), hash.c_str(), decorator, upper_case, true);
                }
            }
        }
        catch(const std::filesystem::filesystem_error& e)
        {
            print_thread(stderr, "Failed to open file '%s': %s\n", e.path1().string().c_str(), e.what());
        }
    }
}


std::mutex g_MutexDirectories;
std::queue<std::pair<std::string, bool>> g_Directories; // root is true
void hash_directory_thread_func(Settings::HashFunction func, bool decorator, bool upper_case, const std::string& search, bool multiple, bool conceal, bool directory_hash)
{
    while (true)
    {
        std::pair<std::string, bool> path;
        {
            std::scoped_lock lock{g_MutexDirectories};
            if (!g_Directories.empty())
            {
                path = g_Directories.front();
                g_Directories.pop();
            }
            else
            {
                break;
            }
        }
        if (path.second) // is root
        {
            hash_directory<std::filesystem::directory_iterator>(path.first, func, decorator, upper_case, search, multiple, conceal, directory_hash);
        }
        else
        {
            hash_directory<std::filesystem::recursive_directory_iterator>(path.first, func, decorator, upper_case, search, multiple, conceal, directory_hash);
        }
    }
}


void hash_directory_threads(Settings::HashFunction func, bool decorator, bool upper_case, const std::string& search, bool multiple, bool conceal, bool directory_hash, size_t max_num_of_threads)
{
    std::vector<std::thread> hash_threads;
    const size_t num_of_threads = max_num_of_threads == 0 ? (std::min((size_t)std::thread::hardware_concurrency(), g_Directories.size()) == 0 ? std::min(g_Directories.size(), (size_t)6) : std::min((size_t)std::thread::hardware_concurrency(), g_Directories.size()) - 1) : max_num_of_threads;
    for (size_t i = 0; i < num_of_threads; ++i)
    {
        hash_threads.push_back(std::thread(hash_directory_thread_func, func, decorator, upper_case, search, multiple, conceal, directory_hash));
    }
    hash_directory_thread_func(func, decorator, upper_case, search, multiple, conceal, directory_hash);

    for (auto& th : hash_threads)
        th.join();
}


void hash_directory_setup(const std::string& path, Settings::HashFunction func, bool decorator, bool upper_case, const std::string& search, bool multiple, bool conceal, bool disable_threads, bool directory_hash, size_t max_num_of_threads)
{
    if (disable_threads)
    {
        hash_directory<std::filesystem::recursive_directory_iterator>(path, func, decorator, upper_case, search, multiple, conceal, directory_hash);
    }
    else
    {
        g_Directories.push({ path, true });
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (std::filesystem::is_directory(entry.path()))
                {
                    g_Directories.push({ entry.path().string(), false });
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            fprintf(stderr, "Failed to add folder '%s' to thread pool: %s\n", e.path1().string().c_str(), e.what());
        }
        hash_directory_threads(func, decorator, upper_case, search, multiple, conceal, directory_hash, max_num_of_threads);
    }


    if (directory_hash)
    {
        std::string all_hashes;
        for (const auto& pair : g_HashFilesMap)
            all_hashes += pair.first;

        switch (func)
        {
            case Settings::HashFunction::MD5:        print_hash(std::string("[" + path + "] ").c_str(), "MD5: ", hash_md5_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha1:       print_hash(std::string("[" + path + "] ").c_str(), "Sha1: ", hash_sha1_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha224:     print_hash(std::string("[" + path + "] ").c_str(), "Sha224: ", hash_sha224_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha256:     print_hash(std::string("[" + path + "] ").c_str(), "Sha256: ", hash_sha256_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha384:     print_hash(std::string("[" + path + "] ").c_str(), "Sha384: ", hash_sha384_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha512:     print_hash(std::string("[" + path + "] ").c_str(), "Sha512: ", hash_sha512_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha512_224: print_hash(std::string("[" + path + "] ").c_str(), "Sha512-224: ", hash_sha512t_binary(224, all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha512_256: print_hash(std::string("[" + path + "] ").c_str(), "Sha512-256: ", hash_sha512t_binary(256, all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha3_224:   print_hash(std::string("[" + path + "] ").c_str(), "Sha3-224: ", hash_sha3_224_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha3_256:   print_hash(std::string("[" + path + "] ").c_str(), "Sha3-256: ", hash_sha3_256_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha3_384:   print_hash(std::string("[" + path + "] ").c_str(), "Sha3-384: ", hash_sha3_384_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
            case Settings::HashFunction::Sha3_512:   print_hash(std::string("[" + path + "] ").c_str(), "Sha3-512: ", hash_sha3_512_binary(all_hashes.data(), all_hashes.size(), NULL), decorator, upper_case); return;
        }
        return;
    }


    size_t matches = 0;
    size_t identical = 0;
    for (const auto& pair : g_HashFilesUnorderedMap)
    {
        if (!search.empty() || pair.second.size() > 1)
        {
            matches++;
            if (decorator)
                printf("%s: %-*s\n", get_hash_function_name(func), static_cast<int>(g_PrevPathLength), pair.first.c_str());
            for (const auto& s : pair.second)
            {
                ++identical;
                printf("%s\n", s.c_str());
            }
            if (decorator)
                puts("");
        }
    }

    if (!decorator || conceal)
        return;

    if (multiple)
    {
        printf("Matches: %-*zu\nSearched: %zu\nIdentical: %zu\n", static_cast<int>(g_PrevPathLength), matches, g_HashFilesUnorderedMap.size()+identical-matches, identical);
    }
    if (!search.empty())
    {
        printf("Found: %-*zu\n", static_cast<int>(g_PrevPathLength), matches);
    }
}


int main(int argc, const char** argv)
{
    const Settings settings = parse_args(argc, argv);
    if (!settings.valid)
        return 0;

    if (settings.force_dir)
    {
        try
        {
            hash_directory_setup(settings.input, settings.hash_func, settings.decorator, settings.upper_case, settings.search_hash, settings.print_same, settings.conceal, settings.disable_threads, settings.directory_hash, settings.max_num_of_threads);
            return 0;
        }
        catch(const std::filesystem::filesystem_error& e)
        {
            fprintf(stderr, "Failed to iterate directory '%s': %s\n", settings.input.c_str(), e.what());
            return 0;
        }
    }
    else if (settings.force_file)
    {
        const std::string hash = hash_file(settings.input.c_str(), settings.hash_func);

        if (hash.empty())
        {
            fprintf(stderr, "Failed to open file '%s': %s\n", settings.input.c_str(), strerror(errno));
            return 0;
        }

        return print_hash(std::string("[" + settings.input + "] ").c_str(), std::string(std::string(get_hash_function_name(settings.hash_func)) + ": ").c_str(), hash.c_str(), settings.decorator, settings.upper_case);
    }
    else if (settings.force_text)
    {
        hash_string(settings.input, settings.hash_func, settings.decorator, settings.upper_case);
        return 0;
    }


    if (std::filesystem::is_directory(settings.input))
    {
        try
        {
            hash_directory_setup(settings.input, settings.hash_func, settings.decorator, settings.upper_case, "", false, true, settings.disable_threads, false, settings.max_num_of_threads);
            return 0;
        }
        catch(const std::filesystem::filesystem_error&)
        {
        }
    }

    if (std::filesystem::exists(settings.input))
    {
        const std::string hash = hash_file(settings.input.c_str(), settings.hash_func);

        if (!hash.empty())
        {
            return print_hash(std::string("[" + settings.input + "] ").c_str(), std::string(std::string(get_hash_function_name(settings.hash_func)) + ": ").c_str(), hash.c_str(), settings.decorator, settings.upper_case);
        }
    }
    
    hash_string(settings.input, settings.hash_func, settings.decorator, settings.upper_case);
}