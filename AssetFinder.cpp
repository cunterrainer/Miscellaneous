#include <cstdio>
#include <string>
#include <vector>
#include <cerrno>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <algorithm>

#define PRINT_OFFSET(fileFormat, offset) std::cout << "Found " << fileFormat << " at: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << offset << '\n'

volatile bool IsLittleEndian()
{
    uint32_t num = 0x01020304;
    volatile uint8_t *byte = reinterpret_cast<volatile uint8_t*>(&num);
    if (*byte == 0x04)
        return true;
    return false;
}


inline uint16_t SwapEndianness16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}


inline uint32_t SwapEndianness32(uint32_t val)
{
    return ((val << 24) & 0xFF000000) | ((val <<  8) & 0x00FF0000) | ((val >>  8) & 0x0000FF00) | ((val >> 24) & 0x000000FF);
}


void WriteToFile(const std::string& path, const std::vector<unsigned char>& buffer, std::size_t start, std::size_t end, bool write)
{
    if (!write)
        return;
    const auto subStart = buffer.cbegin() + start;
    const auto subEnd = buffer.cbegin() + end;

    // Create a new vector to hold the copied range
    std::vector<unsigned char> subset(subEnd - subStart);
    std::copy(subStart, subEnd, subset.begin());

    std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file: " << path << " for writing Error: " << std::strerror(errno) << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(subset.data()), subset.size());
    if (!file)
    {
        std::cerr << "Failed to write to file: " << path << " Error: " << std::strerror(errno) << std::endl;
    }
    file.close();
}


bool FindStringInArray(const char *array[], size_t arraySize, const char *target, size_t chars)
{
    for (size_t i = 0; i < arraySize; ++i)
    {
        if (strncmp(array[i], target, chars) == 0)
        {
            return true;  // Match found
        }
    }
    return false;
}


struct Config
{
    std::string inputFile;
    bool skip = true;
    bool write = true;

    bool png = true;
    bool jpg = true;
    bool bmp = true;
    bool webp = true;
    bool oggs = true;
    bool wav = true;
    bool avi = true;
    bool ttf = true;
    bool cdr = true;
    bool ani = true;
    bool pal = true;
    bool riff = true;
    bool valid = true;
};


void PrintHelp(const char* program)
{
    puts  ("Asset Finder");
    puts  ("Disclamer!: False positives are normal just skip them");
    printf("Usage: %s [input file] [options]\n", program);
    puts  ("       --help           | -h        Show this help section");
    puts  ("       --no-file        | -f        Don't write found files to the disk only print their locations");
    puts  ("       --no-skip        | -s        Don't skip forward after finding a file. This is usefull to avoid files");
    puts  ("                                    not being detected, however more false positives will be found");
    puts  ("       --disable=[type] | -d=[type] Don't look for file type, type is one of 'Supported file types'");
    puts  ("                                    Disabling any type using the RIFF format implicitly disables RIFF");
    puts  ("                                    To enable RIFF use '--enable=riff'");
    puts  ("       --enable=[type]  | -e=[type] Disables all by default and only enables specified types,");
    puts  ("                                    type is one of 'Supported file types'");
    puts  ("       The last option which is non off the above will be treated as the input file\n");
    puts  ("Supported file types:");
    puts  ("   Images:");
    puts  ("      png");
    puts  ("      jpeg");
    puts  ("      bmp");
    puts  ("      webp");
    puts  ("   Sounds:");
    puts  ("      ogg");
    puts  ("      wav");
    puts  ("   Videos:");
    puts  ("      avi");
    puts  ("   Fonts:");
    puts  ("      ttf");
    puts  ("   Misc:");
    puts  ("      cdr");
    puts  ("      ani");
    puts  ("      pal");
    puts  ("      riff");
}


void PrintUsage(const char* program)
{
    printf("Usage: %s [input file] [options]\n", program);
    puts("Use '--help' to get additional information");
}


Config ParseCmd(int argc, const char* argv[])
{
    Config cfg;
    if (argc == 1)
    {
        PrintUsage(argv[0]);
        cfg.valid = false;
        return cfg;
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            PrintHelp(argv[0]);
            cfg.valid = false;
            return cfg;
        }
        else if (arg == "-no-skip" || arg == "-s")
        {
            cfg.skip = false;
        }
        else if (arg == "-no-file" || arg == "-f")
        {
            cfg.write = false;
        }
        else if (strncmp(argv[i], "--disable=", 10) == 0 || strncmp(argv[i], "-d=", 3) == 0)
        {
            const size_t pos = arg.find('=');
            const std::string fileType = arg.substr(pos+1);

            if (fileType == "png")
                cfg.png = false;
            else if (fileType == "jpeg")
                cfg.jpg = false;
            else if (fileType == "bmp")
                cfg.bmp = false;
            else if (fileType == "webp")
            {
                cfg.riff = false;
                cfg.webp = false;
            }
            else if (fileType == "ogg")
                cfg.oggs = false;
            else if (fileType == "wav")
            {
                cfg.riff = false;
                cfg.wav = false;
            }
            else if (fileType == "avi")
            {
                cfg.riff = false;
                cfg.avi = false;
            }
            else if (fileType == "ttf")
                cfg.ttf = false;
            else if (fileType == "cdr")
            {
                cfg.riff = false;
                cfg.cdr = false;
            }
            else if (fileType == "ani")
            {
                cfg.riff = false;
                cfg.ani = false;
            }
            else if (fileType == "pal")
            {
                cfg.riff = false;
                cfg.pal = false;
            }
            else if (fileType == "riff")
                cfg.riff = false;
            else
            {
                cfg.valid = false;
                printf("Unknown disable type: '%s'\nType has to be one of 'Supported file types'\nTry '--help' for additional information\n", fileType.c_str());
                return cfg;
            }
        }
        else if (strncmp(argv[i], "--enable=", 9) == 0 || strncmp(argv[i], "-e=", 3) == 0)
        {
            static bool firstTime = true;
            if (firstTime)
            {
                cfg.png = false;
                cfg.jpg = false;
                cfg.bmp = false;
                cfg.webp = false;
                cfg.oggs = false;
                cfg.wav = false;
                cfg.avi = false;
                cfg.ttf = false;
                cfg.cdr = false;
                cfg.ani = false;
                cfg.pal = false;
                cfg.riff = false;
                firstTime = false;
            }

            const size_t pos = arg.find('=');
            const std::string fileType = arg.substr(pos+1);

            if (fileType == "png")
                cfg.png = true;
            else if (fileType == "jpeg")
                cfg.jpg = true;
            else if (fileType == "bmp")
                cfg.bmp = true;
            else if (fileType == "webp")
                cfg.webp = true;
            else if (fileType == "ogg")
                cfg.oggs = true;
            else if (fileType == "wav")
                cfg.wav = true;
            else if (fileType == "avi")
                cfg.avi = true;
            else if (fileType == "ttf")
                cfg.ttf = true;
            else if (fileType == "cdr")
                cfg.cdr = true;
            else if (fileType == "ani")
                cfg.ani = true;
            else if (fileType == "pal")
                cfg.pal = true;
            else if (fileType == "riff")
                cfg.riff = true;
            else
            {
                cfg.valid = false;
                printf("Unknown enable type: '%s'\nType has to be one of 'Supported file types'\nTry '--help' for additional information\n", fileType.c_str());
                return cfg;
            }
        }
        else
        {
            cfg.inputFile = arg;
        }
    }

    if (cfg.inputFile.empty())
    {
        PrintUsage(argv[0]);
        cfg.valid = false;
    }
    return cfg;
}


int main(int argc, const char* argv[])
{
    Config cfg = ParseCmd(argc, argv);
    if (!cfg.valid)
        return 0;

    // we use puts, printf in ParseCmd
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::ifstream file(cfg.inputFile, std::ios::binary);
    if (!file)
    {
        std::cerr << "Error opening file '" << cfg.inputFile << "': " << std::strerror(errno) << std::endl;
        return 1;
    }

    file.seekg(0, std::ios::end); // Move to the end of the file
    std::size_t size = file.tellg(); // Get the position of the end (file size)
    file.seekg(0, std::ios::beg); // Move back to the beginning of the file
    std::vector<unsigned char> buffer(size);

    // Read the file into the buffer
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        std::cerr << "Error reading file '" << cfg.inputFile << "': " << std::strerror(errno) << std::endl;
        return 1;
    }
    file.close();


    for (std::size_t i = 0; i < size-24; ++i)
    {
        if (cfg.png && buffer[i] == 137 && buffer[i+1] == 80 && buffer[i+2] == 78 && buffer[i+3] == 71 && buffer[i+4] == 13 && buffer[i+5] == 10 && buffer[i+6] == 26 && buffer[i+7] == 10 && /* end of PNG header start of IHDR chunk*/
            buffer[i+8] == 0 && buffer[i+9] == 0 && buffer[i+10] == 0 && buffer[i+11] == 13 && buffer[i+12] == 0x49 && buffer[i+13] == 0x48 && buffer[i+14] == 0x44 && buffer[i+15] == 0x52)
        {
            // PNG found
            // Find end
            for (std::size_t k = i+8; k < size - 7; ++k)
            {
                if (buffer[k] == 0x49 && buffer[k+1] == 0x45 && buffer[k+2] == 0x4E && buffer[k+3] == 0x44 && buffer[k+4] == 0xAE && buffer[k+5] == 0x42 && buffer[k+6] == 0x60 && buffer[k+7] == 0x82)
                {
                    PRINT_OFFSET("PNG", i);
                    WriteToFile("PNG" + std::to_string(i) + ".png", buffer, i, k+8, cfg.write);
                    if (cfg.skip)
                        i = k+7; // i+1 in for loop
                    break;
                }
            }
        }
        else if (cfg.jpg && buffer[i] == 0xFF && buffer[i+1] == 0xD8)
        {
            // JPEG found
            // Find end
            for (std::size_t k = i+2; k < size-1; ++k)
            {
                if (buffer[k] == 0xFF && buffer[k+1] == 0xD9)
                {
                    PRINT_OFFSET("JPEG", i);
                    WriteToFile("JPEG" + std::to_string(i) + ".jpeg", buffer, i, k+2, cfg.write);
                    if (cfg.skip)
                        i = k+1; // ++i after loop ends
                    break;
                }
            }
        }
        else if (cfg.bmp && buffer[i] == 'B' && buffer[i+1] == 'M' && buffer[i+6] == 0 && buffer[i+7] == 0 && buffer[i+8] == 0 && buffer[i+9] == 0)
        {
            // BMP found
            uint32_t fileSize = *((uint32_t*)&buffer[i+2]);

            if (!IsLittleEndian())
            {
                // swap from little to big endian
                fileSize = SwapEndianness32(fileSize);
            }

            if (i + fileSize > size)
            {
                i += 9;
                continue;
            }

            PRINT_OFFSET("BMP", i);
            WriteToFile("BMP" + std::to_string(i) + ".bmp", buffer, i, i+fileSize, cfg.write);
            if (cfg.skip)
                i += fileSize-1; // ++i after loop ends
        }
        else if (cfg.oggs && buffer[i] == 'O' && buffer[i+1] == 'g' && buffer[i+2] == 'g' && buffer[i+3] == 'S')
        {
            // OGG found
            static constexpr size_t minPageHeaderSize = 27;

            for (size_t l = i; l <= size;)
            {
                if (l < size - minPageHeaderSize && buffer[l] == 'O' && buffer[l+1] == 'g' && buffer[l+2] == 'g' && buffer[l+3] == 'S')
                {
                    const uint8_t pageSegments = buffer[l+26];
                    uint64_t segmentSize = 0;
                    for (uint8_t k = 0; k < pageSegments; ++k)
                    {
                        segmentSize += buffer[l+minPageHeaderSize+k];
                    }
                    l = l + minPageHeaderSize + pageSegments + segmentSize;
                }
                else // OOG file is over
                {
                    PRINT_OFFSET("OGG", i);
                    WriteToFile("OGG" + std::to_string(i) + ".ogg", buffer, i, l, cfg.write);
                    if (cfg.skip)
                        i = l; // ++i after loop ends
                    break;
                }
            }
        }
        else if (cfg.ttf && buffer[i] == 0x00 && buffer[i+1] == 0x01 && buffer[i+2] == 0x00 && buffer[i+3] == 0x00)
        {
            // TTF found
            uint16_t numberOfTables = *((uint16_t*)&buffer[i+4]);
            if (IsLittleEndian())
            {
                // swap from big to little endian
                numberOfTables = SwapEndianness16(numberOfTables);
            }

            static constexpr uint8_t ttfHeaderSize = 12;
            size_t endIdx = i + ttfHeaderSize; // first table directory entry

            size_t maxOffset = 0;
            size_t maxOffsetSize = 0;
            bool found = true;
            for (size_t k = 0; k < numberOfTables; ++k)
            {
                // We need to check each table for a valid tag name because the ttf identifier 0x00010000 is quiet common otherwise we'd get a bunch of false positives
                static const char* allowedTags[] = {
                    "name", "glyf", "cmap", "post", "OS/2", "head", "hmtx", "hhea", "kern", "hdmx", "LTHS", "vmtx", "vhea", "VDMX", "loca", "maxp", "DSIG",
                    "PCLT", "gasp", "prep", "fpgm", "cvt ", "CFF ", "VORG", "EBDT", "EBLC", "EBSC", "GSUB", "GPOS", "GDEF", "BASE", "JSTF", "Silf", "Glat",
                    "Gloc", "Feat", "mort", "morx", "feat", "acnt", "bsln", "just", "Icar", "fdsc", "fmtx", "prop", "Zapf", "opbd", "trak", "fvar", "gvar",
                    "avar", "cvar", "bdat", "bhed", "bloc", "hsty", "PfEd", "TeX ", "BDF ", "FFTM"
                };
                static constexpr size_t allowedTagsSize = sizeof(allowedTags) / sizeof(allowedTags[0]);
                
                static constexpr uint8_t tableDirEntryHeaderSize = 16;
                if (endIdx+tableDirEntryHeaderSize > size || !FindStringInArray(allowedTags, allowedTagsSize, (const char*)&buffer[endIdx], 4))
                {
                    found = false;
                    break;
                }

                uint32_t tableSize = *((uint32_t*)&buffer[endIdx+12]);
                uint32_t tableOffset = *((uint32_t*)&buffer[endIdx+8]);
                
                if (IsLittleEndian())
                {
                    // swap from big to little endian
                    tableSize = SwapEndianness32(tableSize);
                    tableOffset = SwapEndianness32(tableOffset);
                }

                if (tableOffset > maxOffset)
                {
                    maxOffset = tableOffset;
                    maxOffsetSize = tableSize;
                }
                
                endIdx += tableDirEntryHeaderSize;
            }
            
            endIdx = i+maxOffset+maxOffsetSize;
            if (found && endIdx <= size && endIdx != i)
            {
                PRINT_OFFSET("TTF", i);
                WriteToFile("TTF" + std::to_string(i) + ".ttf", buffer, i, endIdx, cfg.write);
                if (cfg.skip)
                    i += maxOffset+maxOffsetSize-1; // ++i in loop
            }
            else
            {
                i += 3;
            }
        }
        else if (buffer[i] == 'R' && buffer[i+1] == 'I' && buffer[i+2] == 'F' && buffer[i+3] == 'F')
        {
            // RIFF found
            uint32_t fileSize = *((uint32_t*)&buffer[i+4]);

            if (!IsLittleEndian())
            {
                // swap from little to big endian
                fileSize = SwapEndianness32(fileSize);
            }

            fileSize += 8; // at offset 0

            if (i + fileSize > size)
            {
                i += 3;
                continue;
            }

            if (cfg.webp && buffer[i+8] == 'W' && buffer[i+9] == 'E' && buffer[i+10] == 'B' && buffer[i+11] == 'P')
            {
                // WEBP found
                if (fileSize > 4294967294 /* Max webp file size */)
                {
                    i += 11;
                    continue;
                }
                
                PRINT_OFFSET("WEBP", i);
                WriteToFile("WEBP" + std::to_string(i) + ".webp", buffer, i, i+fileSize, cfg.write);
            }
            else if (cfg.wav && buffer[i+8] == 'W' && buffer[i+9] == 'A' && buffer[i+10] == 'V' && buffer[i+11] == 'E' && buffer[i+12] == 'f' && buffer[i+13] == 'm' && buffer[i+14] == 't' && buffer[i+15] == 0x20)
            {
                // WAV found
                PRINT_OFFSET("WAV", i);
                WriteToFile("WAV" + std::to_string(i) + ".wav", buffer, i, i+fileSize, cfg.write);
            }
            else if (cfg.avi && buffer[i+8] == 'A' && buffer[i+9] == 'V' && buffer[i+10] == 'I')
            {
                // AVI found
                PRINT_OFFSET("AVI", i);
                WriteToFile("AVI" + std::to_string(i) + ".avi", buffer, i, i+fileSize, cfg.write);
            }
            else if (cfg.cdr && buffer[i+8] == 'C' && buffer[i+9] == 'D' && buffer[i+10] == 'R')
            {
                // CDR (CorelDRAW) found
                PRINT_OFFSET("CDR", i);
                WriteToFile("CDR" + std::to_string(i) + ".cdr", buffer, i, i+fileSize, cfg.write);
            }
            else if (cfg.ani && buffer[i+8] == 'A' && buffer[i+9] == 'C' && buffer[i+10] == 'O' && buffer[i+11] == 'N')
            {
                // ANI found
                PRINT_OFFSET("ANI", i);
                WriteToFile("ANI" + std::to_string(i) + ".ani", buffer, i, i+fileSize, cfg.write);
            }
            else if (cfg.pal && buffer[i+8] == 'P' && buffer[i+9] == 'A' && buffer[i+10] == 'L')
            {
                // PAL found
                PRINT_OFFSET("PAL", i);
                WriteToFile("PAL" + std::to_string(i) + ".pal", buffer, i, i+fileSize, cfg.write);
            }
            else if (cfg.riff)
            {
                // Generic RIFF found
                PRINT_OFFSET("RIFF", i);
                WriteToFile("RIFF" + std::to_string(i) + ".riff", buffer, i, i+fileSize, cfg.write);
            }
            if (cfg.skip)
                i += fileSize-1; // ++i after loop ends
        }
    }

    return 0;
}