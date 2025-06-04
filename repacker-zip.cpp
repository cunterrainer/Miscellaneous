#include <string>
#include <vector>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <utility>
#include <optional>
#include <iostream>
#include <filesystem>
#include <system_error>

#include "zip_file.h"

#include "Hash.h"
namespace fs = std::filesystem;

std::string SystemClockMillis()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
    return std::to_string(ms);
}

std::optional<fs::path> CreateBuildDir()
{
    const fs::path buildDir = "cmp-" + SystemClockMillis();
    std::error_code ec;

    if (!fs::create_directories(buildDir, ec))
    {
        if (ec)
        {
            std::cerr << "Failed to create build directory: " << ec.message() << std::endl;
            return std::nullopt;
        } // else folder already exists, just override
    }
    return buildDir;
}

void DeleteDir(const fs::path& path)
{
    try
    {
        uintmax_t removed = fs::remove_all(path);  // Remove all contents and the folder itself
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Failed to delete folder: " << path << std::endl;
        std::cerr << "Reason: " << e.what() << std::endl;
    }
}

void WriteFile(const fs::path& path, const unsigned char* data, size_t len)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
    {
        std::cerr << "Failed to write file: " << path << std::endl;
        std::cerr << "Reason: " << strerror(errno) << std::endl;
        return;
    }

    out.write(reinterpret_cast<const char*>(data), len);
    out.close();
}

std::pair<std::vector<unsigned char>, size_t> ZipFiles(const fs::path& exePath)
{
    std::error_code ec;
    const fs::path& exeParentDir = exePath.parent_path() == "" ? "." : exePath.parent_path();
    const size_t d = std::distance(fs::recursive_directory_iterator(exeParentDir, ec), {}); // Number of files to zip

    if (ec)
    {
        std::cerr << "Failed to get iterator for: " << exeParentDir << ", Reason: " << ec.message() << std::endl;
        return {};
    }
    
    miniz_cpp::zip_file file;
    std::stringstream repackedInfo;

    uint32_t i = 1;
    size_t unzippedFileSize = 0;
    for (const auto& f : fs::recursive_directory_iterator(exeParentDir, ec))
    {
        if (ec)
        {
            std::cerr << "Failed get iterator for: " << f << ", Reason: " << ec.message() << std::endl;
            continue;
        }

        std::cout << "Zipping file: " << i++ << " of " << d << '\r';
        if (fs::is_regular_file(f))
        {
            unzippedFileSize += f.file_size();
            const std::string md5 = Hash::File::HashFile<Hash::MD5>(f.path().string());

            file.write(f.path().string(), "F" + md5);
            repackedInfo << "F" << md5 << ' ' << fs::relative(f, exeParentDir).string() << '\n';
        }
        else if (fs::is_directory(f) && fs::is_empty(f))
        {
            repackedInfo << "D00000000000000000000000000000000" << ' ' << fs::relative(f, exeParentDir).string() << '\n';
        }
    }
    std::cout << '\n';
    
    
    file.writestr("repacked-info.txt", repackedInfo.str());
    std::vector<unsigned char> zipFile;
    file.save(zipFile);
    return { zipFile, unzippedFileSize };
}


void GenerateHeader(std::ofstream& main, const std::vector<unsigned char>& archive)
{
    main << "#define _CRT_SECURE_NO_WARNINGS\n";
    main << "#include <Windows.h>\n\n";
    main << "#include <chrono>\n";
    main << "#include <vector>\n";
    main << "#include <string>\n";
    main << "#include <cerrno>\n";
    main << "#include <cstdint>\n";
    main << "#include <cstdlib>\n";
    main << "#include <fstream>\n";
    main << "#include <sstream>\n";
    main << "#include <cstring>\n";
    main << "#include <iostream>\n";
    main << "#include <filesystem>\n";
    main << "#include <system_error>\n";
    main << "\n#include \"zip_file.h\"\n\n";
    main << "namespace fs = std::filesystem;\n\n";

    main << "std::vector<unsigned char> g_ZipArchiveData =\n{{";

    for (size_t i = 0; i < archive.size(); ++i)
    {
        //if (i % 64 == 0)
        //main << "\n\t";
        //main << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(archive[i]) << ", ";
        main << "0x" << std::hex << std::uppercase << static_cast<int>(archive[i]) << ",";
    }
    main << "\n}};\n";
}


void GenerateUtilities(std::ofstream& main, bool verbose)
{
    if (verbose)
        main << "const bool g_Verbose = true;\n\n";
    else
        main << "const bool g_Verbose = false;\n\n";
        
    main << R"(
        std::string SystemClockMillis()
        {
            using namespace std::chrono;
            const auto now = system_clock::now();
            const auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
            return std::to_string(ms);
        }

        void CreateDirectories(const fs::path& path)
        {
            std::error_code ec;
            if (!fs::create_directories(path, ec))
            {
                if (ec)
                {
                    std::cerr << "[Repacker-Wrapper] Failed to create directory: " << path << " " << ec.message() << std::endl;
                    std::cerr << "[Repacker-Wrapper] Programm terminates" << std::endl;
                    std::exit(-1);
                } // else folder already exists, just override
            }
        }

        fs::path GetTempDir()
        {
            const fs::path temp = fs::temp_directory_path() / ("repacker-" + SystemClockMillis());
            CreateDirectories(temp);
            return temp;
        }

        void ExtractZipFile(const fs::path& temp)
        {
            miniz_cpp::zip_file file(g_ZipArchiveData);

            std::istringstream infoFile(file.read("repacked-info.txt"));
            std::string line;

            while(std::getline(infoFile, line))
            {
                if (line.empty()) continue;

                const std::string hashedPath = line.substr(0, 32+1);
                const std::string realPath = line.substr(32+2, line.size() - hashedPath.size());

                if (hashedPath[0] == 'D') // empty directory
                {
                    fs::create_directories(temp / realPath);
                }
                else
                {
                    fs::create_directories(temp / fs::path(realPath).parent_path());

                    if (g_Verbose)
                        std::cout << "Extracting:" << hashedPath << " to " << realPath << '\r';
                    file.extract(hashedPath, fs::path(temp / realPath).string());
                }
            }
        }

        void DeleteTempDir(const fs::path& path)
        {
            std::error_code ec;
            const uintmax_t removed = fs::remove_all(path, ec);  // Remove all contents and the folder itself
            // be silent on error, nothing can't show output because console no longer exists, just catch the error so it doesn't crash
        }

        void PrintLastError()
        {
            const DWORD error = GetLastError();
            
            // Allocate a buffer to hold the formatted message
            LPVOID msgBuffer;

            // Use FormatMessage to get the error message
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&msgBuffer, 0, NULL
            );

            // Print the error message
            std::cerr << "[Repacker-Wrapper] Error: " << (char*)msgBuffer << std::endl;

            // Free the buffer allocated by FormatMessage
            LocalFree(msgBuffer);
        }

        BOOL RunProcess(const std::string& executable, const std::vector<std::string>& args)
        {
            std::string commandLine = executable;  // Start with the executable path
            
            // Append arguments to the command line
            for (const auto& arg : args) {
                commandLine += " " + arg;
            }

            // Set up the STARTUPINFO and PROCESS_INFORMATION structures
            STARTUPINFOA si = {0};
            si.cb = sizeof(STARTUPINFO);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOW;  // Show the window (can be modified as needed)

            PROCESS_INFORMATION pi = {0};

            // Create the process
            if (CreateProcessA(
                    NULL,                   // Application name
                    &commandLine[0],        // Command line (passed as a writable string)
                    NULL,                   // Process security attributes
                    NULL,                   // Thread security attributes
                    FALSE,                  // Inherit handles
                    0,                      // Creation flags
                    NULL,                   // Environment block
                    NULL,                   // Current directory
                    &si,                    // Startup info
                    &pi                     // Process info
                )) {
                
                if (!FreeConsole())
                {
                    PrintLastError();
                    std::cerr << "[Repacker-Wrapper] Failed to free console!" << std::endl;
                }

                // Wait for the process to exit
                WaitForSingleObject(pi.hProcess, INFINITE);
                
                // Close handles
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                return TRUE;
            }
            else
            {
                PrintLastError();
                std::cerr << "[Repacker-Wrapper] CreateProcess failed!" << std::endl;
                return FALSE;
            }
        })";
    main << "\n\n";
}


void GenerateMainFunction(std::ofstream& main, size_t unzippedFileSize, const std::string& exeName)
{
    main << "int main(int args, const char** argv)\n{\n";
    main << "\tconst fs::path temp = GetTempDir();\n";
    main << "\tconst auto diskSpace = fs::space(temp);\n";
    main << "\tconst uintmax_t estimatedBytesNeeded = g_ZipArchiveData.size() + 0x" << unzippedFileSize << ";\n\n";

    main << "\tif (estimatedBytesNeeded >= diskSpace.available)\n\t{\n\t\t";
    main << "std::cerr << \"[Repacker-Wrapper] Not enough disk space available in temporary directory: \" << temp << std::endl;\n\t\treturn -1;\n\t}\n\n";
    main << "\tExtractZipFile(temp);\n";
    main << "\tg_ZipArchiveData.clear();\n\n";

    main << "\tstd::vector<std::string> commandLineArgs;\n";
    main << "\tfor (int i = 1; i < args; ++i)\n";
    main << "\t{\n";
    main << "\t\tcommandLineArgs.push_back(argv[i]);\n";
    main << "\t}\n\n";

    main << "\tif (!RunProcess(fs::path(temp / \"" << exeName << "\").string(), commandLineArgs))\n";
    main << "\t{\n";
    main << "\t\tstd::cerr << \"[Repacker-Wrapper] Failed to run Program.\" << std::endl;\n";
    main << "\t}\n";

    main << "\n\tDeleteTempDir(temp);\n";
    main << "}";
}


void GenerateMainFile(const std::string& exeName, const std::pair<std::vector<unsigned char>, size_t>& archive, const fs::path& buildDir, bool verbose)
{
    std::ofstream main(buildDir / "main.cpp");

    GenerateHeader(main, archive.first);
    GenerateUtilities(main, verbose);
    GenerateMainFunction(main, archive.second, exeName);
}


void GenerateZipHeader(const fs::path& buildDir)
{
    std::ofstream zip(buildDir / "zip_file.h");
    zip << g_ZipFileHeaderAsString;
}


void GernerateAdminManifest(const fs::path& buildDir, bool adminMode, const std::string& cc)
{
    if (!adminMode) return;

    if (cc == "cl") // msvc needs manifest.res
    {
        static const unsigned char adminManifest[] =
        {
            0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x87, 0x01, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x18, 0x00, 0xFF, 0xFF, 0x01, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x09, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31,
            0x2E, 0x30, 0x22, 0x20, 0x65, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 0x3D, 0x22, 0x55, 0x54,
            0x46, 0x2D, 0x38, 0x22, 0x20, 0x73, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x6C, 0x6F, 0x6E, 0x65, 0x3D,
            0x22, 0x79, 0x65, 0x73, 0x22, 0x3F, 0x3E, 0x0D, 0x0A, 0x3C, 0x61, 0x73, 0x73, 0x65, 0x6D, 0x62,
            0x6C, 0x79, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3D, 0x22, 0x75, 0x72, 0x6E, 0x3A, 0x73, 0x63,
            0x68, 0x65, 0x6D, 0x61, 0x73, 0x2D, 0x6D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x2D,
            0x63, 0x6F, 0x6D, 0x3A, 0x61, 0x73, 0x6D, 0x2E, 0x76, 0x31, 0x22, 0x20, 0x6D, 0x61, 0x6E, 0x69,
            0x66, 0x65, 0x73, 0x74, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31, 0x2E, 0x30,
            0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x74, 0x72, 0x75, 0x73, 0x74, 0x49, 0x6E, 0x66, 0x6F,
            0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3D, 0x22, 0x75, 0x72, 0x6E, 0x3A, 0x73, 0x63, 0x68, 0x65,
            0x6D, 0x61, 0x73, 0x2D, 0x6D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x2D, 0x63, 0x6F,
            0x6D, 0x3A, 0x61, 0x73, 0x6D, 0x2E, 0x76, 0x33, 0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20,
            0x3C, 0x73, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20,
            0x20, 0x20, 0x3C, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64, 0x50, 0x72, 0x69, 0x76,
            0x69, 0x6C, 0x65, 0x67, 0x65, 0x73, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
            0x20, 0x3C, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64, 0x45, 0x78, 0x65, 0x63, 0x75,
            0x74, 0x69, 0x6F, 0x6E, 0x4C, 0x65, 0x76, 0x65, 0x6C, 0x20, 0x6C, 0x65, 0x76, 0x65, 0x6C, 0x3D,
            0x22, 0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x41, 0x64, 0x6D, 0x69, 0x6E, 0x69, 0x73, 0x74,
            0x72, 0x61, 0x74, 0x6F, 0x72, 0x22, 0x20, 0x75, 0x69, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x3D,
            0x22, 0x66, 0x61, 0x6C, 0x73, 0x65, 0x22, 0x2F, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20,
            0x20, 0x3C, 0x2F, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64, 0x50, 0x72, 0x69, 0x76,
            0x69, 0x6C, 0x65, 0x67, 0x65, 0x73, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x2F, 0x73,
            0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x2F, 0x74, 0x72,
            0x75, 0x73, 0x74, 0x49, 0x6E, 0x66, 0x6F, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x61, 0x73, 0x73, 0x65,
            0x6D, 0x62, 0x6C, 0x79, 0x3E, 0x0D, 0x0A, 0x00
        };
        static const unsigned long long adminManifestSize = sizeof(adminManifest) / sizeof(*adminManifest);
        const fs::path manifestPath = buildDir / "manifest.res";
        WriteFile(manifestPath, adminManifest, adminManifestSize);
    }
    else
    {
        static const unsigned char adminManifest[] =
        {
            0x64, 0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x04, 0x00, 0x2E, 0x72, 0x73, 0x72, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x1C, 0x02, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x30, 0x40, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
            0x18, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x04, 0x00, 0x00,
            0x48, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x87, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F,
            0x6E, 0x3D, 0x22, 0x31, 0x2E, 0x30, 0x22, 0x20, 0x65, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67,
            0x3D, 0x22, 0x55, 0x54, 0x46, 0x2D, 0x38, 0x22, 0x20, 0x73, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x6C,
            0x6F, 0x6E, 0x65, 0x3D, 0x22, 0x79, 0x65, 0x73, 0x22, 0x3F, 0x3E, 0x0D, 0x0A, 0x3C, 0x61, 0x73,
            0x73, 0x65, 0x6D, 0x62, 0x6C, 0x79, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3D, 0x22, 0x75, 0x72,
            0x6E, 0x3A, 0x73, 0x63, 0x68, 0x65, 0x6D, 0x61, 0x73, 0x2D, 0x6D, 0x69, 0x63, 0x72, 0x6F, 0x73,
            0x6F, 0x66, 0x74, 0x2D, 0x63, 0x6F, 0x6D, 0x3A, 0x61, 0x73, 0x6D, 0x2E, 0x76, 0x31, 0x22, 0x20,
            0x6D, 0x61, 0x6E, 0x69, 0x66, 0x65, 0x73, 0x74, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D,
            0x22, 0x31, 0x2E, 0x30, 0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x74, 0x72, 0x75, 0x73, 0x74,
            0x49, 0x6E, 0x66, 0x6F, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3D, 0x22, 0x75, 0x72, 0x6E, 0x3A,
            0x73, 0x63, 0x68, 0x65, 0x6D, 0x61, 0x73, 0x2D, 0x6D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66,
            0x74, 0x2D, 0x63, 0x6F, 0x6D, 0x3A, 0x61, 0x73, 0x6D, 0x2E, 0x76, 0x33, 0x22, 0x3E, 0x0D, 0x0A,
            0x20, 0x20, 0x20, 0x20, 0x3C, 0x73, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x3E, 0x0D, 0x0A,
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64,
            0x50, 0x72, 0x69, 0x76, 0x69, 0x6C, 0x65, 0x67, 0x65, 0x73, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20,
            0x20, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64, 0x45,
            0x78, 0x65, 0x63, 0x75, 0x74, 0x69, 0x6F, 0x6E, 0x4C, 0x65, 0x76, 0x65, 0x6C, 0x20, 0x6C, 0x65,
            0x76, 0x65, 0x6C, 0x3D, 0x22, 0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x41, 0x64, 0x6D, 0x69,
            0x6E, 0x69, 0x73, 0x74, 0x72, 0x61, 0x74, 0x6F, 0x72, 0x22, 0x20, 0x75, 0x69, 0x41, 0x63, 0x63,
            0x65, 0x73, 0x73, 0x3D, 0x22, 0x66, 0x61, 0x6C, 0x73, 0x65, 0x22, 0x2F, 0x3E, 0x0D, 0x0A, 0x20,
            0x20, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x2F, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64,
            0x50, 0x72, 0x69, 0x76, 0x69, 0x6C, 0x65, 0x67, 0x65, 0x73, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20,
            0x20, 0x3C, 0x2F, 0x73, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x3E, 0x0D, 0x0A, 0x20, 0x20,
            0x3C, 0x2F, 0x74, 0x72, 0x75, 0x73, 0x74, 0x49, 0x6E, 0x66, 0x6F, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F,
            0x61, 0x73, 0x73, 0x65, 0x6D, 0x62, 0x6C, 0x79, 0x3E, 0x0D, 0x0A, 0x00, 0x48, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x2E, 0x72, 0x73, 0x72, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00
        };
        static const unsigned long long adminManifestSize = sizeof(adminManifest) / sizeof(*adminManifest);
        const fs::path manifestPath = buildDir / "manifest.o";
        WriteFile(manifestPath, adminManifest, adminManifestSize);
    }
}


void Compile(const fs::path& buildDir, const std::string& cc, const std::string& args, bool adminMode)
{
    std::string command = cc + " " + fs::path(buildDir / "main.cpp").string();
    if (adminMode)
    {
        if (cc == "cl")
            command += " " + fs::path(buildDir / "manifest.res").string();
        else
            command += " " + fs::path(buildDir / "manifest.o").string();
    }

    command += " " + args;
    if (cc == "cl") // msvc
        command += " /Fe:repacked.exe";
    else
        command += " -o repacked.exe";

    std::cout << "Compiling: " << command << std::endl;
    std::system(command.c_str());
}


struct Settings
{
    fs::path exePath;
    std::string cc = "g++";
    std::string args;
    bool verbose = false;
    bool adminMode = false;
    bool leaveIntermediate = false;
    bool ok = false;
};

void PrintHelp(const char* name)
{
    std::cout << "Usage: " << name << " <options> <exe-file>" << std::endl;
    std::cout << "Repacker: Bundle all the necessary files for your program into a single EXE file,\n";
    std::cout << "          that can be distributet as a standalone EXE.\n";
    std::cout << "          The argument is the path to your exe file that will be the executable you wan't to run\n";
    std::cout << "          All files in the programs folder, including subdirectories, will be included\n";
    std::cout << "          Requirements: A C++ compiler that supports C++17\n";
    std::cout << "          -h | --help           Show this help message\n";
    std::cout << "          -v | --verbose        When running show log messages from the repacked program\n";
    std::cout << "          -s | --sudo           The embedded program needs admin priviliges to run\n";
    std::cout << "          -c | --cc             Selected C++ compiler (Default: GCC/Clang/MSVC etc.)\n";
    std::cout << "          -g | --debug          Don't optimize the wrapper program (Not strictly necessary)\n";
    std::cout << "          -i | --intermediate   Get the generated C files (Doesn't compile it for you)\n";
    std::cout << "          -a | --arguments      Provide custom compiler arguments, no defaults will be used\n";
    std::cout << "                                Default: -static -static-libgcc -static-libstdc++ -std=c++17 (-O2 dependend on -g)\n";
}

Settings ParseCmd(int args, const char** argv)
{
    Settings s;
    bool debug = false;

    for (int i = 1; i < args; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            PrintHelp(argv[0]);
            return s;
        }
        else if (arg == "-c" || arg == "--cc")
        {
            if (i + 1 < args)  // Check if we are not at the last argument
            {
                s.cc = argv[i + 1];  // Set the next argument as the compiler
                ++i;  // Skip the next argument since it's already processed
            }
            else
            {
                std::cerr << "Error: Missing argument for compiler selection!" << std::endl;
                return s;
            }
        }
        else if (arg == "-i" || arg == "--intermediate")
        {
            s.leaveIntermediate = true;
        }
        else if (arg == "-s" || arg == "--sudo")
        {
            s.adminMode = true;
        }
        else if (arg == "-v" || arg == "--verbose")
        {
            s.verbose = true;
        }
        else if (arg == "-g" || arg == "--debug")
        {
            debug = true;
        }
        else if (arg == "-a" || arg == "--arguments")
        {
            if (i + 1 < args)  // Check if we are not at the last argument
            {
                s.args = argv[i + 1];  // Set the next argument as the compiler
                ++i;  // Skip the next argument since it's already processed
            }
            else
            {
                std::cerr << "Error: Missing argument for compiler selection!" << std::endl;
                return s;
            }
        }
        else
        {
            s.exePath = arg;
        }
    }

    if (s.exePath.empty())
    {
        std::cerr << "No file specified\nTry --help for additional information\n";
        return s;
    }

    if (s.cc == "gcc")
        s.cc = "g++";

    if (s.cc == "clang")
        s.cc = "clang++";

    if (s.cc == "msvc")
        s.cc = "cl";

    if (s.args.empty())
    {
        if (s.cc == "g++")
            s.args = "-static -static-libgcc -static-libstdc++ -std=c++17"; // -static-libc++

        if (s.cc == "clang++")
            s.args = "-static -std=c++17";

        if (s.cc == "cl")
            s.args = "/MT /std:c++17 /EHsc";

        if (!debug)
        {
            if (s.cc == "cl")
                s.args += " /O2";
            else
                s.args += " -O2";
        }
    }

    s.ok = true;
    return s;
}


int main(int args, const char** argv)
{
    Settings s = ParseCmd(args, argv);
    if (!s.ok) return 0;

    const std::optional<fs::path> buildDir = CreateBuildDir();
    if (!buildDir) return -1;

    std::pair<std::vector<unsigned char>, size_t> archive = ZipFiles(s.exePath);

    std::cout << "Generating build files..." << std::endl;
    GenerateMainFile(s.exePath.filename().string(), archive, buildDir.value(), s.verbose);
    GenerateZipHeader(buildDir.value());
    GernerateAdminManifest(buildDir.value(), s.adminMode, s.cc);

    if (!s.leaveIntermediate)
    {
        Compile(buildDir.value(), s.cc, s.args, s.adminMode);
        DeleteDir(buildDir.value());
    }
}