#pragma once
#include <algorithm>
#include <limits>
#include <ostream>
#include <cstring>
#include <string_view>
#include <type_traits>

#define STRING_INLINE inline
#define CHECK_POS_LESS_EQ(pos) assert(pos <= m_Size && "String subscription out of range")
#define STR_LN(str) strnlen(str, npos)


template <class Ty, class alloc = std::allocator<Ty>>
class basic_string
{
public:
    using allocator_type = alloc;
    using alloc_traits   = std::allocator_traits<alloc>;
    using size_type      = typename alloc_traits::size_type;
    using pointer        = typename alloc_traits::pointer;
    using const_pointer  = typename alloc_traits::const_pointer;

    using value_type      = Ty;
    using reference       = value_type&;
    using const_reference = const value_type&;

    static constexpr size_type npos = std::numeric_limits<size_type>::max();
private:
    alloc m_Alloc;
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


    STRING_INLINE void ReallocIfNeeded(size_type strlen)
    {
        if (strlen > m_Capacity)
            Realloc(strlen);
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


    STRING_INLINE void Assign(const_pointer str, size_type strlen)
    {
        if (strlen == npos) return;

        ReallocIfNeeded(strlen);
        std::memcpy(m_Str, str, strlen);
        std::memset(&m_Str[strlen], 0, m_Capacity - strlen);
        m_Size = strlen;
    }


    template <class StrType>
    STRING_INLINE void Assign(const StrType& str)
    {
        constexpr bool is_same_basic_string  = std::is_same_v<StrType, basic_string>;
        constexpr bool is_same_const_pointer = std::is_same_v<StrType, const_pointer>;
        static_assert(is_same_basic_string || is_same_const_pointer, "Invalid type for assign");

        if constexpr (is_same_basic_string)
            Assign(str.c_str(), str.size());
        else if constexpr (is_same_const_pointer)
            Assign(str, STR_LN(str));
    }
public:
    // Member functions
    constexpr basic_string() = default;
    STRING_INLINE basic_string(const_pointer str)
    {
        Append(str);
    }

    STRING_INLINE basic_string& assign(const_pointer str)       { Assign(str); return *this; }
    STRING_INLINE basic_string& assign(const basic_string& str) { Assign(str); return *this; }
    STRING_INLINE ~basic_string() noexcept { Deallocate(); }
    STRING_INLINE allocator_type get_allocator() const { return m_Alloc; }


    // element access -- done
    STRING_INLINE reference       at(size_type pos)               { CheckPos(pos); return m_Str[pos]; }
    STRING_INLINE const_reference at(size_type pos) const         { CheckPos(pos); return m_Str[pos]; }
    STRING_INLINE reference       operator[](size_type pos)       { return at(pos); }
    STRING_INLINE const_reference operator[](size_type pos) const { return at(pos); }
    STRING_INLINE reference       front()                         { return at(0); }
    STRING_INLINE const_reference front()     const               { return at(0); }
    STRING_INLINE reference       back()                          { return at(size() - 1); }
    STRING_INLINE const_reference back()      const               { return at(size() - 1); }
    STRING_INLINE const_pointer   data()      const noexcept      { return m_Str; }
    STRING_INLINE const_pointer   c_str()     const noexcept      { return m_Str; }
    STRING_INLINE operator std::string_view() const noexcept      { return { m_Str, m_Size }; }


    // capacity -- done
    STRING_INLINE bool      empty()    const noexcept { return m_Size == 0;     }
    STRING_INLINE size_type size()     const noexcept { return m_Size;          }
    STRING_INLINE size_type length()   const noexcept { return m_Size;          }
    constexpr     size_type max_size() const noexcept { return npos - 1;        }
    STRING_INLINE size_type capacity() const noexcept { return m_Capacity;      }
    STRING_INLINE void      shrink_to_fit()           { Realloc(m_Size, false); }
    STRING_INLINE void      reserve(size_type new_cap)
    {
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
        if (count == npos)
        {
            clear();
            return *this;
        }

        CHECK_POS_LESS_EQ(count);
        std::memset (&m_Str[index], 0, count);
        std::memmove(&m_Str[index], &m_Str[index + count], m_Size - index - count);
        m_Size -= count;

        if(count != 0)
            std::memset (&m_Str[m_Size], 0, m_Capacity - m_Size);
        return *this;
    }


    // not fully implemented
    STRING_INLINE basic_string& append(value_type str)          { Append(str);         return *this; }
    STRING_INLINE basic_string& append(const_pointer str)       { Append(str);         return *this; }
    STRING_INLINE basic_string& append(const basic_string& str) { Append(str.c_str()); return *this; }


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


using String = basic_string<char>;