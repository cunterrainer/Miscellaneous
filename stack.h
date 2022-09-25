#pragma once
// C++17 compliant std::stack implementation
#include <deque>
#include <type_traits>
#include <utility>
#include <memory>

template <class T, class Container = std::deque<T>>
class stack
{
public:
    using value_type      = typename Container::value_type;
    using reference       = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using size_type       = typename Container::size_type;
    using container_type  = Container;

    static_assert(std::is_same_v<T, value_type>, "container adaptors require consistent types");
private:
    template <class Alloc>
    using if_alloc_is_same = typename std::enable_if_t<std::uses_allocator_v<Container, Alloc>, int>;

    Container c{};
public:
    stack() = default;
    explicit stack(const Container& cont) : c(cont) {}
    explicit stack(Container&& cont) noexcept(std::is_nothrow_move_constructible_v<Container>) 
        : c(std::move(cont)) {}

    template <class Alloc, if_alloc_is_same<Alloc> = 0>
    explicit stack(const Alloc& alloc) noexcept(std::is_nothrow_constructible_v<Container, const Alloc&>) 
        : c(alloc) {}

    template <class Alloc, if_alloc_is_same<Alloc> = 0>
    stack(const Container& cont, const Alloc& alloc)
        : c(cont, alloc) {}

    template< class Alloc, if_alloc_is_same<Alloc> = 0>
    stack(Container&& cont, const Alloc& alloc) noexcept(std::is_nothrow_constructible_v<Container, Container, const Alloc&>)
        : c(std::move(cont), alloc) {}

    template< class Alloc, if_alloc_is_same<Alloc> = 0>
    stack(const stack& other, const Alloc& alloc) 
        : c(other.c, alloc) {}

    template< class Alloc, if_alloc_is_same<Alloc> = 0 >
    stack(stack&& other, const Alloc& alloc) noexcept(std::is_nothrow_constructible_v<Container, Container, const Alloc&>)
        : c(std::move(other.c), alloc) {}

    reference top() noexcept(noexcept(c.back())) {
        return c.back();
    }

    const_reference top() const noexcept(noexcept(c.back())) {
        return c.back();
    }

    bool empty() const noexcept(noexcept(c.empty())) {
        return c.empty();
    }

    size_type size() const noexcept(noexcept(c.size())) {
        return c.size();
    }

    void push(const value_type& value) {
        c.push_back(value);
    }

    void push(value_type&& value) {
        c.push_back(std::move(value));
    }
    
    template< class... Args >
    decltype(auto) emplace(Args&&... args)
    {
        return c.emplace_back(std::forward<Args>(args)...);
    }

    void pop() {
        c.pop_back();
    }

    void swap(stack& other) noexcept(std::is_nothrow_swappable_v<Container>)
    {
        std::swap(c, other.c);
    }

    const Container& _get_container() const noexcept {
        return c;
    }
};


template < class T, class Container >
bool operator==(const stack<T, Container>& lhs, const stack<T, Container>& rhs) {
    return lhs._get_container() == rhs._get_container();
}

template < class T, class Container >
bool operator!=(const stack<T, Container>& lhs, const stack<T, Container>& rhs) {
    return lhs._get_container() != rhs._get_container();
}

template < class T, class Container >
bool operator<(const stack<T, Container>& lhs, const stack<T, Container>& rhs) {
    return lhs._get_container() < rhs._get_container();
}

template < class T, class Container >
bool operator>(const stack<T, Container>& lhs, const stack<T, Container>& rhs) {
    return lhs._get_container() > rhs._get_container();
}

template < class T, class Container >
bool operator<=(const stack<T, Container>& lhs, const stack<T, Container>& rhs) {
    return lhs._get_container() <= rhs._get_container();
}

template < class T, class Container >
bool operator>=(const stack<T, Container>& lhs, const stack<T, Container>& rhs) {
    return lhs._get_container() >= rhs._get_container();
}

template <class T, class Container, std::enable_if_t<std::is_swappable_v<Container>, int> = 0>
void swap(const stack<T, Container>& lhs, const stack<T, Container>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}


template <class T, class = void>
struct is_allocator : std::false_type {};

template <class T>
struct is_allocator<T, std::void_t<typename T::value_type, decltype(std::declval<T&>().deallocate(std::declval<T&>().allocate(std::size_t{1}), std::size_t{1}))>> : std::true_type {};

template <class T>
inline constexpr bool is_allocator_v = is_allocator<T>::value;



template <class T, class Alloc, class = void>
struct has_allocator_type : std::false_type{}; // tests for suitable _Ty::allocator_type

template <class T, class Alloc>
struct has_allocator_type<T, Alloc, std::void_t<typename T::allocator_type>> : std::is_convertible<Alloc, typename T::allocator_type>::type {};


template<class Container, class Alloc>
struct uses_allocator : has_allocator_type<Container, Alloc>::value {};

template<class T, class Container, class Alloc>
struct uses_allocator<stack<T, Container>, Alloc> : uses_allocator<Container, Alloc>::type {};



template <class Container, std::enable_if_t<!is_allocator_v<Container>, int> = 0>
stack(Container) -> stack<typename Container::value_type, Container>;

template<class Container, class Alloc, std::enable_if_t<std::conjunction_v<std::negation<is_allocator<Container>>, uses_allocator<Container, Alloc>>, int> = 0>
stack(Container, Alloc) -> stack<typename Container::value_type, Container>;