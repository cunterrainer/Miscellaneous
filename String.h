#pragma once
#include <algorithm>
#include <limits>
#include <ostream>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <iterator>
#include <utility>
#include <memory>

#define STRING_INLINE inline
#define STRING_NODISCARD [[nodiscard]]

#define THROW_OUT_OF_RANGE throw std::out_of_range("basic_string subscription out of range")
#define THROW_LENGTH_ERROR throw std::length_error("Exceeded basic_string length limit")

#define CHECK_POS_LESS_EQ(pos) if (pos > m_Size)             THROW_OUT_OF_RANGE
#define CHECK_IN_BOUNDS(pos)   if (pos >= m_Size || empty()) THROW_OUT_OF_RANGE
#define CHECK_LENGTH_LIMIT(s)  if (s > max_size())           THROW_LENGTH_ERROR

#define STR_LN(str) strnlen(str, npos)


template <class _value_type, class _size_type, class _difference_type, class _pointer,
          class _const_pointer, class _reference, class _const_reference>
struct string_iter_types {
    using value_type      = _value_type;
    using size_type       = _size_type;
    using difference_type = _difference_type;
    using pointer         = _pointer;
    using const_pointer   = _const_pointer;
};


template <class val_types>
struct string_val
{
    using value_type      = typename val_types::value_type;
    using size_type       = typename val_types::size_type;
    using difference_type = typename val_types::difference_type;
    using pointer         = typename val_types::pointer;
    using const_pointer   = typename val_types::const_pointer;
    using reference       = value_type&;
    using const_reference = const value_type&;
};


template <class types>
class string_const_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = typename types::value_type;
    using difference_type   = typename types::difference_type;
    using pointer           = typename types::const_pointer;
    using reference         = const value_type&;

    pointer m_Ptr;
public:
    STRING_INLINE string_const_iterator() noexcept : m_Ptr() {}
    STRING_INLINE string_const_iterator(pointer ptr) noexcept : m_Ptr(ptr) {}

    STRING_NODISCARD STRING_INLINE reference operator*()  const noexcept { return *m_Ptr; }
    STRING_NODISCARD STRING_INLINE pointer   operator->() const noexcept { return std::pointer_traits<pointer>::pointer_to(**this); }

    STRING_INLINE string_const_iterator& operator++() noexcept 
    {
        ++m_Ptr;
        return *this;
    }

    STRING_INLINE string_const_iterator operator++(int) noexcept 
    {
        string_const_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    STRING_INLINE string_const_iterator& operator--() noexcept 
    {
        --m_Ptr;
        return *this;
    }

    STRING_INLINE string_const_iterator operator--(int) noexcept 
    {
        string_const_iterator tmp = *this;
        --*this;
        return tmp;
    }

    STRING_INLINE string_const_iterator& operator+=(const difference_type off) noexcept 
    {
        m_Ptr += off;
        return *this;
    }

    STRING_NODISCARD STRING_INLINE string_const_iterator operator+(const difference_type off) const noexcept
    {
        string_const_iterator tmp = *this;
        tmp += off;
        return tmp;
    }

    STRING_INLINE string_const_iterator& operator-=(const difference_type off) noexcept
    {
        return *this += -off;
    }

    STRING_NODISCARD STRING_INLINE string_const_iterator operator-(const difference_type off) const noexcept 
    {
        string_const_iterator tmp = *this;
        tmp -= off;
        return tmp;
    }

    STRING_NODISCARD STRING_INLINE difference_type operator- (const string_const_iterator& right) const noexcept { return m_Ptr - right.m_Ptr;  }
    STRING_NODISCARD STRING_INLINE reference       operator[](const difference_type off)          const noexcept { return *(*this + off);       }
    STRING_NODISCARD STRING_INLINE bool            operator==(const string_const_iterator& right) const noexcept { return m_Ptr == right.m_Ptr; }
    STRING_NODISCARD bool operator!=(const string_const_iterator& right) const noexcept { return !(*this == right);   }
    STRING_NODISCARD bool operator< (const string_const_iterator& right) const noexcept { return m_Ptr < right.m_Ptr; }
    STRING_NODISCARD bool operator> (const string_const_iterator& right) const noexcept { return right < *this;       }
    STRING_NODISCARD bool operator<=(const string_const_iterator& right) const noexcept { return !(right < *this);    }
    STRING_NODISCARD bool operator>=(const string_const_iterator& right) const noexcept { return !(*this < right);    }
};

template <class types>
STRING_NODISCARD STRING_INLINE string_const_iterator<types> operator+(typename string_const_iterator<types>::difference_type off, string_const_iterator<types> next) noexcept 
{
    next += off;
    return next;
}


template <class types>
class string_iterator : public string_const_iterator<types>
{
public:
    using mybase = string_const_iterator<types>;

    using iterator_category = std::random_access_iterator_tag;
    using value_type        = typename mybase::value_type;
    using difference_type   = typename mybase::difference_type;
    using pointer           = typename mybase::pointer;
    using reference         = value_type&;

    using mybase::mybase; // using constructors

    STRING_NODISCARD STRING_INLINE reference operator*()  const noexcept { return const_cast<reference>(mybase::operator*());       }
    STRING_NODISCARD STRING_INLINE pointer   operator->() const noexcept { return std::pointer_traits<pointer>::pointer_to(**this); }

    STRING_INLINE string_iterator& operator++() noexcept 
    {
        mybase::operator++();
        return *this;
    }

    STRING_INLINE string_iterator operator++(int) noexcept 
    {
        string_iterator tmp = *this;
        mybase::operator++();
        return tmp;
    }

    STRING_INLINE string_iterator& operator--() noexcept 
    {
        mybase::operator--();
        return *this;
    }

    STRING_INLINE string_iterator operator--(int) noexcept 
    {
        string_iterator tmp = *this;
        mybase::operator--();
        return tmp;
    }

    STRING_INLINE string_iterator& operator+=(const difference_type off) noexcept 
    {
        mybase::operator += (off);
        return *this;
    }

    STRING_NODISCARD STRING_INLINE string_iterator operator+(const difference_type off) const noexcept 
    {
        string_iterator tmp = *this;
        tmp += off;
        return tmp;
    }

    STRING_INLINE string_iterator& operator-=(const difference_type off) noexcept
    {
        mybase::operator -= (off);
        return *this;
    }

    using mybase::operator-;

    STRING_NODISCARD STRING_INLINE string_iterator operator-(const difference_type off) const noexcept 
    {
        string_iterator tmp = *this;
        tmp -= off;
        return tmp;
    }

    STRING_NODISCARD STRING_INLINE reference operator[](const difference_type off) const noexcept
    {
        return const_cast<reference>(mybase::operator[](off));
    }
};

template <class types>
STRING_NODISCARD STRING_INLINE string_iterator<types> operator+(typename string_iterator<types>::difference_type off, string_iterator<types> next) noexcept
{
    next += off;
    return next;
}


template <class Elem, class Traits = std::char_traits<Elem>, class Alloc = std::allocator<Elem>>
class basic_string
{
private:
    using alloc_traits = std::allocator_traits<Alloc>;

    using scary_val = string_val<string_iter_types<Elem, typename alloc_traits::size_type,
        typename alloc_traits::difference_type, typename alloc_traits::pointer,
        typename alloc_traits::const_pointer, Elem&, const Elem&>>;

    template <class string_view_ish>
    using is_string_view_ish = std::enable_if_t<std::conjunction_v<std::is_convertible<const string_view_ish&, std::basic_string_view<Elem, Traits>>,
        std::negation<std::is_convertible<const string_view_ish&, const Elem*>>>, int>;
public:
    using traits_type     = Traits;
    using allocator_type  = Alloc;
    using size_type       = typename alloc_traits::size_type;
    using difference_type = typename alloc_traits::difference_type;
    using pointer         = typename alloc_traits::pointer;
    using const_pointer   = typename alloc_traits::const_pointer;

    using value_type      = Elem;
    using reference       = value_type&;
    using const_reference = const value_type&;

    using iterator        = string_iterator<scary_val>;
    using const_iterator  = string_const_iterator<scary_val>;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type npos = std::numeric_limits<size_type>::max();
private:
    Alloc m_Alloc;
    pointer m_Str = nullptr;
    size_type m_Size = 0;
    size_type m_Capacity = 0;
private:
    STRING_INLINE void Deallocate()
    {
        const size_type cap = m_Capacity + 1;
        for (size_type i = 0; i < cap; ++i)
            alloc_traits::destroy(m_Alloc, &m_Str[i]);
        alloc_traits::deallocate(m_Alloc, m_Str, cap);
    }


    STRING_INLINE pointer Allocate(size_type size)
    {
        pointer tmp = alloc_traits::allocate(m_Alloc, size, 0);
        for (size_type i = 0; i < size; ++i)
            alloc_traits::construct(m_Alloc, &tmp[i], 0);
        return tmp;
    }


    STRING_INLINE void Realloc(size_type strlen, bool multiply_capacity = true)
    {
        const size_type cap = GetCapacity(strlen, multiply_capacity);
        if (cap == npos)
            return;

        pointer tmp = Allocate(cap + 1);
        std::memcpy(tmp, m_Str, m_Size);
        Deallocate();
        m_Str = tmp;
        m_Capacity = cap;
    }


    STRING_INLINE void ReallocIfNeeded(size_type strlen, bool multiply_capacity = true)
    {
        if (strlen > m_Capacity)
            Realloc(strlen, multiply_capacity);
    }


    STRING_INLINE size_type GetCapacity(size_type strlen, bool multiply_capacity)
    {
        size_type cap = strlen;
        if (multiply_capacity)
        {
            cap = std::max<size_type>(strlen + (strlen / 2), 2);
            if (cap < strlen) // overflow
                cap = max_size();
        }
        if (cap < m_Size || cap > max_size())
            return npos;
        return cap;
    }


    STRING_INLINE void Append(const_pointer str, size_type strlen)
    {
        const size_type newStrLen = m_Size + strlen;
        if (strlen == npos) return;

        ReallocIfNeeded(newStrLen);
        std::strncat(m_Str, str, strlen);
        m_Size = newStrLen;
    }


    template <class StrType>
    STRING_INLINE void Append(const StrType& str)
    {
        constexpr bool is_same_basic_string  = std::is_same_v<StrType, basic_string>;
        constexpr bool is_same_value_type    = std::is_same_v<StrType, value_type>;
        constexpr bool is_same_const_pointer = std::is_same_v<StrType, const_pointer>;
        static_assert(is_same_basic_string || is_same_value_type || is_same_const_pointer, "Invalid type for append");

        if constexpr (is_same_basic_string)
            Append(str.c_str(), str.size());
        else if constexpr (is_same_value_type)
            Append(&str, 1);
        else if constexpr (is_same_const_pointer)
            Append(str, STR_LN(str));
    }


    template <class InputIt>
    STRING_INLINE basic_string& AssignFromIterator(InputIt first, InputIt last)
    {
        const std::iterator_traits<InputIt>::difference_type dist = std::distance(first, last);
        ReallocIfNeeded(dist, false);

        m_Size = dist;
        size_type i = 0;
        while (first != last)
        {
            m_Str[i] = *first;
            ++first;
            ++i;
        }
        return *this;
    }


    STRING_INLINE basic_string& Assign(const_pointer str, size_type strlen)
    {
        if (strlen == npos) return *this;

        ReallocIfNeeded(strlen, false);
        std::memcpy(m_Str, str, strlen);
        std::memset(&m_Str[strlen], 0, m_Capacity - strlen);
        m_Size = strlen;
        return *this;
    }
    STRING_INLINE basic_string& Assign(value_type ch, size_type n) { return Assign(&ch, n);                  }
    STRING_INLINE basic_string& Assign(const basic_string& str)    { return Assign(str.c_str(), str.size()); }
    STRING_INLINE basic_string& Assign(const_pointer str)          { return Assign(str, STR_LN(str));        }
                                                                   

    STRING_INLINE void MoveFromBasicString(basic_string&& other, bool move_allocator = true) noexcept
    {
        if (move_allocator)
            m_Alloc = other.get_allocator();

        m_Str      = other.m_Str;
        m_Size     = other.m_Size;
        m_Capacity = other.m_Capacity;

        other.m_Str = nullptr;
        other.m_Size = 0;
        other.m_Capacity = 0;
    }
public:
    // Member functions
    // constructors -- done -- C++17
    basic_string() noexcept(noexcept(Alloc())) : basic_string(Alloc()) {}
    explicit basic_string(const Alloc& alloc) noexcept : m_Alloc(alloc) {}
    basic_string(size_type count, value_type ch, const Alloc& alloc = Alloc()) : m_Alloc(alloc) { insert(0, count, ch); }

    basic_string(const basic_string& other, size_type pos, const Alloc& alloc = Alloc()) : basic_string(other, pos, npos, alloc) {}
    basic_string(const basic_string& other, size_type pos, size_type count, const Alloc& alloc = Alloc()) : m_Alloc(alloc)
    {
        if (pos > other.size()) THROW_OUT_OF_RANGE;
        const size_type strlen = count == npos || count + pos > other.size() ? other.size() - pos : count;
        Assign(&other.c_str()[pos], strlen);
    }

    basic_string(const_pointer s, size_type count, const Alloc& alloc = Alloc()) : m_Alloc(alloc) { Assign(s, count); }
    basic_string(const_pointer s, const Alloc& alloc = Alloc()) : m_Alloc(alloc) { Assign(s); }

    template <class InputIt>
    basic_string(InputIt first, InputIt last, const Alloc& alloc = Alloc()) : m_Alloc(alloc) { AssignFromIterator(first, last); }

    basic_string(const basic_string& other) : basic_string(other, other.get_allocator()) {}
    basic_string(const basic_string& other, const Alloc& alloc) : m_Alloc(alloc) { Assign(other); }

    basic_string(basic_string&& other) noexcept : basic_string(other, other.get_allocator()) {}
    basic_string(basic_string&& other, const Alloc& alloc) : m_Alloc(alloc) { MoveFromBasicString(other, false); }

    basic_string(std::initializer_list<value_type> ilist, const Alloc& alloc = Alloc()) : basic_string(ilist.begin(), ilist.end(), alloc) {}

    template <class StringViewLike, is_string_view_ish<StringViewLike> = 0>
    explicit basic_string(const StringViewLike& t, const Alloc& alloc = Alloc()) : basic_string(t.data(), t.size(), alloc) {}

    template <class StringViewLike, std::enable_if_t<std::is_convertible_v<const StringViewLike&, std::basic_string_view<Elem, Traits>>, bool> = true>
    basic_string(const StringViewLike& t, size_type pos, size_type n, const Alloc& alloc = Alloc()) : basic_string(t.substr(pos, n), alloc) { if (pos > t.size()) THROW_OUT_OF_RANGE; }


    // assignment operators -- done -- C++17
    basic_string& operator=(const basic_string& str) { return Assign(str); }
    basic_string& operator=(basic_string&& str) noexcept
    {
        if constexpr (alloc_traits::propagate_on_container_move_assignment::value)
            MoveFromBasicString(std::forward<basic_string>(str), true);
        else if constexpr (!alloc_traits::propagate_on_container_move_assignment::value)
            Assign(str);
        return *this;
    }
    basic_string& operator=(const_pointer s) { return Assign(s);     }
    basic_string& operator=(value_type ch)   { return Assign(ch, 1); }
    basic_string& operator=(std::initializer_list<value_type> ilist) { return AssignFromIterator(ilist.begin(), ilist.end()); }
    template<class StringViewLike, is_string_view_ish<StringViewLike> = 0>
    basic_string& operator=(const StringViewLike& t) { return Assign(t.data()); }


    STRING_INLINE basic_string& assign(const_pointer str)       { return Assign(str); }
    STRING_INLINE basic_string& assign(const basic_string& str) { return Assign(str); }
    STRING_INLINE ~basic_string() noexcept { Deallocate(); }
    STRING_INLINE allocator_type get_allocator() const { return m_Alloc; }


    // element access -- done -- C++17
    STRING_INLINE reference       at(size_type pos)               { CHECK_IN_BOUNDS(pos); return m_Str[pos]; }
    STRING_INLINE const_reference at(size_type pos) const         { CHECK_IN_BOUNDS(pos); return m_Str[pos]; }
    STRING_INLINE reference       operator[](size_type pos)       { return at(pos); }
    STRING_INLINE const_reference operator[](size_type pos) const { return at(pos); }
    STRING_INLINE reference       front()                         { return at(0); }
    STRING_INLINE const_reference front() const                   { return at(0); }
    STRING_INLINE reference       back()                          { return at(size() - 1); }
    STRING_INLINE const_reference back()  const                   { return at(size() - 1); }
    STRING_INLINE pointer         data()  noexcept                { return m_Str; }
    STRING_INLINE const_pointer   data()  const noexcept          { return m_Str; }
    STRING_INLINE const_pointer   c_str() const noexcept          { return m_Str; }
    STRING_INLINE operator std::basic_string_view<Elem, Traits>() const noexcept { return { m_Str, m_Size }; }


    // capacity -- done -- C++17
    STRING_NODISCARD STRING_INLINE bool      empty()    const noexcept { return m_Size == 0;     }
    STRING_NODISCARD STRING_INLINE size_type size()     const noexcept { return m_Size;          }
    STRING_NODISCARD STRING_INLINE size_type length()   const noexcept { return m_Size;          }
    STRING_NODISCARD constexpr     size_type max_size() const noexcept { return npos ;        }
    STRING_NODISCARD STRING_INLINE size_type capacity() const noexcept { return m_Capacity;      }
                     STRING_INLINE void shrink_to_fit()                { Realloc(m_Size, false); }
    STRING_INLINE void reserve(size_type new_cap = 0)
    {
        CHECK_LENGTH_LIMIT(new_cap);
        if (new_cap == m_Capacity) return;
        if (new_cap <= m_Size)
            shrink_to_fit();
        Realloc(new_cap, false);
    }


    // operations
    STRING_INLINE void clear() noexcept { std::memset(m_Str, 0, m_Size); m_Size = 0; }
    // not fully implemented
    STRING_INLINE basic_string& insert(size_type index, size_type count, value_type ch)
    {
        const size_t new_size = m_Size + count;
        ReallocIfNeeded(new_size);
        
        CHECK_POS_LESS_EQ(index);
        std::memmove(&m_Str[index + count], &m_Str[index], m_Size - index);
        std::memset(&m_Str[index], ch, count);
        m_Size = new_size;
        return *this;
    }


    // not fully implemented
    STRING_INLINE basic_string& erase(size_type index = 0, size_type count = npos)
    {
        if (count == 0)
            return *this;
        if (count == npos || (index + count) > m_Size)
        {
            clear();
            return *this;
        }
        CHECK_POS_LESS_EQ(index);

        std::memset (&m_Str[index], 0, count);
        std::memmove(&m_Str[index], &m_Str[index + count], m_Size - index - count);
        m_Size -= count;
        std::memset (&m_Str[m_Size], 0, m_Capacity - m_Size);
        return *this;
    }


    // not fully implemented
    STRING_INLINE basic_string& append(value_type str)          { Append(str);         return *this; }
    STRING_INLINE basic_string& append(const_pointer str)       { Append(str);         return *this; }
    STRING_INLINE basic_string& append(const basic_string& str) { Append(str.c_str()); return *this; }


    // iterators -- done -- C++17
    STRING_NODISCARD STRING_INLINE iterator       begin() noexcept       { return iterator(m_Str);                                              }
    STRING_NODISCARD STRING_INLINE const_iterator begin() const noexcept { return const_iterator(m_Str);                                        }
    STRING_NODISCARD STRING_INLINE iterator       end()   noexcept       { return iterator(m_Str + static_cast<difference_type>(m_Size));       }
    STRING_NODISCARD STRING_INLINE const_iterator end()   const noexcept { return const_iterator(m_Str + static_cast<difference_type>(m_Size)); }

    STRING_NODISCARD STRING_INLINE reverse_iterator       rbegin()  noexcept       { return reverse_iterator(end());         }
    STRING_NODISCARD STRING_INLINE const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end());   }
    STRING_NODISCARD STRING_INLINE reverse_iterator       rend()    noexcept       { return reverse_iterator(begin());       }
    STRING_NODISCARD STRING_INLINE const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }

    STRING_NODISCARD STRING_INLINE const_iterator         cbegin()  const noexcept { return begin();  }
    STRING_NODISCARD STRING_INLINE const_iterator         cend()    const noexcept { return end();    }
    STRING_NODISCARD STRING_INLINE const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    STRING_NODISCARD STRING_INLINE const_reverse_iterator crend()   const noexcept { return rend();   }



    // Non-member functions
    inline friend std::ostream& operator<<(std::ostream& os, const basic_string& str) noexcept
    {
        if (str.m_Str == nullptr)
            os << "";
        else
            os << str.m_Str;
        return os;
    }
};


using string = basic_string<char>;

/*
    Member types -- done -- C++17
    element access -- done -- C++17
    iterators -- done -- C++17
    capacity -- done -- C++17
*/