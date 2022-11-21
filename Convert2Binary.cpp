#include <iostream>
#include <bitset>
#include <sstream>
#include <vector>
#include <fstream>
#include <string>

#include "InputHandler.h"

#ifdef _DEBUG
unsigned int allocations_G = 0;
void* operator new(size_t size)
{
    //std::cout << "Allocating " << size << " bytes\n";
    ++allocations_G;
    return malloc(size);
}
#endif

void charToByte(char number, char* out)
{
    int count = 8;
    /* assert: count <= sizeof(int)*CHAR_BIT */
    unsigned int mask = 1U << (count - 1);

    for (int i = 0; i < count; i++) {
        out[i] = (number & mask) ? '1' : '0';
        number <<= 1;
    }
}

int byteToInt(const char* data)
{
    int result = 0;
    int offset = 1;
    for (char i = 7; i > -1; --i)
    {
        if (data[i] == '1')
            result += offset;
        offset *= 2;
    }

    return result;
}


std::string BinaryStringToText(const std::string& str) 
{
    std::size_t str_size = str.size();
    std::string result;
    result.reserve(str_size);
    
    for (std::size_t i = 0; i < str_size; ++i)
    {
        //result.append(".");
        if (str[i] < 33 || str[i] > 126)
            result.append(".");
        else
            result += str[i];
    }
    return result;
}


char g_HexStr[9] = {(char)'\\0',(char)'\\0',(char)'\\0',(char)'\\0',(char)'\\0',(char)'\\0',(char)'\\0',(char)'\\0', (char)'\0'};
char g_HexBuffer[8];
char* DecToHexString(unsigned long long decimal_number)
{
    if (decimal_number > 4294967295)
    {
        memset(g_HexStr, 'F', 8);
        return g_HexStr;
    }

    sprintf_s(g_HexBuffer, "%X", decimal_number);
    if (decimal_number < 0x10) //16
        memcpy(g_HexStr + 7, g_HexBuffer, 1);
    else if (decimal_number < 0x100) // 256
        memcpy(g_HexStr + 6, g_HexBuffer, 2);
    else if (decimal_number < 0x1000) // 4096
        memcpy(g_HexStr + 5, g_HexBuffer, 3);
    else if (decimal_number < 0x10000) // 65536
        memcpy(g_HexStr + 4, g_HexBuffer, 4);
    else if (decimal_number < 0x100000) // 1048576
        memcpy(g_HexStr + 3, g_HexBuffer, 5);
    else if (decimal_number < 0x1000000) // 16777216
        memcpy(g_HexStr + 2, g_HexBuffer, 6);
    else if (decimal_number < 0x10000000) // 268435456
        memcpy(g_HexStr + 1, g_HexBuffer, 7);
    else if (decimal_number < 0x100000000) // 4294967296
        memcpy(g_HexStr, g_HexBuffer, 8);
    return g_HexStr;
}


//input
struct Input
{
    bool error = false;
    bool use_strings = false;
    bool use_file = false;
    char bytes_per_line = 6;
    const char* file_name = "";
    std::string output_file;
    void setError() { error = true; }
};

void printHelpMessage(const char* name)
{
    std::cout << "Usage: " << name << " <filename>\n";
    std::cout << "       " << name << " <filename> <options>\n";
    std::cout << "  Options:" << "\n";
    std::cout << "       -h          : for help\n";
    std::cout << "       -c          : for console output\n";
    std::cout << "       -n <number> : to change the number of byte blocks per line\n";
    std::cout << "       -f          : could be faster but will use more memory (not recommended for large files\n";
    std::cout << "       -o <file>   : specify an output file\n";
}

Input handleInput(InputHandler& ih)
{
    Input ip;
    int size = ih.size();
    const char* name = ih.at(0);

    if (size == 1)
    {
        ip.setError();
        return ip;
    }
    else if (ih.contains_l("-h"))
    {
        ip.setError();
        return ip;
    }
    else
    {
        ip.file_name = ih.at(1);
        std::ifstream input(ip.file_name);
        if (!input) {
            std::cout << "Couldn't open the file: " << ip.file_name << '\n';
            input.close();
            ip.setError();
            return ip;
        }
        input.close();
    }
    
    if (ih.contains_l("-n"))
    {
        int index = ih.location_l("-n");
        if (size < (index + 2))   
        {
            ip.setError();
            return ip;
        }

        if (!ih.isNumber(ih.at(index + 1)))
        {
            ip.setError();
            return ip;
        }

        ip.bytes_per_line = ih.number(ih.at(index + 1));
    }

    if (ih.contains_l("-o"))
    {
        int index = ih.location_l("-o");

        if (size < (index + 2))
        {
            ip.setError();
            return ip;
        }
        ip.output_file = ih.at(index + 1);
    }
    else
        ip.output_file = ip.file_name + std::string("-Binary.txt");

    if (ih.contains_l("-c"))
        ip.use_file = false;
    else
        ip.use_file = true;

    if (ih.contains_l("-f"))
        ip.use_strings = true;

    return ip;
}



int main(int argc, char** argv)
{
    clock_t begin = clock();

    //Input handling
    InputHandler inputhandler(argc, argv);
    Input ip = handleInput(inputhandler);
    if (ip.error)
    {
        printHelpMessage(inputhandler.at(0));
        std::cin.get();
        return -1;
    }

    unsigned char BYTES_PER_LINE_M = ip.bytes_per_line;
    bool use_strings = ip.use_strings;
    bool file_output = ip.use_file;

    std::ifstream input(ip.file_name, std::ios::binary);
    
    // get file infos
    std::string c_line;
    unsigned int line_counter = 0;
    unsigned int longest_line = 0;
    unsigned long long all_chars = 0;

    while (std::getline(input, c_line))
    {
        std::size_t length = c_line.length();
        if (length == 0)
            continue;

        all_chars += length;
        if (length > longest_line)
            longest_line = length;
        ++line_counter;
    }
    input.clear();
    input.seekg(0);

    std::ofstream output;
    if (file_output)
        output.open(ip.output_file);

    if (!output)
    {
        input.close();
        std::cout << "Couldn't open the output file " << ip.output_file << '\n';
        return -1;
    }


    unsigned int passed_bytes = 0;
    unsigned int line_counter_Loop = 0;

    //For the inner loop
    int chars_per_line = BYTES_PER_LINE_M * 9 + BYTES_PER_LINE_M + 2 + 9;
    char one;
    std::string char_representation;
    std::string string_output;
    if (use_strings)
    {
        //estimate the required size
        int required_size;

        if (all_chars < longest_line) // overflow
            required_size = (((longest_line / line_counter) / BYTES_PER_LINE_M) * line_counter) * chars_per_line;
        else
            required_size = (((all_chars / line_counter) / BYTES_PER_LINE_M) * line_counter) * chars_per_line;
        required_size += line_counter;
        required_size += required_size / 4;

        string_output.reserve(required_size);
    }

    char_representation.reserve(BYTES_PER_LINE_M);

    std::string line_output;
    line_output.reserve(chars_per_line);
    auto byte_data = new char[BYTES_PER_LINE_M][9]();

    //for(size_t j = 0; j < lines_size; ++j)
    while (std::getline(input, c_line))
    {
        std::size_t line_size = c_line.size();
        if (line_size == 0)
            continue;

        // has new line
        if (c_line.back() == 13)
        {
            if (use_strings)
            {
                string_output.append("    ");
                string_output.append(DecToHexString(line_counter_Loop));
                string_output.append(":  ");
                string_output += BinaryStringToText(c_line);
            }

            else if(file_output)
                output << "    " << DecToHexString(line_counter_Loop) << ":  " << BinaryStringToText(c_line);

            else
                std::cout << "    " << DecToHexString(line_counter_Loop) << ":  " << BinaryStringToText(c_line);
        }
        else
        {
            if (use_strings)
            {
                string_output.append("    ");
                string_output.append(DecToHexString(line_counter_Loop));
                string_output.append(":  ");
                string_output += BinaryStringToText(c_line);
                string_output.append("\n");
            }
            
            else if(file_output)
                output << "    " << DecToHexString(line_counter_Loop) << ":  " << BinaryStringToText(c_line) << '\n';

            else
                std::cout << "    " << DecToHexString(line_counter_Loop) << ":  " << BinaryStringToText(c_line) << '\n';
        }

        // loop through every line
        for (std::size_t i = 0; i < line_size; ++i)
        {
            for (int k = 0; k < BYTES_PER_LINE_M; ++k)
            {
                char current_char = c_line.c_str()[i];
                charToByte(current_char, byte_data[k]);
                //add null termination character
                byte_data[k][8] = '\0';

                one = byteToInt(byte_data[k]);

                if (one < 33 || one > 126)
                    one = 46; // 46 == .

                char_representation += one;

                if (k != BYTES_PER_LINE_M - 1)
                    ++i;
            }
            //////////////////////////////////////////////////////////////
      
            // generate current line
            line_output.append(DecToHexString(passed_bytes));
            line_output.append(":  ");
            
            for(int l = 0; l < BYTES_PER_LINE_M; ++l)
            {
                line_output.append(byte_data[l]);
                line_output += ' ';
            }

            line_output.append(char_representation);
            line_output += '\n';


            if (use_strings)
                string_output.append(line_output);
            else if (file_output)
                output << line_output;
            else
                std::cout << line_output;


            passed_bytes += BYTES_PER_LINE_M;
            char_representation.clear();
            line_output.clear();
            //////////////////////////////////////////////////////////////
        }
        ++line_counter_Loop;
    }
    input.close();

    if (use_strings)
    {
        if (file_output)
            output << string_output;
        else
            std::cout << string_output;
    }
    delete[] byte_data;
    output.close();

#if _DEBUG
    std::cout << "\nLines: " << std::hex << line_counter << std::dec << " [" << line_counter << "]" << std::endl;
    std::cout << "Longest line: " << longest_line << std::endl;
    std::cout << "Allocations: " << allocations_G << std::endl;
#endif

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    std::cout << "Execution time: " << time_spent << " sec(s)\n";
}



/*
class DecToHex
{
public:
    unsigned long long m_Number;
    explicit DecToHex(unsigned long long number) : m_Number(number) {}
    friend std::ostream& operator<<(std::ostream& os, const DecToHex&& mh);
};
std::ostream& operator<<(std::ostream& os, const DecToHex&& mh)
{
    if (mh.m_Number < 0x10) //16
        return os << 0 << 0 << 0 << 0 << 0 << 0 << 0 << std::hex << mh.m_Number << std::dec; // 7x0
    else if (mh.m_Number < 0x100) // 256
        return os << 0 << 0 << 0 << 0 << 0 << 0 << std::hex << mh.m_Number << std::dec; // 6x0
    else if (mh.m_Number < 0x1000) // 4096
        return os << 0 << 0 << 0 << 0 << 0 << std::hex << mh.m_Number << std::dec; // 5x0
    else if (mh.m_Number < 0x10000) // 65536
        return os << 0 << 0 << 0 << 0 << std::hex << mh.m_Number << std::dec; // 4x0
    else if (mh.m_Number < 0x100000) // 1048576
        return os << 0 << 0 << 0 << std::hex << mh.m_Number << std::dec; // 3x0
    else if (mh.m_Number < 0x1000000) // 16777216
        return os << 0 << 0 << std::hex << mh.m_Number << std::dec; // 2x0
    else if (mh.m_Number < 0x10000000) // 268435456
        return os << 0 << std::hex << mh.m_Number << std::dec; // 1x0
    else
        return os << std::hex << mh.m_Number << std::dec; // 0x0
}
*/