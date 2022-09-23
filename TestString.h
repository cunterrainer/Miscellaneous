#pragma once
#include <string>
#include <random>
#include "String.h"

#define CHECK(s, o) if (s != o) assert(false)

namespace test
{
    int GetRandomNum(int min, int max)
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(min, max);
        return dist6(rng);
    }


    bool Same(const std::string& std_str, const string& str)
    {
        if (std_str.size() != str.size())
            return false;

        for (size_t i = 0; i < std_str.size(); ++i)
        {
            if (std_str[i] != str[i])
                return false;
        }
        return true;
    }


    struct S
    {
        std::string std;
        string os;
    };


    template <class... Args>
    S Construct(Args&&... args)
    {
        std::string std(std::forward<Args>(args)...);
        string os(std::forward<Args>(args)...);
        return { std, os };
    }


    template <class T>
    S ConstructT(std::initializer_list<T> t)
    {
        std::string std(t);
        string os(t);
        return { std, os };
    }


    void PrintInfo(const S& s)
    {
        std::cout << "std::string: " << s.std << " size: " << s.std.size() << " capacity: " << s.std.capacity() << std::endl;
        std::cout << "string: " << s.os << " size: " << s.os.size() << " capacity: " << s.os.capacity() << std::endl;
    }


    void Compare(const S& s)
    {
        if (!Same(s.std, s.os))
        {
            std::cout << "Test failed: \n";
            PrintInfo(s);
            std::cout << std::endl;
            assert(false);
        }
    }


    void Compare(const std::string& std_str, const string& str)
    {
        Compare({ std_str, str });
    }


    void Constructors()
    {
        // numbers according to constructors on cppreference.com
        // 2
        Compare(Construct(GetRandomNum(0, 100), (char)GetRandomNum(65, 125)));

        // 3
        {
            std::string s1("Hello");
            string o1("Hello");
            Compare(s1, o1);
            for (int i = 0; i < s1.size(); ++i)
            {
                int num = s1.size() - 1;
                std::string ss1(s1, i, num);
                string oo1(o1, i, num);
                Compare(ss1, oo1);
            }
        }

        // 4
        {
            Compare(Construct(""));
            Compare(Construct("12"));
            Compare(Construct("gjakfjkajfacvkanmsc"));

            std::string stdh("Hello");
            string ownh("Hello");
            Compare(stdh, ownh);
            for (int i = 0; i < stdh.size(); ++i)
            {
                std::string s1("Hello", i);
                string o1("Hello", i);
                Compare(s1, o1);
            }
        }


        // 5
        {
            std::string it1 = "";
            std::string it2 = "12";
            std::string it3 = "gjakfjkajfacvkanmsc";
            Compare(Construct(it1.begin(), it1.end()));
            Compare(Construct(it2.begin(), it2.end()));
            Compare(Construct(it3.begin(), it3.end()));
        }


        // 6
        {
            S s1 = Construct("");
            S s2 = Construct("12");
            S s3 = Construct("gjakfjkajfacvkanmsc");

            std::string ss1(s1.std);
            std::string ss2(s2.std);
            std::string ss3(s3.std);
            string oo1(s1.os);
            string oo2(s2.os);
            string oo3(s3.os);

            Compare(ss1, oo1);
            Compare(ss2, oo2);
            Compare(ss3, oo3);
        }


        // 7
        {
            S s1 = Construct("");
            S s2 = Construct("12");
            S s3 = Construct("gjakfjkajfacvkanmsc");

            std::string ss1(std::move(s1.std));
            std::string ss2(std::move(s2.std));
            std::string ss3(std::move(s3.std));
            string oo1(std::move(s1.os));
            string oo2(std::move(s2.os));
            string oo3(std::move(s3.os));

            Compare(ss1, oo1);
            Compare(ss2, oo2);
            Compare(ss3, oo3);
        }


        // 8
        {
            Compare(ConstructT({ ' '}));
            Compare(ConstructT({ 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u' }));
            Compare(ConstructT({ 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u', 'u' }));
        }


        // 9
        {
            std::string_view sv1;
            std::string_view sv2 = "aksdjkas";
            std::string_view sv3 = "asdjkasdaksdjkas";

            Compare(Construct(sv1));
            Compare(Construct(sv2));
            Compare(Construct(sv3));
        }


        // 10
        {
            std::string_view sv1 = "ajskaj";
            std::string_view sv2 = "aksdjkas";
            std::string_view sv3 = "asdjkasdaksdjkas";

            Compare(Construct(sv1, 3, 6));
            Compare(Construct(sv2, 3, 6));
            Compare(Construct(sv3, 3, 6));
        }
        std::cout << "Passed constructor tests" << std::endl;
    }


    void ElemAccess()
    {
        std::string s1 = "akfknveuacakvnueueowadlakdlscksnvkshfkjskajiwaxkack";
        string o1 = "akfknveuacakvnueueowadlakdlscksnvkshfkjskajiwaxkack";

        CHECK(s1.front(), o1.front());
        CHECK(s1.back(), o1.back());

        for (int i = 0; i < s1.size(); ++i)
        {
            CHECK(s1.at(i), o1.at(i));
            CHECK(s1[i], o1[i]);
        }
    }


    void Capacity()
    {
        std::vector<S> vec;
        vec.push_back(Construct(""));
        vec.push_back(Construct("isaifhaklnvkeishjiaödanvansveö"));
        vec.push_back(Construct("isaifhaklnaddadvkeishjiaödanvansveö"));
        vec.push_back(Construct("isaifhakwrafwaxlnvkeishjiaödanvansveö"));
        vec.push_back(Construct("isaifhaklnvvasfakeishjiaödanvansveö"));
        vec.push_back(Construct("isaivkeishjdanvansveö"));

        for (const auto& i : vec)
        {
            CHECK(i.std.empty(), i.os.empty());
            CHECK(i.std.size(), i.os.size());
            CHECK(i.std.length(), i.os.length());
        }
    }


    void Iterator()
    {

    }
}