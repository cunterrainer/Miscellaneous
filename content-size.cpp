// Has to be compiled with clang for some reason

#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <Windows.h> // For Windows specific functions

namespace fs = std::filesystem;

// Function to calculate the size of a folder recursively
std::uintmax_t calculateFolderSize(const fs::path& folderPath) {
    std::uintmax_t totalSize = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(folderPath, fs::directory_options::skip_permission_denied)) {
            if (fs::is_regular_file(entry)) {
                totalSize += fs::file_size(entry);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return totalSize;
}

// Function to format file size appropriately
std::string formatFileSize(std::uintmax_t size) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2);
    if (size >= (1ull << 30)) { // If size is >= 1 GB
        stream << std::setw(8) << static_cast<double>(size) / (1 << 30) << " GB";
    } else if (size >= (1ull << 20)) { // If size is >= 1 MB
        stream << std::setw(8) << static_cast<double>(size) / (1 << 20) << " MB";
    } else if (size >= (1ull << 10)) { // If size is >= 1 KB
        stream << std::setw(8) << static_cast<double>(size) / (1 << 10) << " KB";
    } else { // Otherwise, display in bytes
        stream << std::setw(8) << size << " bytes";
    }
    return stream.str();
}

// Function to list files and folders with their sizes
void listFilesAndFolders(const fs::path& directory) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Get console handle
    for (const auto& entry : fs::directory_iterator(directory)) {
        try {
            if (fs::is_regular_file(entry)) {
                std::uintmax_t fileSize = fs::file_size(entry);
                SetConsoleTextAttribute(hConsole, fileSize >= (1ull << 30) ? FOREGROUND_RED | FOREGROUND_INTENSITY : FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
                std::cout << std::fixed << std::setprecision(2)
                          << formatFileSize(fileSize) << " "
                          << entry.path().filename().string() << " (File)\n";
            } else if (fs::is_directory(entry)) {
                std::uintmax_t folderSize = calculateFolderSize(entry.path());
                SetConsoleTextAttribute(hConsole, folderSize >= (1ull << 30) ? FOREGROUND_RED | FOREGROUND_INTENSITY : FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
                std::cout << std::fixed << std::setprecision(2)
                          << formatFileSize(folderSize) << " "
                          << entry.path().filename().string() << " (Folder)\n";
            }
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED); // Reset color to default
        } catch (...) {
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <folder_path>\n";
        return 1;
    }

    fs::path directory(argv[1]);

    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        std::cerr << "Invalid directory path.\n";
        return 1;
    }

    listFilesAndFolders(directory);

    return 0;
}
