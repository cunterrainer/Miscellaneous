#include <cctype>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <cstring>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <set>

#define PRINT(str)\
    std::cout << str

#define PRINT_NL(str)\
    std::cout << str << '\n'

#define MAX_NUM_WRONG 9

std::ostream& operator<<(std::ostream& os, const std::set<uint8_t>& set)
{
    os << "[ ";

    for(const auto i : set)
        os << i << ' ';

    os << "] " << set.size();
    return os;
}

inline void ltrim(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
}

inline void rtrim(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());
}

inline std::string trim(std::string str)
{
    rtrim(str);
    ltrim(str);
    return str;
}

class HangMan
{
private:
    char bar  = ' ';
    char head = ' ';
    char body = ' ';
    char arms = ' ';
    char legs = ' ';
    char hill = ' ';
    char strick = ' ';
    char sidebar = ' ';
    char m_MistakesMade = 0;
    uint8_t m_Guesses = 0;
    uint8_t m_RightGuesses = 0;
    uint32_t m_LinesInFile = 0;
    const char* m_FilePath;
    std::string m_Word;
    std::string m_DisplayedWord;
    uint8_t m_CorrectCharacters = 0;
    std::unordered_set<uint8_t> m_TriedChars;
    std::set<uint8_t> m_TriedCharsShow;
private:
    void PrintSideBar()
    {
        PRINT("      ");
        for(char i = 0; i < 35; ++i)
            PRINT(sidebar);
        PRINT_NL("");
    }

    void PrintVertical()
    {
        for(char i = 0; i < 20; ++i)
        {
            PRINT("      ");
            PRINT(bar);
            for(char j = 0; j < 22; ++j)
                PRINT(' ');

            // print head
            if(i < 3)
                PRINT("   " << strick);
            else if(i == 3 || i == 6)
                PRINT(' ' << head << head << head << head << head);
            else if(i == 4 || i == 5)
                PRINT(' ' << head << "   " << head);
            
            // print body && arms
            else if(i == 7 || i == 12)
                PRINT("   " << body);
            else if(i == 8)
                PRINT(' ' << arms << arms << body << arms << arms);
            else if(i == 9 || i == 10 || i == 11)
                PRINT(' ' << arms << ' ' << body << ' ' << arms);
            
            // print legs
            else if(i == 13)
                PRINT(' ' << legs << legs << body << legs << legs);
            else if(i == 14 || i == 15 || i == 16)
                PRINT(' '  << legs << "   " << legs);

            PRINT_NL("");
        }

    }

    void PrintHill()
    {
        PRINT_NL("    " << hill << hill << hill << hill << hill);
        PRINT_NL("   " << hill << "     " << hill);
        PRINT_NL("   " << hill << "     " << hill);
    }

    void PrintWord()
    {
        PRINT_NL("Guesses: " << static_cast<int>(m_Guesses));
        PRINT_NL("Correct: " << static_cast<int>(m_RightGuesses));
        PRINT_NL("Wrong: " << static_cast<int>(m_MistakesMade));
        PRINT_NL("Wrong guesses left: " << static_cast<int>(MAX_NUM_WRONG - m_MistakesMade));
        PRINT_NL("Already tried characters: " << m_TriedCharsShow);
        PRINT_NL("");
        PRINT("      ");
        PRINT_NL(m_DisplayedWord << ' ' << m_DisplayedWord.size() << " characters");
    }

    inline void ClearConsole() 
    { 
        #ifdef WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    char GetUserInput()
    {
        const size_t previousMapSize = m_TriedChars.size();

        std::string input;
        while(true)
        {
            while(input.size() == 0)
            {
                PRINT("Input a character: ");
                std::getline(std::cin, input);
                Print();
            }
            
            m_TriedCharsShow.insert(std::toupper(input[0]));
            m_TriedChars.insert(std::toupper(input[0]));
            m_TriedChars.insert(std::tolower(input[0]));
            if(m_TriedChars.size() == previousMapSize)
            {   
                Print();
                PRINT_NL("You've already tried this character");
                input.clear();
            }
            else
                break;
        }
        return input[0];
    }

    void CheckUserInput(char input)
    {
        input = std::tolower(input);
        uint8_t counter = 0;
        for(uint8_t i = 0; i < 2; ++i)
        {
            size_t pos = m_Word.find(input);

            while(pos != std::string::npos) // if found
            {
                ++counter;
                ++m_CorrectCharacters;

                m_DisplayedWord[pos] = m_Word[pos];
                size_t previousPos = pos;
                pos = m_Word.find(input, pos+1);
                if(pos <= previousPos)
                    break;
            }
            if(counter == 0 && i == 1)
                ++m_MistakesMade;
            else if (i == 1)
                ++m_RightGuesses;
            input = std::toupper(input);
        }
    }

    void ResetGame()
    {
        m_MistakesMade = 0;
        m_CorrectCharacters = 0;
        bar  = ' ';
        head = ' ';
        body = ' ';
        arms = ' ';
        legs = ' ';
        hill = ' ';
        sidebar = ' ';
        strick = ' ';

        m_TriedCharsShow.clear();
        m_TriedChars.clear();
        m_Word = trim(GetNewWord());
        m_DisplayedWord.reserve(m_Word.size());
        m_DisplayedWord.assign(m_Word.size(), '_');
    }

    bool RestartGame()
    {
        std::string input;
        while(input.size() == 0)
        {
            PRINT("Would you like to play again [Y|N]: ");
            std::getline(std::cin, input);
        }
        if(input[0] != 'Y' && input[0] != 'y')
            return false;
        ResetGame();
        return true;
    }

    void Print()
    {
        switch(m_MistakesMade)
        {
        case 0:
            break;
        case 1:
            hill = '#';
            break;
        case 2:
            bar = '#';
            break;
        case 3:
            sidebar = '#';
            break;
        case 4:
            strick = '#';
            break;
        case 5:
            head = '#';
            break;
        case 6:
            body = '#';
            break;
        case 7:
            arms = '#';
            break;
        case 8:
            legs = '#';
            break;
        }

        ClearConsole();
        PrintWord();
        PrintSideBar();
        PrintVertical();
        PrintHill();
    }

    uint32_t GetRandomNumber(uint32_t max)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, max);
        return distr(gen);
    }

    uint32_t GetLinesInFile(const char* path)
    {
        uint32_t numberOfLines = 0;
        FILE* infile = fopen(path, "r");
        int8_t ch;
        while((ch=getc(infile)) != EOF)
        {
            if(ch == '\n')
                ++numberOfLines;
        }
        return numberOfLines;
    }

    std::string GetNewWord()
    {
        uint32_t lineNum = GetRandomNumber(m_LinesInFile);
        uint32_t counter = 0;
        std::ifstream input(m_FilePath);
        std::string line;
        while(std::getline(input, line) && counter != lineNum)
            ++counter;
        
        input.close();
        return line;
    }
public:
    HangMan(const char* filepath) : m_FilePath(filepath)
    {
        m_LinesInFile = GetLinesInFile(filepath);
        ResetGame();
    }

    void Start()
    {
        while(true)
        {
            Print();
            char userInput = GetUserInput();
            CheckUserInput(userInput);
            ++m_Guesses;
            
            if(m_CorrectCharacters == m_Word.size())
            {
                Print();
                PRINT_NL("Congratulations you've won");
                if(!RestartGame())
                    break;
            }
            else if(m_MistakesMade == MAX_NUM_WRONG)
            {
                Print();
                PRINT_NL("You've lost, the word was: " << m_Word);
                if(!RestartGame())
                    break;
            }
        }
    }
};

int main()
{
    HangMan man("words.txt");
    man.Start();
}