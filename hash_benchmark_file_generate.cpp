#include <string_view>
#include <functional>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <format>
#include <utility>

#define NUMBER_OF_STRINGS   100
#define MAX_STRING_SIZE     1000000
#define GENERATE_RND_LENGTH 0

typedef std::vector<char> char_array;
char_array charset()
{
    //Change this to suit
    return char_array(
        { '0','1','2','3','4',
        '5','6','7','8','9',
        'A','B','C','D','E','F',
        'G','H','I','J','K',
        'L','M','N','O','P',
        'Q','R','S','T','U',
        'V','W','X','Y','Z',
        'a','b','c','d','e','f',
        'g','h','i','j','k',
        'l','m','n','o','p',
        'q','r','s','t','u',
        'v','w','x','y','z'
        });
};

// given a function that generates a random character,
// return a string of the requested length
std::string random_string(size_t length, std::function<char(void)> rand_char)
{
    std::string str(length, 0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
}


void GenerateStrings(size_t count)
{
    const auto ch_set = charset();

    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, ch_set.size() - 1);
    auto randchar = [ch_set, &dist, &rng]() {return ch_set[dist(rng)]; };

    auto rnum = [](unsigned int min, unsigned int max) {
        static std::random_device dev;
        static std::mt19937 mrng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(min, max); // distribution in range [1, 6]
        return dist6(mrng);
        };
        
    for (size_t i = 0; i < count; ++i)
    {
        #if GENERATE_RND_LENGTH
            std::cout << random_string(rnum(0, MAX_STRING_SIZE), randchar) << std::endl;
        #else
            std::cout << random_string(MAX_STRING_SIZE, randchar) << std::endl;
        #endif
    }
}


int main()
{
    std::ios_base::sync_with_stdio(false);
    GenerateStrings(NUMBER_OF_STRINGS);
}