#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <optional>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <unordered_set>

#include "ProgressBar.h"

using namespace std;
namespace fs = filesystem;

struct FileInfo
{
    fs::path path;
    fs::file_time_type timestamp;
    bool isEmpty;
    bool isDirectory;

    bool operator==(const FileInfo& other) const
    {
        return path == other.path;
    }
};

struct FileInfoHash
{
    size_t operator()(const FileInfo& fi) const
    {
        return hash<string>()(fi.path.string());
    }
};

unordered_set<FileInfo, FileInfoHash> GetFilesInFolder(const fs::path& folderPath)
{
    unordered_set<FileInfo, FileInfoHash> files;
    uint64_t failedFiles = 0;

    for (const auto& entry : fs::recursive_directory_iterator(folderPath))
    {
        try
        {
            const bool isDir = fs::is_directory(entry);
            if (fs::is_regular_file(entry) || (isDir && fs::is_empty(entry)))
            {
                const fs::path relativePath = fs::relative(entry.path(), folderPath);
                FileInfo info;
                info.path = relativePath;
                info.timestamp = fs::last_write_time(entry);
                info.isEmpty = fs::is_empty(entry); // can't do empty check befor if-clause because symlinks fail empty check
                info.isDirectory = isDir;
                files.emplace(std::move(info));
            }
        }
        catch (const fs::filesystem_error& e)
        {
            try
            {
                failedFiles++;
                cerr << "Error indexing file: " << fs::absolute(entry.path()) << ": " << e.what() << endl;
            }
            catch(const fs::filesystem_error& r)
            {
                cerr << "Error indexing file, can't print file path: " << e.what() << endl;
            }
        }
    }

    if (failedFiles > 0)
        cout << "Failed to index " << failedFiles << " files" << endl;
    return files;
}


enum class OutputMode
{
    None,
    Verbose,
    CopyOnly,
    DeleteOnly,
    Numbers,
    ProgressBar
};
OutputMode g_OutputMode = OutputMode::None;
bool g_OnlyCheck = false;
bool g_DeleteEmptyFolders = false;
bool g_WaitForThreshold = false;
int64_t g_ThresholdSeconds = 5;

bool FolderExists(const fs::path& path, bool printErrMsg)
{
    try
    {
        if (!fs::exists(path) || !fs::is_directory(path))
        {
            if (printErrMsg)
                std::cerr << "Invalid folder path: " << path << std::endl;
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Failed to check folders existence " << path << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}


bool ParseArgs(int argc, char** argv, fs::path& source, fs::path& dest)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <source> <destination> <options>" << std::endl;
        return false;
    }

    source = argv[1];
    dest = argv[2];

    for (int i = 3; i < argc; ++i)
    {
        std::string arg(argv[i]);
        std::transform(arg.begin(), arg.end(), arg.begin(),[](unsigned char c){ return std::tolower(c); });

        if (arg == "-h" || arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " <source> <destination> <option>" << std::endl;
            std::cout << "       -v | --verbose     Print every copied/deleted file" << std::endl;
            std::cout << "       -c | --copy        Print every copied file" << std::endl;
            std::cout << "       -d | --delete      Print every deleted" << std::endl;
            std::cout << "       -n | --number      Print the number of files copied/deleted" << std::endl;
            std::cout << "       -p | --progress    Show progress bar" << std::endl;
            std::cout << "       -e | --empty       Delete empty folders in backup folder (can take a lot more time for large folders)" << std::endl;
            std::cout << "       -k | --check       Only check how many files are out of date without copying/deleting anything" << std::endl;
            std::cout << "       -s | --seconds     By how many seconds can the timestamps of different files differ" << std::endl;
            std::cout << "                          when copying a file or if different filesystems are involved, timestamps" << std::endl;
            std::cout << "                          of files can differ by a couple seconds. Next parameter is the seconds threshold" << std::endl;
            std::cout << "                          Default: " << g_ThresholdSeconds << " seconds" << std::endl;
            std::cout << "       -w | --wait        Wait for --seconds threshold before showing a done message," << std::endl;
            std::cout << "                          thus the next backup can safely be done with the same threshold" << std::endl;
            return false;
        }
        else if (arg == "-v" || arg == "--verbose")
            g_OutputMode = OutputMode::Verbose;
        else if (arg == "-n" || arg == "--number")
            g_OutputMode = OutputMode::Numbers;
        else if (arg == "-c" || arg == "--copy")
            g_OutputMode = OutputMode::CopyOnly;
        else if (arg == "-d" || arg == "--delete")
            g_OutputMode = OutputMode::DeleteOnly;
        else if (arg == "-p" || arg == "--progress")
            g_OutputMode = OutputMode::ProgressBar;
        else if (arg == "-k" || arg == "--check")
            g_OnlyCheck = true;
        else if (arg == "-e" || arg == "--empty")
            g_DeleteEmptyFolders = true;
        else if (arg == "-w" || arg == "--wait")
            g_WaitForThreshold = true;
        else if (arg == "-s" || arg == "--seconds")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing value for " << arg << std::endl;
                std::cerr << "Try " << argv[0] << ' ' << argv[1] << ' ' << argv[2] << " -s <seconds value>" << std::endl;
                return false;
            }

            char* endPtr = nullptr;
            const char* input = argv[i + 1];
            g_ThresholdSeconds = std::strtoll(input, &endPtr, 10);

            if (endPtr == input || *endPtr != '\0')
            {
                std::cerr << "Error: Invalid value for " << arg << ": " << input << ", expected an integer" << std::endl;
                return false;
            }

            if (g_ThresholdSeconds < 0)
            {
                std::cerr << "Error: Value for " << arg << " must be greater than or equal to 0" << std::endl;
                return false;
            }
            ++i;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            std::cerr << "Try " << argv[0] << " . . --help for additional information" << std::endl;
            return false;
        }
    }

    if (!FolderExists(dest, false))
    {
        try
        {
            fs::create_directories(dest);
        }
        catch(const std::exception& e)
        {
            std::cerr << "Failed to create destination folder " << dest << ": " << e.what() << std::endl;
            return false;
        }
        
    }
    return FolderExists(source, true);
}


void SetLastWriteTime(const fs::path& path, const fs::file_time_type& timestamp, const char* s)
{
    try
    {
        fs::last_write_time(path, timestamp);
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << s << "Failed to set last write time for " << path << ": " << e.what() << std::endl;
    }
}


bool CopyFile(const FileInfo& source, const fs::path& dest)
{
    try
    {
        if (!g_OnlyCheck && !source.isDirectory)
        {
            fs::create_directories(dest.parent_path());
            SetLastWriteTime(dest.parent_path(), fs::last_write_time(source.path.parent_path()), "1: ");
        }
    }
    catch(const fs::filesystem_error& e)
    {
        std::cerr << "Faile to create folder '" << dest.parent_path() << "': " << e.what() << std::endl;
        return false; // skip to next file
    }

    try
    {
        if (!g_OnlyCheck)
        {
            if (source.isDirectory) // is an empty directory
            {
                fs::create_directories(dest);
            }
            else if (source.isEmpty)
            {
                std::ofstream file(dest);
                if (!file)
                {
                    std::cerr << "Failed to create empty file: " << dest << ": " << std::strerror(errno) << std::endl;
                }
            }
            else
            {
                fs::copy_file(source.path, dest, fs::copy_options::overwrite_existing);
            }

            SetLastWriteTime(dest, source.timestamp, "2: ");
        }

        if (g_OutputMode == OutputMode::Verbose || g_OutputMode == OutputMode::CopyOnly)
        {
            std::cout << "Copied file " << source.path << " to " << dest << '\n';
        }
        return true;
    }
    catch(const fs::filesystem_error& e)
    {
        std::cerr << "Failed to copy '" << source.path << "' to '" << dest << "': " << e.what() << std::endl;
        return false;
    }
}


bool DeleteFile(const fs::path& path)
{
    try
    {
        if (!g_OnlyCheck)
            fs::remove(path);
        
        if (g_OutputMode == OutputMode::Verbose || g_OutputMode == OutputMode::DeleteOnly)
        {
            std::cout << "Deleted file: " << path << '\n';
        }
        return true;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Failed to delete file '" << path << "': " << e.what() << '\n';
        return false;
    }
}


void CopyAllFiles(const unordered_set<FileInfo, FileInfoHash>& sourceFiles, const unordered_set<FileInfo, FileInfoHash>& destFiles, const fs::path& srcDir, const fs::path& destDir)
{
    if (g_OutputMode != OutputMode::None)
    {
        ProgressBarInit();
        std::cout << "Copying..." << std::endl;
    }

    uint64_t failedFiles = 0;
    uint64_t copiedFiles = 0;
    uint64_t didntCopyFiles = 0;
    for (const auto& sourcefile : sourceFiles)
    {
        auto it = destFiles.find(sourcefile);
        FileInfo srcInfo;
        srcInfo.path = srcDir / sourcefile.path;
        srcInfo.isEmpty = sourcefile.isEmpty;
        srcInfo.timestamp = sourcefile.timestamp;
        srcInfo.isDirectory = sourcefile.isDirectory;

        bool copiedFile = false;
        if (it != destFiles.end()) // file already exists in backup drive
        {
            const std::chrono::seconds differenceSeconds = std::chrono::duration_cast<std::chrono::seconds>(sourcefile.timestamp - it->timestamp);

            if (std::abs(differenceSeconds.count()) > g_ThresholdSeconds)
            {
                const fs::path destAbsolutePath = destDir / it->path;
                copiedFile = CopyFile(srcInfo, destAbsolutePath);
            }
            else
            {
                didntCopyFiles++;
                copiedFile = false;
                failedFiles--; // we have to increment otherwise we increment failed files despite nothing failing
            }
        }
        else // file doesn't exist in backup drive
        {
            const fs::path destAbsolutePath = destDir / sourcefile.path; // sourcefile.path already is a relative path
            copiedFile = CopyFile(srcInfo, destAbsolutePath);
        }

        if (copiedFile)
        {
            copiedFiles++;
        }
        else
        {
            failedFiles++;
        }

        if (g_OutputMode == OutputMode::Numbers)
        {
            std::cout << "Copying files: " << copiedFiles << '\r';
        }
        else if (g_OutputMode == OutputMode::ProgressBar)
        {
            ProgressBar(copiedFiles+failedFiles+didntCopyFiles, sourceFiles.size());
            std::fflush(stdout);
        }
    }
    std::cout << (g_OutputMode != OutputMode::None && g_OutputMode != OutputMode::ProgressBar ? "\n" : "") << "Copied " << copiedFiles << " files" << std::endl;
    std::cout << "Failed to copy " << failedFiles << " files" << std::endl;
    std::cout << "Didn't copy " << didntCopyFiles << " files, due to them being up to date" << std::endl;
}


void DeleteAllFiles(const unordered_set<FileInfo, FileInfoHash>& sourceFiles, const unordered_set<FileInfo, FileInfoHash>& destFiles, const fs::path& destDir)
{
    if (g_OutputMode != OutputMode::None)
    {
        ProgressBarInit();
        std::cout << "Deleting..." << std::endl;
    }

    uint64_t failedFiles = 0;
    uint64_t deletedFiles = 0;
    uint64_t bufferFilesProgressBar = 0;
    for (const auto& destfile : destFiles)
    {
        if (sourceFiles.find(destfile) == sourceFiles.end()) // didn't find 
        {
            const fs::path destAbsolutePath = destDir / destfile.path;
            if (DeleteFile(destAbsolutePath))
                deletedFiles++;
            else
                failedFiles++;
        }
        else
        {
            bufferFilesProgressBar++;
        }

        if (g_OutputMode == OutputMode::Numbers)
        {
            std::cout << "Deleting files: " << deletedFiles << '\r';
        }
        else if (g_OutputMode == OutputMode::ProgressBar)
        {
            ProgressBar(deletedFiles+bufferFilesProgressBar, destFiles.size());
            std::fflush(stdout);
        }
    }
    std::cout << (g_OutputMode != OutputMode::None && g_OutputMode != OutputMode::ProgressBar ? "\n" : "") << "Deleted " << deletedFiles << " files" << std::endl;
    std::cout << "Failed to delete " << failedFiles << " files" << std::endl;
}


vector<fs::path> GetEmptyFolders(const fs::path& path)
{
    std::vector<fs::path> foldersToDelete;
    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        if (fs::is_directory(entry) && fs::is_empty(entry))
        {
            foldersToDelete.push_back(entry);
        }
    }
    return foldersToDelete;
}


void DeleteEmptyFolders(const fs::path& dest)
{
    if (!g_DeleteEmptyFolders || g_OnlyCheck)
        return;
    cout << "Deleting empty folders..." << endl;

    uint64_t failedFolders = 0;
    uint64_t deletedFolders = 0;

    vector<fs::path> foldersToDelete = GetEmptyFolders(dest);
    while (!foldersToDelete.empty())
    {
        for (const auto& entry : foldersToDelete)
        {
            try
            {
                fs::remove(entry);
                ++deletedFolders;

                if (g_OutputMode == OutputMode::Numbers || g_OutputMode == OutputMode::ProgressBar)
                {
                    cout << "Deleting empty folders: " << deletedFolders << '\r' << flush;
                }
                else if (g_OutputMode == OutputMode::Verbose)
                {
                    cout << "Deleting empty folder: " << entry << endl;
                }
            }
            catch(const fs::filesystem_error& e)
            {
                failedFolders++;
                try
                {
                    cerr << "Error deleting empty folder: " << entry << ": " << e.what() << endl;
                }
                catch(const fs::filesystem_error& r)
                {
                    cerr << "Error deleting empty folder, can't print file path: " << e.what() << endl;
                }
            }
            
        }
        foldersToDelete = GetEmptyFolders(dest);
    }
    
    std::cout << (g_OutputMode != OutputMode::None && g_OutputMode != OutputMode::ProgressBar ? "\n" : "") << "Deleted " << deletedFolders << " empty folders" << std::endl;
    std::cout << "Failed to delete " << failedFolders << " empty folders" << std::endl;
}


void PrintInfo(const fs::path& src, const fs::path& dest)
{
    std::cout << "------------------------------------------------------------" << std::endl;
    std::cout << "Info: " << std::endl;
    std::cout << "Check only: " << std::boolalpha << g_OnlyCheck << std::endl;
    std::cout << "Src directory: " << src << std::endl;
    std::cout << "Dst directory: " << dest << std::endl;
    std::cout << "Seconds threshold: " << g_ThresholdSeconds << std::endl;
    std::cout << "Delete empty folders: " << std::boolalpha << g_DeleteEmptyFolders << std::endl;
    std::cout << "Wait for " << g_ThresholdSeconds << " seconds after finishing: " << std::boolalpha << g_WaitForThreshold << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl << std::endl;
}


int main(int argc, char** argv)
{
    fs::path sourceDir, destDir;
    if (!ParseArgs(argc, argv, sourceDir, destDir))
        return 0;
    PrintInfo(sourceDir, destDir);

    unordered_set<FileInfo, FileInfoHash> sourceFiles = GetFilesInFolder(sourceDir);
    std::cout << "Finished indexing source directory: " << sourceDir << std::endl;
    unordered_set<FileInfo, FileInfoHash> destFiles = GetFilesInFolder(destDir);
    std::cout << "Finished indexing destination directory: " << destDir << std::endl;
    std::cout << "Starting backup" << std::endl;

    CopyAllFiles(sourceFiles, destFiles, sourceDir, destDir);
    DeleteAllFiles(sourceFiles, destFiles, destDir);
    DeleteEmptyFolders(destDir);

    if (!g_OnlyCheck && g_WaitForThreshold)
    {
        std::cout << "Waiting for " << g_ThresholdSeconds << " seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(g_ThresholdSeconds));
    }
    std::cout << "Finished backup" << std::endl;
    return 0;
}
