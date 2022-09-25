#pragma once
// C++17 compliant std::array implementation
#include <cstddef>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <type_traits>

#define ARR_NODISCARD [[nodiscard]]

template <class T, class = void>
struct is_implicitly_default_constructible : std::false_type {};

template <class T>
void implicitly_default_construct(const T&);

template <class T>
struct is_implicitly_default_constructible < T, std::void_t<decltype(implicitly_default_construct<T>({})) >> : std::true_type {};


template <class T, std::size_t N>
class array_const_iterator
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
    constexpr array_const_iterator() noexcept : m_Ptr() {}
    constexpr explicit array_const_iterator(pointer parg, std::size_t off = 0) noexcept : m_Ptr(parg + off) {}

    ARR_NODISCARD constexpr reference operator*() const noexcept {
        return *m_Ptr;
    }

    ARR_NODISCARD constexpr pointer operator->() const noexcept {
        return m_Ptr;
    }

    constexpr array_const_iterator& operator++() noexcept {
        ++m_Ptr;
        return *this;
    }

    constexpr array_const_iterator operator++(int) noexcept {
        array_const_iterator tmp = *this;
        ++m_Ptr;
        return tmp;
    }

    constexpr array_const_iterator& operator--() noexcept {
        --m_Ptr;
        return *this;
    }

    constexpr array_const_iterator operator--(int) noexcept {
        array_const_iterator tmp = *this;
        --m_Ptr;
        return tmp;
    }

    constexpr array_const_iterator& operator+=(const std::ptrdiff_t off) noexcept {
        m_Ptr += off;
        return *this;
    }

    constexpr array_const_iterator& operator-=(const std::ptrdiff_t off) noexcept {
        m_Ptr -= off;
        return *this;
    }

    ARR_NODISCARD constexpr std::ptrdiff_t operator-(const array_const_iterator& right) const noexcept {
        return m_Ptr - right.m_Ptr;
    }

    ARR_NODISCARD constexpr reference operator[](const std::ptrdiff_t off) const noexcept {
        return m_Ptr[off];
    }

    ARR_NODISCARD constexpr bool operator==(const array_const_iterator& right) const noexcept {
        return m_Ptr == right.m_Ptr;
    }


    ARR_NODISCARD constexpr bool operator<(const array_const_iterator& right) const noexcept {
        return m_Ptr < right.m_Ptr;
    }

    ARR_NODISCARD constexpr array_const_iterator operator+(const std::ptrdiff_t off) const noexcept {
        array_const_iterator tmp = *this;
        tmp += off;
        return tmp;
    }

    ARR_NODISCARD constexpr array_const_iterator operator-(const std::ptrdiff_t off) const noexcept {
        array_const_iterator tmp = *this;
        tmp -= off;
        return tmp;
    }

    ARR_NODISCARD constexpr bool operator!=(const array_const_iterator& right) const noexcept {
        return !(*this == right);
    }

    ARR_NODISCARD constexpr bool operator>(const array_const_iterator& right) const noexcept {
        return right < *this;
    }

    ARR_NODISCARD constexpr bool operator<=(const array_const_iterator& right) const noexcept {
        return !(right < *this);
    }

    ARR_NODISCARD constexpr bool operator>=(const array_const_iterator& right) const noexcept {
        return !(*this < right);
    }
};

template <class T, std::size_t N>
ARR_NODISCARD constexpr array_const_iterator<T, N> operator+(const std::ptrdiff_t off, array_const_iterator<T, N> next) noexcept {
    next += off;
    return next;
}


template <class T, std::size_t N>
class array_iterator : public array_const_iterator<T, N> {
public:
    using mybase = array_const_iterator<T, N>;

    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    constexpr array_iterator() noexcept {}
    constexpr explicit array_iterator(pointer parg, std::size_t off = 0) noexcept : mybase(parg, off) {}

    ARR_NODISCARD constexpr reference operator*() const noexcept {
        return const_cast<reference>(mybase::operator*());
    }

    ARR_NODISCARD constexpr pointer operator->() const noexcept {
        return const_cast<pointer>(mybase::operator->());
    }

    constexpr array_iterator& operator++() noexcept {
        mybase::operator++();
        return *this;
    }

    constexpr array_iterator operator++(int) noexcept {
        array_iterator tmp = *this;
        mybase::operator++();
        return tmp;
    }

    constexpr array_iterator& operator--() noexcept {
        mybase::operator--();
        return *this;
    }

    constexpr array_iterator operator--(int) noexcept {
        array_iterator tmp = *this;
        mybase::operator--();
        return tmp;
    }

    constexpr array_iterator& operator+=(const std::ptrdiff_t off) noexcept {
        mybase::operator+=(off);
        return *this;
    }

    ARR_NODISCARD constexpr array_iterator operator+(const std::ptrdiff_t off) const noexcept {
        array_iterator tmp = *this;
        tmp += off;
        return tmp;
    }

    constexpr array_iterator& operator-=(const std::ptrdiff_t off) noexcept {
        mybase::operator-=(off);
        return *this;
    }

    using mybase::operator-;

    ARR_NODISCARD constexpr array_iterator operator-(const std::ptrdiff_t off) const noexcept {
        array_iterator tmp = *this;
        tmp -= off;
        return tmp;
    }

    ARR_NODISCARD constexpr reference operator[](const std::ptrdiff_t off) const noexcept {
        return const_cast<reference>(mybase::operator[](off));
    }
};

template <class T, std::size_t N>
ARR_NODISCARD constexpr array_iterator<T, N> operator+(const std::ptrdiff_t off, array_iterator<T, N> next) noexcept {
    next += off;
    return next;
}


template <class T, std::size_t N>
class array
{
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    using iterator       = array_iterator<T, N>;
    using const_iterator = array_const_iterator<T, N>;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    T m_Elems[N];
public:
    // element access -- done -- C++17
    constexpr reference at(size_type pos) {
        if (pos >= N)
            throw std::out_of_range("invalid array<T, N> subscript");
        return m_Elems[pos];
    }

    constexpr const_reference at(size_type pos) const {
        if (pos >= N)
            throw std::out_of_range("invalid array<T, N> subscript");
        return m_Elems[pos];
    }

    constexpr reference operator[](size_type pos) {
        return m_Elems[pos];
    }

    constexpr const_reference operator[](size_type pos) const {
        return m_Elems[pos];
    }

    constexpr reference front() {
        return m_Elems[0];
    }

    constexpr const_reference front() const {
        return m_Elems[0];
    }

    constexpr reference back() {
        return m_Elems[N - 1];
    }

    constexpr const_reference back() const {
        return m_Elems[N - 1];
    }

    constexpr T* data() noexcept {
        return m_Elems;
    }

    constexpr const T* data() const noexcept {
        return m_Elems;
    }


    // iterators -- done -- C++17
    ARR_NODISCARD constexpr iterator begin() noexcept {
        return iterator(m_Elems, 0);
    }

    ARR_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator(m_Elems, 0);
    }

    ARR_NODISCARD constexpr iterator end() noexcept {
        return iterator(m_Elems, N);
    }

    ARR_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator(m_Elems, N);
    }

    ARR_NODISCARD constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    ARR_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    ARR_NODISCARD constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    ARR_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    ARR_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    ARR_NODISCARD constexpr const_iterator cend() const noexcept {
        return end();
    }

    ARR_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    ARR_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }


    // capacity -- done -- C++17
    constexpr bool empty() const noexcept {
        return false;
    }

    constexpr size_type size() const noexcept {
        return N;
    }

    constexpr size_type max_size() const noexcept {
        return N;
    }

    // operations -- done -- C++17
    inline void fill(const T& value) {
        std::fill_n(m_Elems, N, value);
    }

    inline void swap(array& other) noexcept(std::is_nothrow_swappable_v<T>) {
        for (size_type i = 0; i < N; ++i) {
            T tmp = m_Elems[i];
            m_Elems[i] = other[i];
            other[i] = std::move(tmp);
        }
    }
};


struct empty_array_element {};

template <class T>
class array<T, 0> {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;

    using iterator       = array_iterator<T, 0>;
    using const_iterator = array_const_iterator<T, 0>;

    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    inline void fill(const T&) {}

    inline void swap(array&) noexcept {}

    ARR_NODISCARD constexpr iterator begin() noexcept {
        return iterator{};
    }

    ARR_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator{};
    }

    ARR_NODISCARD constexpr iterator end() noexcept {
        return iterator{};
    }

    ARR_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator{};
    }

    ARR_NODISCARD constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    ARR_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    ARR_NODISCARD constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    ARR_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    ARR_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    ARR_NODISCARD constexpr const_iterator cend() const noexcept {
        return end();
    }

    ARR_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    ARR_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

    ARR_NODISCARD constexpr size_type size() const noexcept {
        return 0;
    }

    ARR_NODISCARD constexpr size_type max_size() const noexcept {
        return 0;
    }

    ARR_NODISCARD constexpr bool empty() const noexcept {
        return true;
    }

    [[noreturn]] reference at(size_type) {
        throw std::out_of_range("invalid array<T, 0> subscript");
    }

    [[noreturn]] const_reference at(size_type) const {
        throw std::out_of_range("invalid array<T, 0> subscript");
    }

    ARR_NODISCARD reference operator[](size_type) noexcept {
        return *data();
    }

    ARR_NODISCARD const_reference operator[](size_type) const noexcept {
        return *data();
    }

    ARR_NODISCARD reference front() noexcept {
        return *data();
    }

    ARR_NODISCARD const_reference front() const noexcept {
        return *data();
    }

    ARR_NODISCARD reference back() noexcept {
        return *data();
    }

    ARR_NODISCARD const_reference back() const noexcept {
        return *data();
    }

    ARR_NODISCARD constexpr T* data() noexcept {
        return nullptr;
    }

    ARR_NODISCARD constexpr const T* data() const noexcept {
        return nullptr;
    }

    std::conditional_t<std::disjunction_v<std::is_default_constructible<T>, is_implicitly_default_constructible<T>>, T, empty_array_element> m_Elems[1];
};



template <class T, std::size_t N, std::enable_if_t<N == 0 || std::is_swappable_v<T>, int> = 0>
inline void swap(array<T, N>& left, array<T, N>& right) noexcept(noexcept(left.swap(right))) {
    return left.swap(right);
}


template <class T, std::size_t N>
ARR_NODISCARD inline bool operator==(const array<T, N>& left, const array<T, N>& right) {
    return std::equal(left.begin(), left.end(), right.begin());
}

template <class T, std::size_t N>
ARR_NODISCARD bool operator!=(const array<T, N>& left, const array<T, N>& right) {
    return !(left == right);
}

template <class T, std::size_t N>
ARR_NODISCARD inline bool operator<(const array<T, N>& left, const array<T, N>& right) {
    return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

template <class T, std::size_t N>
ARR_NODISCARD inline bool operator>(const array<T, N>& left, const array<T, N>& right) {
    return right < left;
}

template <class T, std::size_t N>
ARR_NODISCARD inline bool operator<=(const array<T, N>& left, const array<T, N>& right) {
    return !(right < left);
}

template <class T, std::size_t N>
ARR_NODISCARD inline bool operator>=(const array<T, N>& left, const array<T, N>& right) {
    return !(left < right);
}


template <std::size_t I, class T, std::size_t N>
ARR_NODISCARD constexpr T& get(array<T, N>& arr) noexcept {
    static_assert(I < N, "array index out of bounds");
    return arr.m_Elems[I];
}

template <std::size_t I, class T, std::size_t N>
ARR_NODISCARD constexpr const T& get(const array<T, N>& arr) noexcept {
    static_assert(I < N, "array index out of bounds");
    return arr.m_Elems[I];
}

template <std::size_t I, class T, std::size_t N>
ARR_NODISCARD constexpr T&& get(array<T, N>&& arr) noexcept {
    static_assert(I < N, "array index out of bounds");
    return std::move(arr._Elems[I]);
}

template <std::size_t I, class T, std::size_t N>
ARR_NODISCARD constexpr const T&& get(const array<T, N>&& arr) noexcept {
    static_assert(I < N, "array index out of bounds");
    return std::move(arr._Elems[I]);
}

template <class T>
struct tuple_size;

template <class T, std::size_t N>
struct tuple_size<array<T, N>> : std::integral_constant<std::size_t, N> {};


template<std::size_t I, class T>
struct tuple_element;

template<std::size_t I, class T, std::size_t N>
struct tuple_element<I, array<T, N>> { using type = T; };


template <class T, class... U, std::enable_if_t<std::conjunction_v<std::is_same<T, U>...>, int> = 0>
array(T, U...) -> array<T, 1 + sizeof...(U)>;