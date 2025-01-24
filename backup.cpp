#include <ctime>
#include <chrono>
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <optional>
#include <iostream>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

std::optional<std::pair<std::string, std::chrono::time_point<fs::file_time_type::clock>>> GetMostRecentFile(const fs::path& folderPath, std::unordered_set<std::string>& destFiles)
{
    std::optional<std::pair<std::string, std::chrono::time_point<fs::file_time_type::clock>>> mostRecentFile = std::nullopt;
    std::chrono::time_point<fs::file_time_type::clock> latestTime = std::chrono::time_point<fs::file_time_type::clock>::min();

    uint64_t failedFiles = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath))
    {
        try
        {
            if (fs::is_regular_file(entry))
            {
                auto lastWriteTime = fs::last_write_time(entry);

                const fs::path relativePath = fs::relative(entry.path(), folderPath);
                destFiles.insert(relativePath.string());

                if (lastWriteTime > latestTime)
                {
                    mostRecentFile = { fs::absolute(entry.path()).lexically_normal().string(), lastWriteTime };
                    latestTime = lastWriteTime;
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            try
            {
                failedFiles++;
                std::cerr << "Error processing file: " << entry.path() << ": " << e.what() << std::endl;
            }
            catch(const std::filesystem::filesystem_error& r)
            {
                std::cerr << "Error processing file, can't print file path: " << e.what() << std::endl;
            }
        }
    }

    std::cout << "Failed to process " << failedFiles << " files" << std::endl;
    return mostRecentFile;
}


bool FolderExists(const fs::path& path)
{
    if (!fs::exists(path) || !fs::is_directory(path))
    {
        std::cerr << "Invalid folder path: " << path << std::endl;
        return false;
    }
    return true;
}


std::string TimestampToString(const std::chrono::time_point<fs::file_time_type::clock>& timestamp)
{
    const auto systemTimePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(timestamp- std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t cftime = std::chrono::system_clock::to_time_t(systemTimePoint);
    return std::asctime(std::localtime(&cftime));
}


bool checkOnly = false; // only show out of date files
int verboseMode = 0; // 0 = not verbose, 1 = print numbers, 2 = print every file, 3 = only copied, 4 = only deleted
void DeleteFiles(const std::unordered_set<std::string> destFiles, const fs::path& dest)
{
    uint64_t failedFiles = 0;
    uint64_t deletedFiles = 0;
    if (verboseMode == 1)
        std::cout << "Deleted files:\n";

    for (const auto& relativePath : destFiles)
    {
        fs::path obsoleteFile = dest / relativePath;
        std::error_code ec;
        if (!checkOnly)
            fs::remove(obsoleteFile, ec);
        if (ec)
        {
            failedFiles++;
            std::cerr << "Failed to delete " << obsoleteFile << ": " << ec.message() << '\n';
        }
        else
        {
            if (verboseMode == 2 || verboseMode == 4)
                std::cout << "Deleted obsolete file: " << obsoleteFile << '\n';

            deletedFiles++;
            if (verboseMode == 1)
                std::cout << deletedFiles << '\r';
        }
    }
    
    if (verboseMode == 1)
        std::cout << std::endl;

    std::cout << "Failed to delete " << failedFiles << " files" << std::endl;
}


void BackupFolder(const std::chrono::time_point<fs::file_time_type::clock>& timestamp, const fs::path& source, const fs::path& dest, std::unordered_set<std::string>& destFiles)
{
    // if source has a file newer than newest file in backup folder, back it up

    uint64_t failedFiles = 0;
    uint64_t copiedFiles = 0;
    if (verboseMode == 1)
        std::cout << "Copied files:\n";

    for (const auto& entry : fs::recursive_directory_iterator(source))
    {
        try
        {
            if (fs::is_regular_file(entry))
            {
                const fs::path relativePath = fs::relative(entry.path(), source);
                fs::path destinationPath = dest / relativePath;

                // Remove from the set, indicating it still exists in the source
                destFiles.erase(relativePath.string());

                auto lastWriteTime = fs::last_write_time(entry);
                if (lastWriteTime > timestamp) // copy file
                {
                    std::error_code ec;
                    if (!checkOnly)
                        fs::create_directories(destinationPath.parent_path(), ec);
                    if (ec)
                    {
                        std::cerr << "Failed to create directories for " << destinationPath.parent_path() << ": " << ec.message() << '\n';
                        continue; // Skip to the next file
                    }

                    try
                    {
                        if (!checkOnly)
                            fs::copy_file(entry.path(), destinationPath, fs::copy_options::overwrite_existing);

                        if (verboseMode == 2 || verboseMode == 3)
                            std::cout << "Copied file " << entry.path() << " to " << destinationPath << '\n';

                        copiedFiles++;
                        if (verboseMode == 1)
                            std::cout << copiedFiles << '\r';
                    }
                    catch(const fs::filesystem_error& e)
                    {
                        failedFiles++;
                        std::cerr << "Failed to copy " << entry.path() << " to " << destinationPath << ": " << e.what() << '\n';
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            try
            {
                failedFiles++;
                std::cerr << "Error backing up file: " << entry.path() << ": " << e.what() << std::endl;
            }
            catch(const std::filesystem::filesystem_error& r)
            {
                std::cerr << "Error backing up file, can't print file path: " << e.what() << std::endl;
            }
        }
    }

    if (verboseMode == 1)
        std::cout << '\n' << std::endl;

    std::cout << "Failed to backup " << failedFiles << " files" << std::endl;
    DeleteFiles(destFiles, dest);
}


bool ParseArgs(int argc, char** argv, fs::path& source, fs::path& dest, std::optional<std::chrono::time_point<fs::file_time_type::clock>>& timestamp)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <source> <destination> <option>" << std::endl;
        std::cout << "       -v | --verbose     Print every copied/deleted file" << std::endl;
        std::cout << "       -c | --copy        Print every copied file" << std::endl;
        std::cout << "       -d | --delete      Print every deleted" << std::endl;
        std::cout << "       -n | --number      Print the number of files copied/deleted to see progress" << std::endl;
        std::cout << "       -t | --time <str>  <str> specifies a time string of the following format 'DD.MM.YYYY HH:MM:SS', to backup all files older than that" << std::endl;
        std::cout << "                          should be used if the program crashed for some reason, as there might be a more recent file backed up by then" << std::endl;
        std::cout << "       --check            Only check how many files are out of date without copying/deleting anything" << std::endl;
        return false;
    }

    source = argv[1];
    dest = argv[2];

    for (int i = 3; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "-v" || arg == "--verbose")
            verboseMode = 2;
        else if (arg == "-n" || arg == "--number")
            verboseMode = 1;
        else if (arg == "-c" || arg == "--copy")
            verboseMode = 3;
        else if (arg == "-d" || arg == "--delete")
            verboseMode = 4;
        else if (arg == "--check")
            checkOnly = true;
        else if (arg == "-t" || arg == "--time")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: Missing time string for " << arg << std::endl;
                return false;
            }

            const std::string timeString = argv[++i];
            std::istringstream timeStream(timeString);

            std::tm tmTime;
            timeStream >> std::get_time(&tmTime, "%d.%m.%Y %H:%M:%S");
            if (timeStream.fail() || tmTime.tm_year < 70)
            {
                std::cerr << "Error: Invalid time format. Expected format: 'DD.MM.YYYY HH:MM:SS', use 4-digit years" << std::endl;
                return false;
            }

            // Explicitly set tm_isdst to -1 so mktime determines DST automatically
            tmTime.tm_isdst = -1;
            const std::time_t timeT = std::mktime(&tmTime);
            if (timeT == -1)
            {
                std::cerr << "Error: Unable to convert time string to time_t" << std::endl;
                return false;
            }

            timestamp = std::chrono::time_point_cast<fs::file_time_type::clock::duration>(std::chrono::system_clock::from_time_t(timeT) - std::chrono::system_clock::now() + fs::file_time_type::clock::now());
            std::cout << "Specified time (small discrepancies are normal): " << TimestampToString(*timestamp) << std::flush;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return false;
        }
    }

    return FolderExists(source) && FolderExists(dest);
}


int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);

    fs::path backupSource, backupDest;
    std::optional<std::chrono::time_point<fs::file_time_type::clock>> customTimestamp = std::nullopt;
    if (!ParseArgs(argc, argv, backupSource, backupDest, customTimestamp))
        return 1;

    std::unordered_set<std::string> destFiles;
    auto mostRecentFile = GetMostRecentFile(backupDest, destFiles);
    if (customTimestamp)
    {
        std::cout << "Starting backup from custom time stamp: " << TimestampToString(*customTimestamp) << std::endl;
        BackupFolder(*customTimestamp, backupSource, backupDest, destFiles);
    }
    else if (mostRecentFile)
    {
        std::cout << "Most recent file in backup destination: " << mostRecentFile->first << ' ' << TimestampToString(mostRecentFile->second) << std::endl; // asctime has \n
        BackupFolder(mostRecentFile->second, backupSource, backupDest, destFiles);
    }
    else
    {
        std::cout << "Starting backup\n" << std::endl;
        BackupFolder(std::chrono::time_point<fs::file_time_type::clock>::min(), backupSource, backupDest, destFiles);
    }
    return 0;
}
