#include "../Test.h"
#include "../array.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    using IntArray = array<int, 4>;

    template <class T, class = void>
    struct HasStdTupleSize : std::false_type
    {
    };

    template <class T>
    struct HasStdTupleSize<T, std::void_t<decltype(std::tuple_size<T>::value)>> : std::true_type
    {
    };

    template <class T, class = void>
    struct HasGlobalSwap : std::false_type
    {
    };

    template <class T>
    struct HasGlobalSwap<T, std::void_t<decltype(::swap(std::declval<T&>(), std::declval<T&>()))>>
        : std::true_type
    {
    };

    struct NoDefault
    {
        explicit NoDefault(int input) : value(input) {}
        int value;
    };

    struct Unswappable
    {
        Unswappable() = default;
        Unswappable(const Unswappable&) = delete;
        Unswappable& operator=(const Unswappable&) = delete;
    };

    struct LifetimeProbe
    {
        LifetimeProbe()
        {
            ++constructions;
        }

        ~LifetimeProbe()
        {
            ++destructions;
        }

        static int constructions;
        static int destructions;
    };

    int LifetimeProbe::constructions = 0;
    int LifetimeProbe::destructions = 0;

    struct SwapProbe
    {
        explicit SwapProbe(int input = 0) : value(input) {}

        SwapProbe(const SwapProbe& other) : value(other.value)
        {
            ++copies;
        }

        SwapProbe(SwapProbe&& other) noexcept : value(other.value)
        {
            ++moves;
            other.value = -1;
        }

        SwapProbe& operator=(const SwapProbe& other)
        {
            ++copies;
            value = other.value;
            return *this;
        }

        SwapProbe& operator=(SwapProbe&& other) noexcept
        {
            ++moves;
            value = other.value;
            other.value = -1;
            return *this;
        }

        friend void swap(SwapProbe& left, SwapProbe& right) noexcept
        {
            ++swaps;
            const int value = left.value;
            left.value = right.value;
            right.value = value;
        }

        static void Reset()
        {
            copies = 0;
            moves = 0;
            swaps = 0;
        }

        int value;
        static int copies;
        static int moves;
        static int swaps;
    };

    int SwapProbe::copies = 0;
    int SwapProbe::moves = 0;
    int SwapProbe::swaps = 0;

    struct ComparisonProbe
    {
        explicit ComparisonProbe(int input = 0) : value(input) {}

        friend bool operator==(const ComparisonProbe& left, const ComparisonProbe& right)
        {
            ++equal_calls;
            return left.value == right.value;
        }

        friend bool operator<(const ComparisonProbe& left, const ComparisonProbe& right)
        {
            ++less_calls;
            return left.value < right.value;
        }

        static void Reset()
        {
            equal_calls = 0;
            less_calls = 0;
        }

        int value;
        static int equal_calls;
        static int less_calls;
    };

    int ComparisonProbe::equal_calls = 0;
    int ComparisonProbe::less_calls = 0;

    static_assert(std::is_same_v<IntArray::value_type, int>);
    static_assert(std::is_same_v<IntArray::size_type, std::size_t>);
    static_assert(std::is_same_v<IntArray::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<IntArray::reference, int&>);
    static_assert(std::is_same_v<IntArray::const_reference, const int&>);
    static_assert(std::is_same_v<IntArray::pointer, int*>);
    static_assert(std::is_same_v<IntArray::const_pointer, const int*>);
    static_assert(std::is_same_v<typename std::iterator_traits<IntArray::iterator>::iterator_category,
                                 std::random_access_iterator_tag>);
    static_assert(std::is_same_v<typename std::iterator_traits<IntArray::iterator>::reference, int&>);
    static_assert(std::is_same_v<typename std::iterator_traits<IntArray::const_iterator>::reference,
                                 const int&>);
    static_assert(std::is_convertible_v<IntArray::iterator, IntArray::const_iterator>);
    static_assert(std::is_aggregate_v<IntArray>);
    static_assert(std::is_standard_layout_v<IntArray>);
    static_assert(std::is_trivially_copyable_v<IntArray>);
    static_assert(std::is_default_constructible_v<array<NoDefault, 0>>);
    static_assert(!std::is_constructible_v<array<int, 2>, int, int>);
    static_assert(::tuple_size<array<int, 3>>::value == 3);
    static_assert(std::is_same_v<typename ::tuple_element<1, array<long, 3>>::type, long>);
    static_assert(HasGlobalSwap<array<int, 2>>::value);
    static_assert(HasGlobalSwap<array<NoDefault, 0>>::value);
    static_assert(!HasGlobalSwap<array<Unswappable, 1>>::value);
    static_assert(noexcept(std::declval<IntArray&>().begin()));
    static_assert(noexcept(std::declval<const IntArray&>().begin()));
    static_assert(noexcept(std::declval<IntArray&>().data()));
    static_assert(noexcept(std::declval<array<int, 0>&>().swap(std::declval<array<int, 0>&>())));

    constexpr array<int, 3> constexpr_values{{2, 4, 6}};
    static_assert(constexpr_values.size() == 3);
    static_assert(constexpr_values.max_size() == 3);
    static_assert(!constexpr_values.empty());
    static_assert(constexpr_values.front() == 2);
    static_assert(constexpr_values.back() == 6);
    static_assert(constexpr_values[1] == 4);
    static_assert(constexpr_values.at(2) == 6);
    static_assert(*constexpr_values.begin() == 2);
    static_assert(constexpr_values.end() - constexpr_values.begin() == 3);
    static_assert(get<1>(constexpr_values) == 4);

    constexpr array<int, 0> constexpr_zero{};
    static_assert(constexpr_zero.empty());
    static_assert(constexpr_zero.size() == 0);
    static_assert(constexpr_zero.max_size() == 0);
    static_assert(constexpr_zero.begin() == constexpr_zero.end());
    static_assert(constexpr_zero.data() == nullptr);

    TEST(construction_assignment_traits_and_deduction_guide)
    {
        IntArray value_initialized{};
        CHECK(!value_initialized.empty());
        CHECK_EQUAL(value_initialized.max_size(), value_initialized.size());
        CHECK(std::all_of(value_initialized.begin(), value_initialized.end(),
                          [](int value) { return value == 0; }));

        IntArray values{{1, 2, 3, 4}};
        IntArray copied(values);
        CHECK(copied == values);

        IntArray moved(std::move(copied));
        CHECK(moved == values);

        IntArray copy_assigned{};
        copy_assigned = values;
        CHECK(copy_assigned == values);

        IntArray move_assigned{};
        move_assigned = std::move(copy_assigned);
        CHECK(move_assigned == values);

        array deduced{1, 2, 3};
        static_assert(std::is_same_v<decltype(deduced), array<int, 3>>);
        CHECK((deduced == array<int, 3>{{1, 2, 3}}));

        array strings{std::string("one"), std::string("two")};
        static_assert(std::is_same_v<decltype(strings), array<std::string, 2>>);
        CHECK_EQUAL(strings.front(), "one");
        CHECK_EQUAL(strings.back(), "two");
    }

    TEST(element_access_mutable_const_and_boundaries)
    {
        IntArray values{{10, 20, 30, 40}};
        CHECK_EQUAL(values.at(0), 10);
        CHECK_EQUAL(values.at(values.size() - 1), 40);
        values.at(1) = 21;
        values[2] = 31;
        values.front() = 11;
        values.back() = 41;
        CHECK(values.data() == &values[0]);

        const IntArray& constant = values;
        CHECK_EQUAL(constant.at(0), 11);
        CHECK_EQUAL(constant[1], 21);
        CHECK_EQUAL(constant[2], 31);
        CHECK_EQUAL(constant.front(), 11);
        CHECK_EQUAL(constant.back(), 41);
        CHECK(constant.data() == &constant[0]);

        CHECK_THROWS_AS(values.at(values.size()), std::out_of_range);
        CHECK_THROWS_AS(constant.at(values.size() + 1), std::out_of_range);
        CHECK_THROWS_AS(values.at(std::numeric_limits<std::size_t>::max()), std::out_of_range);

        try
        {
            static_cast<void>(constant.at(99));
            FAIL_CHECK("const at() accepted an out-of-range index");
        }
        catch (const std::out_of_range& error)
        {
            CHECK_EQUAL(std::string(error.what()), "invalid array<T, N> subscript");
        }
    }

    TEST(const_iterator_supports_every_random_access_operation)
    {
        const IntArray values{{10, 20, 30, 40}};
        IntArray::const_iterator default_one;
        IntArray::const_iterator default_two;
        CHECK(default_one == default_two);
        CHECK(!(default_one != default_two));

        auto iterator = values.begin();
        CHECK_EQUAL(*iterator, 10);
        CHECK(iterator.operator->() == values.data());
        CHECK_EQUAL(iterator[2], 30);

        auto old = iterator++;
        CHECK_EQUAL(*old, 10);
        CHECK_EQUAL(*iterator, 20);
        auto& incremented = ++iterator;
        CHECK(&incremented == &iterator);
        CHECK_EQUAL(*iterator, 30);
        old = iterator--;
        CHECK_EQUAL(*old, 30);
        CHECK_EQUAL(*iterator, 20);
        auto& decremented = --iterator;
        CHECK(&decremented == &iterator);
        CHECK_EQUAL(*iterator, 10);

        auto& advanced = (iterator += 3);
        CHECK(&advanced == &iterator);
        CHECK_EQUAL(*iterator, 40);
        auto& retreated = (iterator -= 2);
        CHECK(&retreated == &iterator);
        CHECK_EQUAL(*iterator, 20);
        CHECK_EQUAL(iterator - values.begin(), 1);
        CHECK_EQUAL(*(iterator + 2), 40);
        CHECK_EQUAL(*(2 + iterator), 40);
        CHECK_EQUAL(*(iterator - 1), 10);

        CHECK(iterator != values.begin());
        CHECK(!(iterator == values.begin()));
        CHECK(values.begin() < iterator);
        CHECK(!(iterator < values.begin()));
        CHECK(iterator > values.begin());
        CHECK(!(values.begin() > iterator));
        CHECK(values.begin() <= iterator);
        CHECK(iterator <= iterator);
        CHECK(!(iterator <= values.begin()));
        CHECK(iterator >= values.begin());
        CHECK(iterator >= iterator);
        CHECK(!(values.begin() >= iterator));
    }

    TEST(mutable_iterator_supports_every_random_access_operation)
    {
        IntArray values{{1, 2, 3, 4}};
        IntArray::iterator default_one;
        IntArray::iterator default_two;
        CHECK(default_one == default_two);

        auto iterator = values.begin();
        *iterator = 10;
        iterator.operator->()[1] = 20;
        iterator[2] = 30;
        CHECK((values == IntArray{{10, 20, 30, 4}}));

        auto old = iterator++;
        CHECK_EQUAL(*old, 10);
        CHECK_EQUAL(*iterator, 20);
        auto& incremented = ++iterator;
        CHECK(&incremented == &iterator);
        CHECK_EQUAL(*iterator, 30);
        old = iterator--;
        CHECK_EQUAL(*old, 30);
        CHECK_EQUAL(*iterator, 20);
        auto& decremented = --iterator;
        CHECK(&decremented == &iterator);
        CHECK_EQUAL(*iterator, 10);

        auto& advanced = (iterator += 3);
        CHECK(&advanced == &iterator);
        CHECK_EQUAL(*iterator, 4);
        auto& retreated = (iterator -= 2);
        CHECK(&retreated == &iterator);
        CHECK_EQUAL(*iterator, 20);
        CHECK_EQUAL(iterator - values.begin(), 1);
        CHECK_EQUAL(*(iterator + 2), 4);
        CHECK_EQUAL(*(2 + iterator), 4);
        CHECK_EQUAL(*(iterator - 1), 10);

        IntArray::const_iterator converted = iterator;
        CHECK_EQUAL(*converted, 20);
    }

    TEST(all_traversal_entry_points_and_standard_algorithms)
    {
        IntArray values{{1, 2, 3, 4}};
        const IntArray& constant = values;

        std::vector<int> forward(values.begin(), values.end());
        CHECK((forward == std::vector<int>{1, 2, 3, 4}));
        std::vector<int> const_forward(constant.begin(), constant.end());
        CHECK((const_forward == forward));
        CHECK_EQUAL(std::distance(constant.cbegin(), constant.cend()), 4);

        std::vector<int> reverse(values.rbegin(), values.rend());
        CHECK((reverse == std::vector<int>{4, 3, 2, 1}));
        std::vector<int> const_reverse(constant.rbegin(), constant.rend());
        CHECK((const_reverse == reverse));
        CHECK_EQUAL(std::distance(constant.crbegin(), constant.crend()), 4);

        CHECK_EQUAL(std::accumulate(values.begin(), values.end(), 0), 10);
        std::sort(values.begin(), values.end(), std::greater<int>{});
        CHECK((values == IntArray{{4, 3, 2, 1}}));

        int range_sum = 0;
        for (int value : values)
            range_sum += value;
        CHECK_EQUAL(range_sum, 10);
    }

    TEST(fill_member_and_swap_entry_points)
    {
        IntArray values{{1, 2, 3, 4}};
        values.fill(7);
        CHECK((values == IntArray{{7, 7, 7, 7}}));

        IntArray left{{1, 2, 3, 4}};
        IntArray right{{5, 6, 7, 8}};
        left.swap(right);
        CHECK((left == IntArray{{5, 6, 7, 8}}));
        CHECK((right == IntArray{{1, 2, 3, 4}}));

        ::swap(left, right);
        CHECK((left == IntArray{{1, 2, 3, 4}}));
        CHECK((right == IntArray{{5, 6, 7, 8}}));

        std::swap(left, right);
        CHECK((left == IntArray{{5, 6, 7, 8}}));
        CHECK((right == IntArray{{1, 2, 3, 4}}));

        left.swap(left);
        CHECK((left == IntArray{{5, 6, 7, 8}}));
    }

    TEST(member_swap_uses_element_swap_without_copying)
    {
        SwapProbe direct_left(10);
        SwapProbe direct_right(20);
        using std::swap;
        swap(direct_left, direct_right);
        CHECK_EQUAL(direct_left.value, 20);
        CHECK_EQUAL(direct_right.value, 10);

        array<SwapProbe, 2> left{{SwapProbe(1), SwapProbe(2)}};
        array<SwapProbe, 2> right{{SwapProbe(3), SwapProbe(4)}};
        SwapProbe::Reset();

        left.swap(right);

        CHECK_EQUAL(left[0].value, 3);
        CHECK_EQUAL(left[1].value, 4);
        CHECK_EQUAL(right[0].value, 1);
        CHECK_EQUAL(right[1].value, 2);
        CHECK_EQUAL_MSG(SwapProbe::swaps, 2,
                        "array::swap must exchange each element through swap");
        CHECK_EQUAL_MSG(SwapProbe::copies, 0,
                        "array::swap must support swappable move-only element types");
    }

    TEST(comparisons_cover_equal_mismatch_and_lexicographic_branches)
    {
        const IntArray one{{1, 2, 3, 4}};
        const IntArray same{{1, 2, 3, 4}};
        const IntArray middle_larger{{1, 2, 4, 0}};
        const IntArray first_larger{{2, 0, 0, 0}};

        CHECK(one == same);
        CHECK(!(one == middle_larger));
        CHECK(!(one != same));
        CHECK(one != middle_larger);
        CHECK(one < middle_larger);
        CHECK(middle_larger < first_larger);
        CHECK(!(first_larger < one));
        CHECK(first_larger > one);
        CHECK(!(one > first_larger));
        CHECK(one <= same);
        CHECK(one <= middle_larger);
        CHECK(!(middle_larger <= one));
        CHECK(middle_larger >= one);
        CHECK(one >= same);
        CHECK(!(one >= middle_larger));

        array<ComparisonProbe, 4> early_left{{ComparisonProbe(1), ComparisonProbe(9),
                                              ComparisonProbe(9), ComparisonProbe(9)}};
        array<ComparisonProbe, 4> early_right{{ComparisonProbe(2), ComparisonProbe(0),
                                               ComparisonProbe(0), ComparisonProbe(0)}};
        ComparisonProbe::Reset();
        CHECK(!(early_left == early_right));
        CHECK_EQUAL(ComparisonProbe::equal_calls, 1);

        ComparisonProbe::Reset();
        CHECK(early_left < early_right);
        CHECK_EQUAL(ComparisonProbe::less_calls, 1);
    }

    TEST(tuple_get_overloads_and_tuple_traits)
    {
        array<std::string, 3> values{{"one", "two", "three"}};
        const array<std::string, 3>& constant = values;

        get<0>(values) = "ONE";
        CHECK_EQUAL(get<0>(values), "ONE");
        CHECK_EQUAL(get<1>(constant), "two");

        std::string moved = get<2>(array<std::string, 3>{{"a", "b", "c"}});
        CHECK_EQUAL(moved, "c");
        const array<std::string, 1> const_rvalue{{"constant"}};
        const std::string&& const_moved = get<0>(std::move(const_rvalue));
        CHECK_EQUAL(const_moved, "constant");

        CHECK_EQUAL(::tuple_size<decltype(values)>::value, 3u);
        CHECK((std::is_same_v<typename ::tuple_element<2, decltype(values)>::type, std::string>));

        CHECK_MSG(HasStdTupleSize<decltype(values)>::value,
                  "array must specialize std::tuple_size for standard tuple and structured-binding integration");
    }

    TEST(zero_size_capacity_iterators_access_and_operations)
    {
        array<int, 0> values{};
        const array<int, 0>& constant = values;

        CHECK(values.empty());
        CHECK_EQUAL(values.size(), 0u);
        CHECK_EQUAL(values.max_size(), 0u);
        CHECK(values.begin() == values.end());
        CHECK(constant.begin() == constant.end());
        CHECK(constant.cbegin() == constant.cend());
        CHECK(values.rbegin() == values.rend());
        CHECK(constant.rbegin() == constant.rend());
        CHECK(constant.crbegin() == constant.crend());
        CHECK(values.data() == nullptr);
        CHECK(constant.data() == nullptr);

        CHECK_THROWS_AS(values.at(0), std::out_of_range);
        CHECK_THROWS_AS(constant.at(0), std::out_of_range);
        CHECK_THROWS_AS(values.at(std::numeric_limits<std::size_t>::max()), std::out_of_range);
        CHECK_THROWS_AS(constant.at(std::numeric_limits<std::size_t>::max()), std::out_of_range);

        values.fill(123);
        values.swap(values);
        array<int, 0> other{};
        ::swap(values, other);
        CHECK(values == other);
        CHECK(!(values != other));
        CHECK(!(values < other));
        CHECK(!(values > other));
        CHECK(values <= other);
        CHECK(values >= other);

        array<NoDefault, 0> no_default{};
        CHECK(no_default.empty());
        no_default.fill(NoDefault(1));
    }

    TEST(zero_size_array_does_not_construct_or_destroy_an_element)
    {
        LifetimeProbe::constructions = 0;
        LifetimeProbe::destructions = 0;
        {
            array<LifetimeProbe, 0> values{};
            CHECK(values.empty());
            CHECK_EQUAL_MSG(LifetimeProbe::constructions, 0,
                            "a zero-length array must not create a hidden T element");
        }
        CHECK_EQUAL_MSG(LifetimeProbe::destructions, 0,
                        "a zero-length array must not destroy a hidden T element");
    }
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
