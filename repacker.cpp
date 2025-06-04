#include <string>
#include <vector>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <iostream>
#include <filesystem>
#include <system_error>

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
    fs::path buildDir = "cmp-" + SystemClockMillis();
    std::error_code ec;

    if (!fs::create_directories(buildDir, ec))
    {
        if (ec) {
            std::cerr << "Failed to create build directory: " << ec.message() << std::endl;
            return std::nullopt;
        } // else folder already exists, just override
    }
    return buildDir;
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

bool WriteBinaryFile(std::ofstream& file, const fs::path& srcFile)
{
    std::ifstream inFile(srcFile, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open file for header data generation: " << srcFile << std::endl;
        std::cerr << "Reason: " << strerror(errno) << std::endl;
        return false;
    }

    constexpr size_t bufferSize = 4096;
    unsigned char buffer[bufferSize];
    
    size_t totalBytesRead = 0;

    while (inFile.read(reinterpret_cast<char*>(buffer), bufferSize) || inFile.gcount() > 0)
    {
        size_t bytesRead = inFile.gcount();  // Number of bytes actually read
        
        // Write each byte as a hex value in the output file
        for (size_t i = 0; i < bytesRead; ++i)
        {
            file << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]) << ", ";
            ++totalBytesRead;

            if (totalBytesRead % 16 == 0)
                file << "\n";
        }
    }

    return true;
}

std::vector<fs::path> CollectFiles(const fs::path& dir)
{
    std::vector<fs::path> files;

    try
    {
        for (const auto& entry : fs::recursive_directory_iterator(dir))
        {
            try
            {
                if (fs::is_regular_file(entry))
                {
                    files.push_back(entry.path());
                }
            }
            catch (const fs::filesystem_error& e)
            {
                std::cerr << "Error accessing file: " << entry.path() << " - " << e.what() << std::endl;
            }
        }
    }
    catch(const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem error while processing directory " << dir << ": " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        // Catch any other std::exception
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return files;
}

struct FileInfo
{
    std::string fileName;
    std::string fileData;
    std::string fileSize;
    std::string fileHeader;
    std::string filePath;
    bool ok = false;
};

FileInfo GenerateFileHeader(const fs::path& srcPath, const fs::path& buildDir, const fs::path& exePath)
{
    const std::string hash = Hash::File::HashFile<Hash::MD5>(srcPath.string());

    const fs::path fileNamePath = fs::relative(srcPath, exePath.parent_path() == "" ? "." : exePath.parent_path());
    FileInfo info;
    info.fileData = "F_" + hash + "_Data";
    info.fileSize = "F_" + hash + "_Size";
    info.fileHeader = "F" + hash + ".h";
    info.fileName = "F_" + hash + "_Name";
    info.filePath= "F_" + hash + "_Path";

    const std::string ifdefguard = "D_" + hash + "_H";
    const std::string filePath = info.filePath + " = R\"(" + fileNamePath.parent_path().string() + ")\";";
    const std::string fileName = info.fileName + " = R\"(" + fileNamePath.string() + ")\";";
    const std::string fileData = info.fileData;
    const std::string fileSize = info.fileSize + " = sizeof(" + fileData + ") / sizeof(*" + fileData + ");";

    std::ofstream outFile(buildDir / info.fileHeader);
    if (!outFile)
    {
        std::cerr << "Failed to generate header file for: " << srcPath << std::endl;
        std::cerr << "Reason: " << strerror(errno) << std::endl;
        return info;
    }

    outFile << "#ifndef " << ifdefguard << '\n';
    outFile << "#define " << ifdefguard << "\n\n";
    outFile << "inline const char* " << filePath << '\n';
    outFile << "inline const char* " << fileName << '\n';
    outFile << "inline const unsigned char " << fileData << "[] = {\n";

    if (!WriteBinaryFile(outFile, srcPath))
        return info;

    outFile << "\n};\n";
    outFile << "inline const unsigned long long " << fileSize << "\n";
    outFile << "#endif // " << ifdefguard << "\n";

    outFile.close();
    info.ok = true;
    return info;
}

void GenerateMainFile(const fs::path& buildDir, const std::vector<FileInfo>& fileInfo)
{
    std::ofstream outFile(buildDir / "main.cpp");
    if (!outFile)
    {
        std::cerr << "Failed to open main file" << std::endl;
        std::cerr << "Reason: " << strerror(errno) << std::endl;
        return;
    }

    outFile << "#define _CRT_SECURE_NO_WARNINGS\n";
    outFile << "#include <Windows.h>\n\n";
    outFile << "#include <chrono>\n";
    outFile << "#include <vector>\n";
    outFile << "#include <string>\n";
    outFile << "#include <cerrno>\n";
    outFile << "#include <cstdlib>\n";
    outFile << "#include <fstream>\n";
    outFile << "#include <cstring>\n";
    outFile << "#include <iostream>\n";
    outFile << "#include <filesystem>\n";
    outFile << "#include <system_error>\n";
    outFile << "namespace fs = std::filesystem;\n\n";

    for (const FileInfo& info : fileInfo)
    {
        outFile << "#include \"" << info.fileHeader << "\"\n";
    }
    outFile << "\n";

    const std::string test = R"(
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
                    std::cerr << "Failed to create directory: " << path << " " << ec.message() << std::endl;
                    std::cerr << "Programm terminates" << std::endl;
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

        void WriteFile(const fs::path& path, const fs::path& dirPath, const unsigned char* data, size_t len)
        {
            if (!dirPath.empty())
                CreateDirectories(dirPath);

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

        void DeleteTempDir(const fs::path& path)
        {
            try
            {
                uintmax_t removed = fs::remove_all(path);  // Remove all contents and the folder itself
            }
            catch (const fs::filesystem_error& e)
            {
                // nothing can't show output because console no longer exists, just catch the error so it doesn't crash
            }
        }

        void PrintLastError()
        {
            DWORD error = GetLastError();
            
            // Allocate a buffer to hold the formatted message
            LPVOID msgBuffer;

            // Use FormatMessage to get the error message
            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&msgBuffer, 0, NULL
            );

            // Print the error message
            std::cerr << "Error: " << (char*)msgBuffer << std::endl;

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
                    std::cerr << "Failed to free console!" << std::endl;
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
                std::cerr << "CreateProcess failed!" << std::endl;
                return FALSE;
            }
        }
    )";

    outFile << test << std::endl;
    std::string mainFunc = "int main(int args, const char** argv)\n{\n";
    mainFunc += "\tconst fs::path temp = GetTempDir();\n";
    mainFunc += "\tconst auto diskSpace = fs::space(temp);\n";
    mainFunc += "\tconst unsigned long long estimatedBytesNeeded = ";
    
    for (const FileInfo& info : fileInfo)
    {
        mainFunc += info.fileSize + " + ";
    }
    mainFunc += "0;\n";
    
    mainFunc += "\tif (diskSpace.available < estimatedBytesNeeded)\n\t{\n";
    mainFunc += "\t\tstd::cerr << \"Not enough disk space available in temporary directory: \" << temp << std::endl;\n\t\treturn -1;\n\t}\n\n";
    
    for (const FileInfo& info : fileInfo)
    {
        mainFunc += "\tWriteFile(temp / " + info.fileName + ", temp / " + info.filePath + ", " + info.fileData + ", " + info.fileSize + ");\n";
    }

    mainFunc += R"(
    std::vector<std::string> commandLineArgs;
    for (int i = 1; i < args; ++i)
    {
        commandLineArgs.push_back(argv[i]);
    }
    )";

    mainFunc += "std::string exePath = (temp / " + fileInfo[0].fileName + ").string();\n";
    mainFunc += R"(
    if (!RunProcess(exePath, commandLineArgs))
    {
        std::cerr << "Failed to run Program." << std::endl;
    }

    DeleteTempDir(temp);)";

    mainFunc += "\n}";
    outFile << mainFunc;
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


void Compile(const fs::path& buildDir, const std::string& cc, const std::string& args, bool adminMode)
{
    std::string command = cc + " " + fs::path(buildDir / "main.cpp").string();
    if (adminMode)
    {
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
            command += " " + manifestPath.string();
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
            command += " " + manifestPath.string();
        }
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
    std::vector<fs::path> files;
    std::string cc = "g++";
    std::string args;
    bool includeFolder = false;
    bool adminMode = false;
    bool leaveIntermediate = false;
    bool ok = false;
};

void PrintHelp(const char* name)
{
    std::cout << "Usage: " << name << " <options> <files>" << std::endl;
    std::cout << "Repacker: Bundle all the necessary files for your program into a single EXE file,\n";
    std::cout << "          that can be distributet as a standalone EXE.\n";
    std::cout << "          The first file in files has to be the EXE that is supposed to be executed, the rest are the additional files\n";
    std::cout << "          Requirements: A C++ compiler that supports C++17\n";
    std::cout << "          -h | --help           Show this help message\n";
    std::cout << "          -s | --sudo           The embedded program needs admin priviliges to run\n";
    std::cout << "          -c | --cc             Selected C++ compiler (Default: GCC/Clang/MSVC etc.)\n";
    std::cout << "          -g | --debug          Don't optimize the wrapper program (Not strictly necessary)\n";
    std::cout << "          -i | --intermediate   Get the generated C files (Doesn't compile it for you)\n";
    std::cout << "          -f | --folder         This automatically option includes all the files that are in your programs directory\n";
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
        else if (arg == "-f" || arg == "--folder")
        {
            s.includeFolder = true;
        }
        else if (arg == "-s" || arg == "--sudo")
        {
            s.adminMode = true;
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
            s.files.push_back(arg);
        }
    }

    if (s.files.empty())
    {
        std::cerr << "No files specified\nTry --help for additional information\n";
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

    if (s.includeFolder)
    {
        for (const auto& entry : fs::directory_iterator(s.files[0].parent_path().empty() ? "." : s.files[0].parent_path()))
        {
            if (fs::is_regular_file(entry.status()) || fs::is_directory(entry.status()))
            {
                if (entry.path() != s.files[0])
                    s.files.push_back(entry.path());
            }
        }
    }

    s.ok = true;
    return s;
}


int main(int args, const char** argv)
{
    Settings settings = ParseCmd(args, argv);
    if (!settings.ok)
        return 0;

    const std::optional<fs::path> buildDir = CreateBuildDir();
    if (!buildDir) return -1;

    std::vector<fs::path> files;
    for (size_t i = 0; i < settings.files.size(); i++)
    {
        const fs::path filePath(settings.files[i]);
        if (fs::is_directory(filePath))
        {
            std::vector<fs::path> collectedFiles = CollectFiles(filePath);
            files.insert(files.end(), collectedFiles.begin(), collectedFiles.end());
        }
        else if (fs::is_regular_file(filePath))
        {
            files.push_back(filePath);
        }
    }

    if (files.empty())
    {
        std::cerr << "None of the specified files have been found\n";
        return -1;
    }
    std::cout << "Packing executable: " << files[0] << std::endl;

    std::vector<FileInfo> fileInfo;
    for (const fs::path& f : files)
    {
        FileInfo info = GenerateFileHeader(f, buildDir.value(), files[0]);
        if (!info.ok)
            return -1;

        fileInfo.push_back(info);
    }

    GenerateMainFile(buildDir.value(), fileInfo);

    if (!settings.leaveIntermediate)
    {
        Compile(buildDir.value(), settings.cc, settings.args, settings.adminMode);
        DeleteDir(buildDir.value());
    }
}