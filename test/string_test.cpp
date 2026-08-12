#include "Test.h"
#include "String.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <ios>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef __has_feature
#define __has_feature(feature) 0
#endif

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define STRING_TEST_WITH_ASAN 1
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#define STRING_TEST_WITH_ASAN 0
#endif

namespace
{
    using String = ::string;

    static_assert(std::is_same_v<String::traits_type, std::char_traits<char>>);
    static_assert(std::is_same_v<String::allocator_type, std::allocator<char>>);
    static_assert(std::is_same_v<String::value_type, char>);
    static_assert(std::is_same_v<String::size_type, std::size_t>);
    static_assert(std::is_same_v<String::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<String::reference, char&>);
    static_assert(std::is_same_v<String::const_reference, const char&>);
    static_assert(std::is_same_v<String::pointer, char*>);
    static_assert(std::is_same_v<String::const_pointer, const char*>);
    static_assert(std::is_same_v<typename std::iterator_traits<String::iterator>::iterator_category,
                                 std::random_access_iterator_tag>);
    static_assert(std::is_same_v<typename std::iterator_traits<String::const_iterator>::reference,
                                 const char&>);
    static_assert(std::is_constructible_v<String, const char*>);
    static_assert(std::is_constructible_v<String, const char*, String::size_type>);
    static_assert(std::is_constructible_v<String, std::string_view>);
    static_assert(std::is_convertible_v<const String&, std::string_view>);
    static_assert(!std::is_convertible_v<std::string_view, String>);
    static_assert(noexcept(std::declval<String&>().clear()));
    static_assert(noexcept(std::declval<const String&>().data()));
    static_assert(noexcept(std::declval<const String&>().c_str()));

    template <class T, class = void>
    struct HasSwap : std::false_type
    {
    };

    template <class T>
    struct HasSwap<T, std::void_t<decltype(std::declval<T&>().swap(std::declval<T&>()))>>
        : std::true_type
    {
    };

    template <class T, class = void>
    struct HasAppendView : std::false_type
    {
    };

    template <class T>
    struct HasAppendView<T,
                         std::void_t<decltype(std::declval<T&>().append(
                             std::declval<std::string_view>()))>> : std::true_type
    {
    };

    template <class T, class = void>
    struct HasCompare : std::false_type
    {
    };

    template <class T, class = void>
    struct AcceptsOrdinaryCountAssignCall : std::false_type
    {
    };

    template <class T>
    struct AcceptsOrdinaryCountAssignCall<
        T,
        std::void_t<decltype(std::declval<T&>().assign(
            3,
            std::declval<typename T::value_type>()))>> : std::true_type
    {
    };

    template <class T>
    struct HasCompare<T,
                      std::void_t<decltype(std::declval<const T&>().compare(
                          std::declval<const T&>()))>> : std::true_type
    {
    };

    template <class Traits, class Allocator>
    std::string Bytes(const basic_string<char, Traits, Allocator>& value)
    {
        return value.data() == nullptr ? std::string{} : std::string(value.data(), value.size());
    }

    struct AllocationState
    {
        std::size_t allocations = 0;
        std::size_t deallocations = 0;
        bool fail = false;
    };

    template <bool CopyPropagation, bool MovePropagation>
    class StatefulAllocator
    {
    public:
        using value_type = char;
        using propagate_on_container_copy_assignment =
            std::integral_constant<bool, CopyPropagation>;
        using propagate_on_container_move_assignment =
            std::integral_constant<bool, MovePropagation>;
        using is_always_equal = std::false_type;

        StatefulAllocator() noexcept = default;

        StatefulAllocator(int identifier, AllocationState* allocation_state) noexcept
            : id(identifier), state(allocation_state)
        {
        }

        char* allocate(std::size_t count)
        {
            if (count > max_size() || (state != nullptr && state->fail))
                throw std::bad_alloc();
            if (state != nullptr)
                ++state->allocations;
            return std::allocator<char>{}.allocate(count);
        }

        void deallocate(char* pointer, std::size_t count) noexcept
        {
            if (state != nullptr)
                ++state->deallocations;
            std::allocator<char>{}.deallocate(pointer, count);
        }

        std::size_t max_size() const noexcept
        {
            return 64;
        }

        StatefulAllocator select_on_container_copy_construction() const noexcept
        {
            return StatefulAllocator{id + 100, state};
        }

        friend bool operator==(const StatefulAllocator& left,
                               const StatefulAllocator& right) noexcept
        {
            return left.id == right.id;
        }

        friend bool operator!=(const StatefulAllocator& left,
                               const StatefulAllocator& right) noexcept
        {
            return !(left == right);
        }

        int id = 0;
        AllocationState* state = nullptr;
    };

    using NonPropagatingAllocator = StatefulAllocator<false, false>;
    using CopyPropagatingAllocator = StatefulAllocator<true, false>;
    using MovePropagatingAllocator = StatefulAllocator<false, true>;
    using AllocString = basic_string<char, std::char_traits<char>, NonPropagatingAllocator>;
    using CopyAllocString = basic_string<char, std::char_traits<char>, CopyPropagatingAllocator>;
    using MoveAllocString = basic_string<char, std::char_traits<char>, MovePropagatingAllocator>;

    static_assert(!std::allocator_traits<NonPropagatingAllocator>::is_always_equal::value);
    static_assert(std::allocator_traits<CopyPropagatingAllocator>::
                      propagate_on_container_copy_assignment::value);
    static_assert(std::allocator_traits<MovePropagatingAllocator>::
                      propagate_on_container_move_assignment::value);

    const char* executable_path = nullptr;

    TEST(default_empty_state_and_member_types)
    {
        String value;
        CHECK(value.empty());
        CHECK_EQUAL(value.size(), 0u);
        CHECK_EQUAL(value.length(), 0u);
        CHECK_EQUAL(value.capacity(), 0u);
        CHECK_EQUAL(value.max_size(), std::numeric_limits<String::size_type>::max());

        // std::basic_string guarantees a non-null, null-terminated empty range.
        CHECK(value.data() != nullptr);
        CHECK(value.c_str() != nullptr);

        CHECK_THROWS_AS(value.at(0), std::out_of_range);
        CHECK_THROWS_AS(value.at(std::numeric_limits<String::size_type>::max()),
                        std::out_of_range);
        CHECK_THROWS_AS(value[0], std::out_of_range);
        CHECK_THROWS_AS(value.front(), std::out_of_range);
        CHECK_THROWS_AS(value.back(), std::out_of_range);
    }

    TEST(all_constructor_families)
    {
        String repeated(4, 'x');
        CHECK_EQUAL(Bytes(repeated), std::string("xxxx"));

        String counted("ab\0cd", 5);
        CHECK_EQUAL(counted.size(), 5u);
        CHECK_EQUAL(Bytes(counted), std::string("ab\0cd", 5));

        String c_string("hello");
        CHECK_EQUAL(Bytes(c_string), std::string("hello"));

        String zero_count("", 0);
        CHECK(zero_count.empty());

        const std::vector<char> source{'i', 't', 'e', 'r'};
        String iterator_range(source.begin(), source.end());
        CHECK_EQUAL(Bytes(iterator_range), std::string("iter"));

        String initialized{'l', 'i', 's', 't'};
        CHECK_EQUAL(Bytes(initialized), std::string("list"));

        String copied(c_string);
        CHECK_EQUAL(Bytes(copied), std::string("hello"));
        CHECK(copied.data() != c_string.data());

        String suffix(c_string, 2);
        CHECK_EQUAL(Bytes(suffix), std::string("llo"));
        String middle(c_string, 1, 3);
        CHECK_EQUAL(Bytes(middle), std::string("ell"));
        String clipped_count(c_string, 3, 99);
        CHECK_EQUAL(Bytes(clipped_count), std::string("lo"));
        String clipped(c_string, 3, String::npos);
        CHECK_EQUAL(Bytes(clipped), std::string("lo"));
        CHECK_THROWS_AS(String(c_string, 6), std::out_of_range);

        const std::string_view view("view-data");
        String from_view(view);
        CHECK_EQUAL(Bytes(from_view), std::string("view-data"));
        String view_part(view, 5, 99);
        CHECK_EQUAL(Bytes(view_part), std::string("data"));
        CHECK_THROWS_AS(String(view, view.size() + 1, 1), std::out_of_range);
    }

    TEST(allocator_aware_constructors_copy_and_move)
    {
        AllocationState state;
        const NonPropagatingAllocator one{1, &state};
        const NonPropagatingAllocator two{2, &state};

        AllocString allocated(one);
        CHECK_EQUAL(allocated.get_allocator().id, 1);
        allocated.assign("alpha");
        allocated.assign("a");
        CHECK_EQUAL(Bytes(allocated), std::string("a"));
        allocated.assign("alpha");

        AllocString never_allocated("", 0, one);
        CHECK(never_allocated.empty());

        AllocString copied(allocated, two);
        CHECK_EQUAL(Bytes(copied), std::string("alpha"));
        CHECK_EQUAL(copied.get_allocator().id, 2);

        AllocString equal_source("equal", one);
        const char* const equal_storage = equal_source.data();
        AllocString equal_move(std::move(equal_source), one);
        CHECK_EQUAL(Bytes(equal_move), std::string("equal"));
        CHECK(equal_move.data() == equal_storage);
        CHECK(equal_source.empty());

        AllocString different_source("different", one);
        const char* const different_storage = different_source.data();
        AllocString different_move(std::move(different_source), two);
        CHECK_EQUAL(Bytes(different_move), std::string("different"));
        CHECK(different_move.data() != different_storage);
        CHECK_EQUAL(Bytes(different_source), std::string("different"));

        AllocString direct_move_source("direct", one);
        const char* const direct_storage = direct_move_source.data();
        AllocString direct_move(std::move(direct_move_source));
        CHECK(direct_move.data() == direct_storage);
        CHECK_EQUAL(Bytes(direct_move), std::string("direct"));
        CHECK(direct_move_source.empty());
    }

    TEST(copy_construction_uses_allocator_selection)
    {
        AllocationState state;
        AllocString original("copy", NonPropagatingAllocator{7, &state});
        AllocString copied(original);
        CHECK_EQUAL(Bytes(copied), std::string("copy"));
        CHECK_EQUAL(copied.get_allocator().id, 107);
    }

    TEST(assignment_overloads_and_self_assignment)
    {
        String value("start");
        String other("copied");
        value = other;
        CHECK_EQUAL(Bytes(value), std::string("copied"));

        value = "pointer";
        CHECK_EQUAL(Bytes(value), std::string("pointer"));
        value = 'z';
        CHECK_EQUAL(Bytes(value), std::string("z"));
        value = {'a', 'b', 'c'};
        CHECK_EQUAL(Bytes(value), std::string("abc"));
        value = std::string_view("view");
        CHECK_EQUAL(Bytes(value), std::string("view"));

        String empty_but_allocated("storage");
        empty_but_allocated.clear();
        String* const self = &empty_but_allocated;
        empty_but_allocated = *self;
        CHECK(empty_but_allocated.empty());

        String move_self("self-move");
        String* const move_self_pointer = &move_self;
        move_self = std::move(*move_self_pointer);
        CHECK_EQUAL(Bytes(move_self), std::string("self-move"));

        String move_source("moved");
        const char* const storage = move_source.data();
        String move_target("old");
        move_target = std::move(move_source);
        CHECK_EQUAL(Bytes(move_target), std::string("moved"));
        CHECK(move_target.data() == storage);
        CHECK(move_source.empty());
    }

    TEST(assign_overloads_ranges_and_boundaries)
    {
        String value;
        using CountAssign = String& (String::*)(String::size_type, String::value_type);
        (value.*static_cast<CountAssign>(&String::assign))(3, 'q');
        CHECK_EQUAL(Bytes(value), std::string("qqq"));

        const String source("012345");
        value.assign(source);
        CHECK_EQUAL(Bytes(value), std::string("012345"));
        value.assign(source, 2, 3);
        CHECK_EQUAL(Bytes(value), std::string("234"));
        value.assign(source, source.size(), String::npos);
        CHECK(value.empty());
        CHECK_THROWS_AS(value.assign(source, source.size() + 1), std::out_of_range);

        value.assign("ab\0cd", 5);
        CHECK_EQUAL(Bytes(value), std::string("ab\0cd", 5));
        value.assign("cstring");
        CHECK_EQUAL(Bytes(value), std::string("cstring"));

        const std::vector<char> range{'r', 'a', 'n', 'g', 'e'};
        value.assign(range.begin(), range.end());
        CHECK_EQUAL(Bytes(value), std::string("range"));
        value.assign({'i', 'n', 'i', 't'});
        CHECK_EQUAL(Bytes(value), std::string("init"));

        const std::string_view view("view-range");
        value.assign(view);
        CHECK_EQUAL(Bytes(value), std::string("view-range"));
        value.assign(view, 5, 3);
        CHECK_EQUAL(Bytes(value), std::string("ran"));
        value.assign(view, 5, 99);
        CHECK_EQUAL(Bytes(value), std::string("range"));
        value.assign(view, view.size(), String::npos);
        CHECK(value.empty());
        CHECK_THROWS_AS(value.assign(view, view.size() + 1), std::out_of_range);

        String moved("assigned move");
        value.assign(std::move(moved));
        CHECK_EQUAL(Bytes(value), std::string("assigned move"));
        CHECK(moved.empty());
    }

    TEST(single_pass_input_iterators_are_consumed_once)
    {
        std::istringstream input("a b c");
        String value((std::istream_iterator<char>(input)), std::istream_iterator<char>());
        CHECK_EQUAL(Bytes(value), std::string("abc"));

        std::istringstream assigned_input("d e f");
        value.assign(std::istream_iterator<char>(assigned_input),
                     std::istream_iterator<char>());
        CHECK_EQUAL(Bytes(value), std::string("def"));
    }

    TEST(iterator_assignment_restores_the_null_terminator)
    {
        String value("abcdef");
        const std::vector<char> replacement{'x', 'y'};
        value.assign(replacement.begin(), replacement.end());
        REQUIRE_EQUAL(value.size(), 2u);
        CHECK_EQUAL(Bytes(value), std::string("xy"));
        CHECK_EQUAL(value.data()[value.size()], '\0');

        std::ostringstream stream;
        stream << value;
        CHECK_EQUAL(stream.str(), std::string("xy"));
    }

    TEST(element_access_mutable_const_and_embedded_null)
    {
        String value("abc");
        CHECK_EQUAL(value.at(1), 'b');
        value.at(1) = 'B';
        value[2] = 'C';
        value.front() = 'A';
        CHECK_EQUAL(value.back(), 'C');
        value.back() = 'c';
        CHECK_EQUAL(Bytes(value), std::string("ABc"));
        CHECK(value.data() == &value[0]);
        CHECK(value.c_str() == value.data());

        const String& constant = value;
        CHECK_EQUAL(constant.at(0), 'A');
        CHECK_EQUAL(constant[1], 'B');
        CHECK_EQUAL(constant.front(), 'A');
        CHECK_EQUAL(constant.back(), 'c');
        CHECK(constant.data() == constant.c_str());
        CHECK_THROWS_AS(value.at(value.size()), std::out_of_range);
        CHECK_THROWS_AS(value.at(std::numeric_limits<String::size_type>::max()),
                        std::out_of_range);

        bool subscript_threw = false;
        char terminator = 'x';
        try
        {
            terminator = constant[constant.size()];
        }
        catch (...)
        {
            subscript_threw = true;
        }
        CHECK(!subscript_threw);
        CHECK_EQUAL(terminator, '\0');

        String binary("a\0b", 3);
        CHECK_EQUAL(binary[1], '\0');
        CHECK_EQUAL(Bytes(binary), std::string("a\0b", 3));
    }

    TEST(const_iterator_supports_every_random_access_operation)
    {
        const String value("abcd");
        String::const_iterator default_one;
        String::const_iterator default_two;
        CHECK(default_one == default_two);
        CHECK(!(default_one != default_two));

        auto iterator = value.begin();
        CHECK_EQUAL(*iterator, 'a');
        CHECK(iterator.operator->() == value.data());
        CHECK_EQUAL(iterator[2], 'c');

        const auto old_increment = iterator++;
        CHECK_EQUAL(*old_increment, 'a');
        CHECK_EQUAL(*iterator, 'b');
        auto& incremented = ++iterator;
        CHECK(&incremented == &iterator);
        CHECK_EQUAL(*iterator, 'c');
        const auto old_decrement = iterator--;
        CHECK_EQUAL(*old_decrement, 'c');
        CHECK_EQUAL(*iterator, 'b');
        auto& decremented = --iterator;
        CHECK(&decremented == &iterator);
        CHECK_EQUAL(*iterator, 'a');

        auto& advanced = (iterator += 3);
        CHECK(&advanced == &iterator);
        CHECK_EQUAL(*iterator, 'd');
        auto& retreated = (iterator -= 2);
        CHECK(&retreated == &iterator);
        CHECK_EQUAL(*iterator, 'b');
        CHECK_EQUAL(iterator - value.begin(), 1);
        CHECK_EQUAL(*(iterator + 2), 'd');
        CHECK_EQUAL(*(2 + iterator), 'd');
        CHECK_EQUAL(*(iterator - 1), 'a');

        CHECK(iterator != value.begin());
        CHECK(!(iterator == value.begin()));
        CHECK(value.begin() < iterator);
        CHECK(iterator > value.begin());
        CHECK(value.begin() <= iterator);
        CHECK(iterator <= iterator);
        CHECK(iterator >= value.begin());
        CHECK(!(value.begin() >= iterator));
    }

    TEST(mutable_and_reverse_iterators_support_traversal_and_algorithms)
    {
        String value("dcba");
        auto iterator = value.begin();
        CHECK_EQUAL(*iterator, 'd');
        *iterator = 'D';
        CHECK_EQUAL(value.front(), 'D');
        CHECK(iterator.operator->() == value.data());

        auto old = iterator++;
        CHECK_EQUAL(*old, 'D');
        CHECK_EQUAL(*iterator, 'c');
        ++iterator;
        old = iterator--;
        CHECK_EQUAL(*old, 'b');
        --iterator;
        iterator += 3;
        CHECK_EQUAL(*iterator, 'a');
        iterator -= 2;
        CHECK_EQUAL(*iterator, 'c');
        CHECK_EQUAL(iterator[1], 'b');
        CHECK_EQUAL(iterator - value.begin(), 1);
        CHECK_EQUAL(*(1 + iterator), 'b');
        CHECK_EQUAL(*(iterator + 2), 'a');
        CHECK_EQUAL(*(iterator - 1), 'D');

        std::sort(value.begin(), value.end());
        CHECK_EQUAL(Bytes(value), std::string("Dabc"));
        CHECK_EQUAL(std::string(value.rbegin(), value.rend()), std::string("cbaD"));

        const String& constant = value;
        CHECK(constant.cbegin() == constant.begin());
        CHECK(constant.cend() == constant.end());
        CHECK(constant.crbegin() == constant.rbegin());
        CHECK(constant.crend() == constant.rend());
        CHECK_EQUAL(std::string(constant.crbegin(), constant.crend()), std::string("cbaD"));

        CHECK((std::is_same_v<typename std::iterator_traits<String::iterator>::pointer,
                              char*>));
    }

    TEST(empty_iteration_uses_valid_allocated_storage)
    {
        String value("allocated");
        value.clear();
        CHECK(value.empty());
        CHECK(value.begin() == value.end());
        CHECK(value.cbegin() == value.cend());
        CHECK(value.rbegin() == value.rend());
        CHECK(value.crbegin() == value.crend());
        CHECK_EQUAL(value.data()[0], '\0');
    }

    TEST(capacity_reserve_shrink_clear_and_transitions)
    {
        String value("abc");
        CHECK_EQUAL(value.capacity(), 3u);
        const char* storage = value.data();
        value.reserve(value.capacity());
        CHECK(value.data() == storage);

        value.reserve(12);
        CHECK_EQUAL(value.capacity(), 12u);
        CHECK_EQUAL(Bytes(value), std::string("abc"));
        CHECK(value.data() != storage);

        storage = value.data();
        value.reserve(4);
        CHECK(value.data() == storage);
        CHECK_EQUAL(value.capacity(), 12u);

        // This implementation treats reserve(size()) as a shrink request.
        value.reserve(value.size());
        CHECK_EQUAL(value.capacity(), value.size());
        CHECK_EQUAL(Bytes(value), std::string("abc"));

        value.shrink_to_fit();
        CHECK_EQUAL(value.capacity(), value.size());
        CHECK_EQUAL(Bytes(value), std::string("abc"));
        storage = value.data();
        value.shrink_to_fit();
        CHECK(value.data() == storage);

        value.reserve(9);
        value.clear();
        CHECK(value.empty());
        CHECK_EQUAL(value.capacity(), 9u);
        CHECK_EQUAL(value.data()[0], '\0');
        value.shrink_to_fit();
        CHECK_EQUAL(value.capacity(), 0u);
        CHECK(value.data() != nullptr);
        CHECK_EQUAL(value.data()[0], '\0');
    }

    TEST(insert_all_positions_growth_and_invalid_index_exception_safety)
    {
        String value("ac");
        value.insert(1, 2, 'b');
        CHECK_EQUAL(Bytes(value), std::string("abbc"));
        value.insert(0, 1, '^');
        CHECK_EQUAL(Bytes(value), std::string("^abbc"));
        value.insert(value.size(), 1, '$');
        CHECK_EQUAL(Bytes(value), std::string("^abbc$"));

        const std::string before = Bytes(value);
        const auto before_capacity = value.capacity();
        String& same = value.insert(2, 0, 'x');
        CHECK(&same == &value);
        CHECK_EQUAL(Bytes(value), before);
        CHECK_EQUAL(value.capacity(), before_capacity);

        CHECK_THROWS_AS(value.insert(value.size() + 1, 1, 'x'), std::out_of_range);
        CHECK_EQUAL(Bytes(value), before);

        String strong("abc");
        const auto strong_capacity = strong.capacity();
        CHECK_THROWS_AS(strong.insert(99, 20, 'x'), std::out_of_range);
        CHECK_EQUAL(Bytes(strong), std::string("abc"));
        CHECK_EQUAL(strong.capacity(), strong_capacity);
    }

    TEST(erase_prefix_middle_suffix_all_and_boundaries)
    {
        String value("012345");
        String& same = value.erase(2, 2);
        CHECK(&same == &value);
        CHECK_EQUAL(Bytes(value), std::string("0145"));
        value.erase(0, 1);
        CHECK_EQUAL(Bytes(value), std::string("145"));
        value.erase(2, String::npos);
        CHECK_EQUAL(Bytes(value), std::string("14"));
        value.erase(value.size(), 99);
        CHECK_EQUAL(Bytes(value), std::string("14"));
        value.erase();
        CHECK(value.empty());
        CHECK_EQUAL(value.data()[0], '\0');

        CHECK_THROWS_AS(value.erase(1), std::out_of_range);
        CHECK_THROWS_AS(value.erase(std::numeric_limits<String::size_type>::max()),
                        std::out_of_range);
    }

    TEST(append_character_pointer_string_growth_and_embedded_null)
    {
        String value;
        String& same = value.append('a');
        CHECK(&same == &value);
        value.append("bc");
        const String suffix("de");
        value.append(suffix);
        CHECK_EQUAL(Bytes(value), std::string("abcde"));
        value.append("");
        CHECK_EQUAL(Bytes(value), std::string("abcde"));

        const auto old_capacity = value.capacity();
        value.append('f');
        CHECK_EQUAL(Bytes(value), std::string("abcdef"));
        CHECK(value.capacity() >= value.size());
        CHECK(value.capacity() >= old_capacity);

        String binary("a\0b", 3);
        String output("prefix:");
        output.append(binary);
        CHECK_EQUAL(Bytes(output), std::string("prefix:a\0b", 10));
    }

    TEST(string_view_conversion_preserves_size_and_embedded_null)
    {
        const String value("a\0bc", 4);
        const std::string_view view = value;
        CHECK_EQUAL(view.size(), 4u);
        CHECK(view.data() == value.data());
        CHECK_EQUAL(std::string(view.data(), view.size()), std::string("a\0bc", 4));
    }

    TEST(stream_insertion_is_length_aware_and_preserves_stream_state)
    {
        std::ostringstream text;
        String ordinary("text");
        std::ostream& returned = (text << ordinary);
        CHECK(&returned == &text);
        CHECK_EQUAL(text.str(), std::string("text"));

        std::ostringstream binary_stream;
        const String binary("a\0b", 3);
        binary_stream << binary;
        CHECK_EQUAL(binary_stream.str(), std::string("a\0b", 3));

        std::ostringstream empty_stream;
        const String empty;
        CHECK_NOTHROW(empty_stream << empty);
        CHECK(empty_stream.str().empty());
    }

    TEST(allocator_copy_and_move_assignment_propagation)
    {
        AllocationState state;
        CopyAllocString copy_source("source", CopyPropagatingAllocator{2, &state});
        copy_source.reserve(20);
        CopyAllocString copy_target("target", CopyPropagatingAllocator{1, &state});
        copy_target = copy_source;
        CHECK_EQUAL(Bytes(copy_target), std::string("source"));
        CHECK_EQUAL(copy_target.get_allocator().id, 2);

        MoveAllocString move_source("move", MovePropagatingAllocator{4, &state});
        move_source.reserve(20);
        const char* const move_storage = move_source.data();
        MoveAllocString move_target("old", MovePropagatingAllocator{3, &state});
        move_target = std::move(move_source);
        CHECK_EQUAL(Bytes(move_target), std::string("move"));
        CHECK_EQUAL(move_target.get_allocator().id, 4);
        CHECK(move_target.data() == move_storage);
        CHECK(move_source.empty());

        AllocString unequal_source("copied move", NonPropagatingAllocator{6, &state});
        const char* const unequal_storage = unequal_source.data();
        AllocString unequal_target("target", NonPropagatingAllocator{5, &state});
        unequal_target = std::move(unequal_source);
        CHECK_EQUAL(Bytes(unequal_target), std::string("copied move"));
        CHECK(unequal_target.data() != unequal_storage);
        CHECK_EQUAL(Bytes(unequal_source), std::string("copied move"));
        CHECK_EQUAL(unequal_target.get_allocator().id, 5);

        AllocString equal_source("stolen move", NonPropagatingAllocator{8, &state});
        const char* const equal_storage = equal_source.data();
        AllocString equal_target("old", NonPropagatingAllocator{8, &state});
        equal_target = std::move(equal_source);
        CHECK_EQUAL(Bytes(equal_target), std::string("stolen move"));
        CHECK(equal_target.data() == equal_storage);
        CHECK(equal_source.empty());

        CopyAllocString copy_self("self", CopyPropagatingAllocator{9, &state});
        CopyAllocString* const copy_self_pointer = &copy_self;
        copy_self = std::move(*copy_self_pointer);
        CHECK_EQUAL(Bytes(copy_self), std::string("self"));

        MoveAllocString move_self("self", MovePropagatingAllocator{10, &state});
        MoveAllocString* const move_self_pointer = &move_self;
        move_self = std::move(*move_self_pointer);
        CHECK_EQUAL(Bytes(move_self), std::string("self"));
    }

    TEST(allocator_max_size_and_length_limit_are_respected)
    {
        AllocationState state;
        AllocString value("small", NonPropagatingAllocator{1, &state});
        value.reserve(value.capacity());
        value.reserve(10);
        CHECK_THROWS_AS(value.insert(99, 1, 'x'), std::out_of_range);
        CHECK_LESS_THAN_OR_EQUAL(value.max_size(), 64u);
        CHECK_THROWS_AS(value.reserve(65), std::length_error);
        CHECK_EQUAL(Bytes(value), std::string("small"));
    }

    TEST(allocation_failures_preserve_value_capacity_and_storage)
    {
        AllocationState state;
        AllocString value("stable", NonPropagatingAllocator{1, &state});
        const std::string before = Bytes(value);
        const auto capacity = value.capacity();
        const char* const storage = value.data();

        state.fail = true;
        CHECK_THROWS_AS(value.reserve(20), std::bad_alloc);
        CHECK_EQUAL(Bytes(value), before);
        CHECK_EQUAL(value.capacity(), capacity);
        CHECK(value.data() == storage);

        CHECK_THROWS_AS(value.assign("a much larger replacement"), std::bad_alloc);
        CHECK_EQUAL(Bytes(value), before);
        CHECK_EQUAL(value.capacity(), capacity);
        CHECK(value.data() == storage);

        CHECK_THROWS_AS(value.insert(2, 20, 'x'), std::bad_alloc);
        CHECK_EQUAL(Bytes(value), before);
        CHECK_EQUAL(value.capacity(), capacity);
        CHECK(value.data() == storage);
    }

    TEST(allocator_lifetime_balances_allocations_and_deallocations)
    {
        AllocationState state;
        {
            AllocString value("abc", NonPropagatingAllocator{1, &state});
            value.reserve(20);
            value.shrink_to_fit();
            AllocString copy(value, NonPropagatingAllocator{2, &state});
            CHECK_EQUAL(Bytes(copy), std::string("abc"));
        }
        CHECK_EQUAL(state.allocations, state.deallocations);
    }

    TEST(cxx17_string_surface_exposes_required_overloads)
    {
        CHECK(HasSwap<String>::value);
        CHECK(HasAppendView<String>::value);
        CHECK(HasCompare<String>::value);
        CHECK(AcceptsOrdinaryCountAssignCall<String>::value);
    }

#if STRING_TEST_WITH_ASAN
    TEST(self_append_overlap_is_diagnosed_in_an_isolated_process)
    {
        REQUIRE(executable_path != nullptr);
        int diagnostic_pipe[2] = {-1, -1};
        REQUIRE_EQUAL(pipe(diagnostic_pipe), 0);

        const pid_t child = fork();
        REQUIRE(child >= 0);
        if (child == 0)
        {
            close(diagnostic_pipe[0]);
            dup2(diagnostic_pipe[1], STDERR_FILENO);
            close(diagnostic_pipe[1]);
            setenv("STRING_TEST_UB_PROBE", "self-append", 1);
            setenv("ASAN_OPTIONS", "halt_on_error=1:detect_leaks=0", 1);
            execl(executable_path, executable_path, static_cast<char*>(nullptr));
            _exit(127);
        }

        close(diagnostic_pipe[1]);
        std::string diagnostic;
        char buffer[1024];
        for (;;)
        {
            const ssize_t count = read(diagnostic_pipe[0], buffer, sizeof(buffer));
            if (count > 0)
                diagnostic.append(buffer, static_cast<std::size_t>(count));
            else if (count == 0)
                break;
            else if (errno != EINTR)
                break;
        }
        close(diagnostic_pipe[0]);

        int status = 0;
        pid_t waited = -1;
        do
        {
            waited = waitpid(child, &status, 0);
        } while (waited < 0 && errno == EINTR);

        REQUIRE_EQUAL(waited, child);
        CHECK(!(WIFEXITED(status) && WEXITSTATUS(status) == 0));
        CHECK_MSG(diagnostic.find("AddressSanitizer") != std::string::npos, diagnostic);
        CHECK_MSG(diagnostic.find("overlap") != std::string::npos ||
                      diagnostic.find("heap-use-after-free") != std::string::npos,
                  diagnostic);
    }
#endif
}

int main(int argc, char** argv)
{
    executable_path = argc > 0 ? argv[0] : nullptr;
    const char* const probe = std::getenv("STRING_TEST_UB_PROBE");
    if (probe != nullptr && std::strcmp(probe, "self-append") == 0)
    {
        String value("self-append-needs-reallocation");
        value.append(value);
        return value.size() == 60u ? 0 : 3;
    }
    return Test::RunAllTests(argc, argv);
}
