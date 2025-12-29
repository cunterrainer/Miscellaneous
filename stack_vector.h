#ifndef STACK_VECTOR_H
#define STACK_VECTOR_H

// At least C++ 17 needed

#include <cstddef>
#include <cassert>
#include <utility>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

// Forward declaration for get declarations
template <class T, std::size_t N>
class stack_vector;

// Forward declarations for friend declarations in stack_vector class
template <std::size_t I, class Ty, std::size_t Nn>
constexpr Ty& get(stack_vector<Ty, Nn>& arr) noexcept;

template <std::size_t I, class Ty, std::size_t Nn>
constexpr const Ty& get(const stack_vector<Ty, Nn>& arr) noexcept;

template <std::size_t I, class Ty, std::size_t Nn>
constexpr Ty&& get(stack_vector<Ty, Nn>&& arr) noexcept;

template <std::size_t I, class Ty, std::size_t Nn>
constexpr const Ty&& get(const stack_vector<Ty, Nn>&& arr) = delete;


#define STACK_VECTOR_NODISCARD [[nodiscard]]

template <class T, std::size_t N>
class stack_vector_const_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const T*;
    using reference         = const T&;
private:
    pointer m_Ptr; // beginning of array
public:
    constexpr stack_vector_const_iterator() noexcept : m_Ptr() {}
    constexpr explicit stack_vector_const_iterator(pointer parg, std::size_t off = 0) noexcept : m_Ptr(parg + off) {}

    STACK_VECTOR_NODISCARD constexpr reference operator*() const noexcept
    {
        return *m_Ptr;
    }

    STACK_VECTOR_NODISCARD constexpr pointer operator->() const noexcept
    {
        return m_Ptr;
    }

    constexpr stack_vector_const_iterator& operator++() noexcept
    {
        ++m_Ptr;
        return *this;
    }

    constexpr stack_vector_const_iterator operator++(int) noexcept
    {
        stack_vector_const_iterator tmp = *this;
        ++m_Ptr;
        return tmp;
    }

    constexpr stack_vector_const_iterator& operator--() noexcept
    {
        --m_Ptr;
        return *this;
    }

    constexpr stack_vector_const_iterator operator--(int) noexcept
    {
        stack_vector_const_iterator tmp = *this;
        --m_Ptr;
        return tmp;
    }

    constexpr stack_vector_const_iterator& operator+=(const std::ptrdiff_t off) noexcept
    {
        m_Ptr += off;
        return *this;
    }

    constexpr stack_vector_const_iterator& operator-=(const std::ptrdiff_t off) noexcept
    {
        m_Ptr -= off;
        return *this;
    }

    STACK_VECTOR_NODISCARD constexpr std::ptrdiff_t operator-(const stack_vector_const_iterator& right) const noexcept
    {
        return m_Ptr - right.m_Ptr;
    }

    STACK_VECTOR_NODISCARD constexpr reference operator[](const std::ptrdiff_t off) const noexcept
    {
        return m_Ptr[off];
    }

    STACK_VECTOR_NODISCARD constexpr bool operator==(const stack_vector_const_iterator& right) const noexcept
    {
        return m_Ptr == right.m_Ptr;
    }


    STACK_VECTOR_NODISCARD constexpr bool operator<(const stack_vector_const_iterator& right) const noexcept
    {
        return m_Ptr < right.m_Ptr;
    }

    STACK_VECTOR_NODISCARD constexpr stack_vector_const_iterator operator+(const std::ptrdiff_t off) const noexcept
    {
        stack_vector_const_iterator tmp = *this;
        tmp += off;
        return tmp;
    }

    STACK_VECTOR_NODISCARD constexpr stack_vector_const_iterator operator-(const std::ptrdiff_t off) const noexcept
    {
        stack_vector_const_iterator tmp = *this;
        tmp -= off;
        return tmp;
    }

    STACK_VECTOR_NODISCARD constexpr bool operator!=(const stack_vector_const_iterator& right) const noexcept
    {
        return !(*this == right);
    }

    STACK_VECTOR_NODISCARD constexpr bool operator>(const stack_vector_const_iterator& right) const noexcept
    {
        return right < *this;
    }

    STACK_VECTOR_NODISCARD constexpr bool operator<=(const stack_vector_const_iterator& right) const noexcept
    {
        return !(right < *this);
    }

    STACK_VECTOR_NODISCARD constexpr bool operator>=(const stack_vector_const_iterator& right) const noexcept
    {
        return !(*this < right);
    }
};

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD constexpr stack_vector_const_iterator<T, N> operator+(const std::ptrdiff_t off, stack_vector_const_iterator<T, N> next) noexcept
{
    next += off;
    return next;
}


template <class T, std::size_t N>
class stack_vector_iterator : public stack_vector_const_iterator<T, N> {
public:
    using mybase = stack_vector_const_iterator<T, N>;

    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    constexpr stack_vector_iterator() noexcept {}
    constexpr explicit stack_vector_iterator(pointer parg, std::size_t off = 0) noexcept : mybase(parg, off) {}

    STACK_VECTOR_NODISCARD constexpr reference operator*() const noexcept
    {
        return const_cast<reference>(mybase::operator*());
    }

    STACK_VECTOR_NODISCARD constexpr pointer operator->() const noexcept
    {
        return const_cast<pointer>(mybase::operator->());
    }

    constexpr stack_vector_iterator& operator++() noexcept
    {
        mybase::operator++();
        return *this;
    }

    constexpr stack_vector_iterator operator++(int) noexcept
    {
        stack_vector_iterator tmp = *this;
        mybase::operator++();
        return tmp;
    }

    constexpr stack_vector_iterator& operator--() noexcept
    {
        mybase::operator--();
        return *this;
    }

    constexpr stack_vector_iterator operator--(int) noexcept
    {
        stack_vector_iterator tmp = *this;
        mybase::operator--();
        return tmp;
    }

    constexpr stack_vector_iterator& operator+=(const std::ptrdiff_t off) noexcept
    {
        mybase::operator+=(off);
        return *this;
    }

    STACK_VECTOR_NODISCARD constexpr stack_vector_iterator operator+(const std::ptrdiff_t off) const noexcept
    {
        stack_vector_iterator tmp = *this;
        tmp += off;
        return tmp;
    }

    constexpr stack_vector_iterator& operator-=(const std::ptrdiff_t off) noexcept
    {
        mybase::operator-=(off);
        return *this;
    }

    using mybase::operator-;

    STACK_VECTOR_NODISCARD constexpr stack_vector_iterator operator-(const std::ptrdiff_t off) const noexcept
    {
        stack_vector_iterator tmp = *this;
        tmp -= off;
        return tmp;
    }

    STACK_VECTOR_NODISCARD constexpr reference operator[](const std::ptrdiff_t off) const noexcept
    {
        return const_cast<reference>(mybase::operator[](off));
    }
};

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD constexpr stack_vector_iterator<T, N> operator+(const std::ptrdiff_t off, stack_vector_iterator<T, N> next) noexcept
{
    next += off;
    return next;
}


/*
    @brief An entirely stack allocated array following the C++ 17 Standart but with std::vector utility functions e.g.
    push_back: Like std::vector::push_back
    emplace_back: Like std::vector::emplace_back
    size: Like std::vector::size
    empty: Like std::vector::empty
    etc.
*/
template <class T, std::size_t N>
class stack_vector
{
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    using iterator       = stack_vector_iterator<T, N>;
    using const_iterator = stack_vector_const_iterator<T, N>;
    
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
    T m_Elems[N];
    size_type m_Size = 0;
public:
    template <class... U, std::enable_if_t<(sizeof...(U) <= N) && std::conjunction_v<std::is_convertible<U, T>...>, int> = 0>
    constexpr stack_vector(U&&... args) noexcept : m_Elems{ std::forward<U>(args)... }, m_Size(sizeof...(U)) {}

    // element access -- done -- C++17
    constexpr reference at(size_type pos)
    {
        if (pos >= m_Size)
            throw std::out_of_range("Invalid stack_vector<T, N> subscript");
        return m_Elems[pos];
    }

    constexpr const_reference at(size_type pos) const
    {
        if (pos >= m_Size)
            throw std::out_of_range("Invalid stack_vector<T, N> subscript");
        return m_Elems[pos];
    }

    constexpr reference operator[](size_type pos)
    {
        assert(!(pos >= m_Size && pos < N) && "stack_vector::operator[], parameter pos should be less than number of elements in stack_vector, if the use was intentional, no need to worry about it!");
        assert(pos < N && "stack_vector::operator[], parameter pos should be less than max_size() of stack_vector!");
        return m_Elems[pos];
    }

    constexpr const_reference operator[](size_type pos) const
    {
        assert(!(pos >= m_Size && pos < N) && "stack_vector::operator[], parameter pos should be less than number of elements in stack_vector, if the use was intentional, no need to worry about it!");
        assert(pos < N && "stack_vector::operator[], parameter pos should be less than max_size() of stack_vector!");
        return m_Elems[pos];
    }

    constexpr reference front()
    {
        return m_Elems[0];
    }

    constexpr const_reference front() const
    {
        return m_Elems[0];
    }

    constexpr reference back()
    {
        return m_Elems[m_Size - 1];
    }

    constexpr const_reference back() const
    {
        return m_Elems[m_Size - 1];
    }

    constexpr T* data() noexcept
    {
        return m_Elems;
    }

    constexpr const T* data() const noexcept
    {
        return m_Elems;
    }


    // iterators -- done -- C++17
    STACK_VECTOR_NODISCARD constexpr iterator begin() noexcept
    {
        return iterator(m_Elems, 0);
    }
    
    STACK_VECTOR_NODISCARD constexpr const_iterator begin() const noexcept
    {
        return const_iterator(m_Elems, 0);
    }
    
    STACK_VECTOR_NODISCARD constexpr iterator end() noexcept
    {
        return iterator(m_Elems, m_Size);
    }
    
    STACK_VECTOR_NODISCARD constexpr const_iterator end() const noexcept
    {
        return const_iterator(m_Elems, m_Size);
    }
    
    STACK_VECTOR_NODISCARD constexpr reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }
    
    STACK_VECTOR_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    
    STACK_VECTOR_NODISCARD constexpr reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }
    
    STACK_VECTOR_NODISCARD constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    
    STACK_VECTOR_NODISCARD constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }
    
    STACK_VECTOR_NODISCARD constexpr const_iterator cend() const noexcept
    {
        return end();
    }
    
    STACK_VECTOR_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }
    
    STACK_VECTOR_NODISCARD constexpr const_reverse_iterator crend() const noexcept
    {
        return rend();
    }


    // capacity -- done -- C++17
    constexpr bool empty() const noexcept
    {
        return m_Size == 0;
    }

    constexpr size_type size() const noexcept
    {
        return m_Size;
    }

    constexpr size_type max_size() const noexcept
    {
        return N;
    }

    constexpr size_type capacity() const noexcept
    {
        return N;
    }


    // Modifiers
    constexpr void clear() noexcept
    {
        m_Size = 0;
    }

    constexpr bool push_back(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        static_assert(std::is_copy_assignable_v<T>, "stack_vector::push_back(const T& value): T must be copy assignable!");
        if (m_Size >= N)
        {
            assert(false && "stack_vector::push_back<T>(const T& value): Max number of elements already satisfied!");
            return false;
        }
        m_Elems[m_Size++] = value;
        return true;
    }

    constexpr bool push_back(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        static_assert(std::is_move_assignable_v<T>, "stack_vector::push_back(T&& value): T must be move assignable!");
        if (m_Size >= N)
        {
            assert(false && "stack_vector::push_back<T>(T&& value): Max number of elements already satisfied!");
            return false;
        }
        m_Elems[m_Size++] = std::move(value);
        return true;
    }

    template <class... Args>
    constexpr bool emplace_back(Args&&... args) noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        static_assert(std::is_assignable_v<T&, T>, "stack_vector::emplace_back<T>(Args&&... args): T must be assignable!");
        if (m_Size >= N)
        {
            assert(false && "stack_vector::emplace_back<T>(Args&&... args): Max number of elements already satisfied!");
            return false;
        }
        m_Elems[m_Size++] = T(std::forward<Args>(args)...);
        return true;
    }

    constexpr void pop_back() noexcept
    {
        assert(!empty() && "stack_vector::pop_back(): Container is empty");
        --m_Size;
    }

    constexpr void resize(size_type count, const value_type& value) noexcept(std::is_nothrow_copy_assignable_v<T>)
    {
        assert(count <= N && "stack_vector::resize(size_type count, const value_type& value): parameter count has to be less than max_size()!");
        if (count > m_Size)
        {
            std::fill_n(begin() + m_Size, count - m_Size, value);
        }
        m_Size = count;
    }

    constexpr void resize(size_type count) noexcept(std::is_nothrow_default_constructible_v<T>&& std::is_nothrow_copy_assignable_v<T>)
    {
        assert(count <= N && "stack_vector::resize(size_type count): parameter count has to be less than max_size()!");
        resize(count, T{});
    }

    constexpr bool insert(size_type pos, const T& value) noexcept(std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_move_assignable_v<T>)
    {
        if (m_Size >= N)
        {
            assert(false && "stack_vector::insert(size_type pos, const T& value): Already at full capacity!");
            return false;
        }

        if (pos > m_Size)
        {
            assert(false && "stack_vector::insert(size_type pos, const T& value): Parameter pos is an invalid position!");
            return false;
        }

        // Shift elements right
        for (size_type i = m_Size; i > pos; --i)
            m_Elems[i] = std::move(m_Elems[i - 1]);

        m_Elems[pos] = value;
        m_Size++;
        return true;
    }

    constexpr bool insert(size_type pos, T&& value) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (m_Size >= N)
        {
            assert(false && "stack_vector::insert(size_type pos, T&& value): Already at full capacity!");
            return false;
        }

        if (pos > m_Size)
        {
            assert(false && "stack_vector::insert(size_type pos, T&& value): Parameter pos is an invalid position!");
            return false;
        }

        for (size_type i = m_Size; i > pos; --i)
            m_Elems[i] = std::move(m_Elems[i - 1]);

        m_Elems[pos] = std::move(value);
        m_Size++;
        return true;
    }

    template <class... Args>
    constexpr bool emplace(size_type pos, Args&&... args) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        if (m_Size >= N)
        {
            assert(false && "stack_vector::emplace(size_type pos, Args&&... args): Already at full capacity!");
            return false;
        }

        if (pos > m_Size)
        {
            assert(false && "stack_vector::emplace(size_type pos, Args&&... args): Parameter pos is an invalid position!");
            return false;
        }

        for (size_type i = m_Size; i > pos; --i)
            m_Elems[i] = std::move(m_Elems[i - 1]);

        m_Elems[pos] = T(std::forward<Args>(args)...);
        m_Size++;
        return true;
    }

    // operations -- done -- C++17
    inline void fill(const T& value)
    {
        std::fill_n(m_Elems, N, value);
    }

    inline void swap(stack_vector& other) noexcept(std::is_nothrow_swappable_v<T>)
    {
        std::swap(m_Elems, other.m_Elems);
        const size_type tmp = m_Size;
        m_Size = other.m_Size;
        other.m_Size = tmp;
    }


    // friends
    template <std::size_t I, class Ty, std::size_t Nn>
    friend constexpr Ty& get(stack_vector<Ty, Nn>& arr) noexcept;

    template <std::size_t I, class Ty, std::size_t Nn>
    friend constexpr const Ty& get(const stack_vector<Ty, Nn>& arr) noexcept;

    template <std::size_t I, class Ty, std::size_t Nn>
    friend constexpr Ty&& get(stack_vector<Ty, Nn>&& arr) noexcept;

    // We leave it in here even though it's deleted on why see comment below
    //template <std::size_t I, class Ty, std::size_t Nn>
    //friend constexpr const Ty&& get(const stack_vector<Ty, Nn>&& arr) = delete;
};


template <class T, std::size_t N>
STACK_VECTOR_NODISCARD inline bool operator==(const stack_vector<T, N>& left, const stack_vector<T, N>& right)
{
    return std::equal(left.begin(), left.end(), right.begin());
}

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD bool operator!=(const stack_vector<T, N>& left, const stack_vector<T, N>& right)
{
    return !(left == right);
}

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD inline bool operator<(const stack_vector<T, N>& left, const stack_vector<T, N>& right)
{
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD inline bool operator>(const stack_vector<T, N>& left, const stack_vector<T, N>& right)
{
    return right < left;
}

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD inline bool operator<=(const stack_vector<T, N>& left, const stack_vector<T, N>& right)
{
    return !(right < left);
}

template <class T, std::size_t N>
STACK_VECTOR_NODISCARD inline bool operator>=(const stack_vector<T, N>& left, const stack_vector<T, N>& right)
{
    return !(left < right);
}


template <class T, std::size_t N, std::enable_if_t<N == 0 || std::is_swappable_v<T>, int> = 0>
inline void swap(stack_vector<T, N>&left, stack_vector<T, N>&right) noexcept(noexcept(left.swap(right)))
{
    return left.swap(right);
}


template <std::size_t I, class Ty, std::size_t Nn>
STACK_VECTOR_NODISCARD constexpr Ty& get(stack_vector<Ty, Nn>& arr) noexcept
{
    static_assert(I < Nn, "stack_vector index out of bounds");
    return arr.m_Elems[I];
}

template <std::size_t I, class Ty, std::size_t Nn>
STACK_VECTOR_NODISCARD constexpr const Ty& get(const stack_vector<Ty, Nn>& arr) noexcept
{
    static_assert(I < Nn, "stack_vector index out of bounds");
    return arr.m_Elems[I];
}

template <std::size_t I, class Ty, std::size_t Nn>
STACK_VECTOR_NODISCARD constexpr Ty&& get(stack_vector<Ty, Nn>&& arr) noexcept
{
    static_assert(I < Nn, "stack_vector index out of bounds");
    return std::move(arr.m_Elems[I]);
}

//template <std::size_t I, class Ty, std::size_t Nn>
//STACK_VECTOR_NODISCARD constexpr const Ty&& get(const stack_vector<Ty, Nn>&& arr) noexcept
// The code in case it's ever needed, but this implementation prevents some optimizations thus we leave it out
//{
//    static_assert(I < Nn, "stack_vector index out of bounds");
//    return std::move(arr._Elems[I]);
//}


namespace std
{
    template <class T, std::size_t N>
    struct tuple_size<stack_vector<T, N>> : std::integral_constant<std::size_t, N> {};

    template<std::size_t I, class T, std::size_t N>
    struct tuple_element<I, stack_vector<T, N>> { using type = T; };

    template <class T, std::size_t N, std::enable_if_t<N == 0 || std::is_swappable_v<T>, int> = 0>
    inline void swap(stack_vector<T, N>&left, stack_vector<T, N>&right) noexcept(noexcept(left.swap(right)))
    {
        return ::swap(left, right);
    }

    template <std::size_t I, class T, std::size_t N>
    constexpr decltype(auto) get(::stack_vector<T, N>& v) noexcept(noexcept(::get<I>(v)))
    {
        return ::get<I>(v);
    }
    
    template <std::size_t I, class T, std::size_t N>
    constexpr decltype(auto) get(const ::stack_vector<T, N>& v) noexcept(noexcept(::get<I>(v)))
    {
        return ::get<I>(v);
    }
    
    template <std::size_t I, class T, std::size_t N>
    constexpr decltype(auto) get(::stack_vector<T, N>&& v) noexcept(noexcept(::get<I>(std::move(v))))
    {
        return ::get<I>(std::move(v));
    }
    
    template <std::size_t I, class T, std::size_t N>
    constexpr const T&& get(const ::stack_vector<T, N>&&) = delete;
}

#undef STACK_VECTOR_NODISCARD
#endif // STACK_VECTOR_H