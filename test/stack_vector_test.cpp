#include "Test.h"
#include "stack_vector.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#ifndef NDEBUG
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace
{
    using Vector = stack_vector<int, 4>;

    static_assert(std::is_same_v<Vector::value_type, int>);
    static_assert(std::is_same_v<Vector::size_type, std::size_t>);
    static_assert(std::is_same_v<Vector::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<Vector::reference, int&>);
    static_assert(std::is_same_v<Vector::const_reference, const int&>);
    static_assert(std::is_same_v<Vector::pointer, int*>);
    static_assert(std::is_same_v<Vector::const_pointer, const int*>);
    static_assert(std::is_same_v<typename std::iterator_traits<Vector::iterator>::iterator_category,
                                 std::random_access_iterator_tag>);
    static_assert(std::is_same_v<typename std::iterator_traits<Vector::const_iterator>::reference,
                                 const int&>);
    static_assert(std::is_constructible_v<stack_vector<int, 2>, int, int>);
    static_assert(!std::is_constructible_v<stack_vector<int, 2>, int, int, int>);
    static_assert(std::is_standard_layout_v<Vector>);
    static_assert(std::tuple_size_v<stack_vector<int, 3>> == 3);
    static_assert(std::is_same_v<std::tuple_element_t<1, stack_vector<int, 3>>, int>);

    constexpr stack_vector<int, 4> constexpr_values(2, 4, 6);
    static_assert(constexpr_values.size() == 3);
    static_assert(!constexpr_values.empty());
    static_assert(constexpr_values.max_size() == 4);
    static_assert(constexpr_values.capacity() == 4);
    static_assert(constexpr_values.front() == 2);
    static_assert(constexpr_values.back() == 6);
    static_assert(constexpr_values[1] == 4);
    static_assert(constexpr_values.at(2) == 6);
    static_assert(*constexpr_values.begin() == 2);
    static_assert(constexpr_values.end() - constexpr_values.begin() == 3);
    static_assert(get<1>(constexpr_values) == 4);

    struct ThrowingValue
    {
        ThrowingValue() noexcept = default;
        ThrowingValue(int)
        {
            throw std::runtime_error("construction failed");
        }

        ThrowingValue(const ThrowingValue&) = default;
        ThrowingValue(ThrowingValue&&) noexcept = default;
        ThrowingValue& operator=(const ThrowingValue&) = default;
        ThrowingValue& operator=(ThrowingValue&&) noexcept = default;
    };

    static_assert(noexcept(Vector(1, 2, 3, 4)));
    static_assert(!std::is_nothrow_constructible_v<stack_vector<ThrowingValue, 2>, int>);
    static_assert(!noexcept(std::declval<stack_vector<ThrowingValue, 2>&>().emplace_back(1)));
    static_assert(!noexcept(std::declval<stack_vector<ThrowingValue, 2>&>().emplace(0, 1)));

#ifndef NDEBUG
    template <class Function>
    bool DiesFromAssertion(Function&& function)
    {
        const pid_t child = fork();
        if (child < 0)
            return false;

        if (child == 0)
        {
            close(STDERR_FILENO);
            function();
            _exit(0);
        }

        int status = 0;
        pid_t waited = 0;
        do
        {
            waited = waitpid(child, &status, 0);
        } while (waited < 0 && errno == EINTR);

        return waited == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
    }
#endif

    TEST(construction_copy_move_and_capacity)
    {
        Vector empty;
        CHECK(empty.empty());
        CHECK_EQUAL(empty.size(), 0u);
        CHECK_EQUAL(empty.max_size(), 4u);
        CHECK_EQUAL(empty.capacity(), 4u);

        Vector values(1, 2, 3);
        CHECK(!values.empty());
        CHECK_EQUAL(values.size(), 3u);

        Vector copied(values);
        CHECK(copied == values);
        Vector moved(std::move(copied));
        CHECK(moved == values);

        Vector copy_assigned;
        copy_assigned = values;
        CHECK(copy_assigned == values);
        Vector* copy_self = &copy_assigned;
        copy_assigned = *copy_self;
        CHECK(copy_assigned == values);

        Vector move_assigned;
        move_assigned = std::move(copy_assigned);
        CHECK(move_assigned == values);
        Vector* move_self = &move_assigned;
        move_assigned = std::move(*move_self);
        CHECK(move_assigned == values);

        stack_vector<int, 0> zero;
        CHECK(zero.empty());
        CHECK_EQUAL(zero.size(), 0u);
        CHECK_EQUAL(zero.max_size(), 0u);
        CHECK(zero.begin() == zero.end());
        zero.swap(zero);
    }

    TEST(element_access_mutable_and_const)
    {
        Vector values(10, 20, 30);
        CHECK_EQUAL(values.at(1), 20);
        values.at(1) = 21;
        CHECK_EQUAL(values[1], 21);
        values[1] = 22;
        CHECK_EQUAL(values.front(), 10);
        CHECK_EQUAL(values.back(), 30);
        values.front() = 11;
        values.back() = 31;
        CHECK(values.data() == &values[0]);

        const Vector& constant = values;
        CHECK_EQUAL(constant.at(0), 11);
        CHECK_EQUAL(constant[1], 22);
        CHECK_EQUAL(constant.front(), 11);
        CHECK_EQUAL(constant.back(), 31);
        CHECK(constant.data() == &constant[0]);

        CHECK_THROWS_AS(values.at(values.size()), std::out_of_range);
        CHECK_THROWS_AS(constant.at(99), std::out_of_range);
        CHECK_THROWS_AS(values.at(std::numeric_limits<std::size_t>::max()), std::out_of_range);

#ifdef NDEBUG
        // The backing objects exist for the full fixed capacity, so accessing
        // a default-initialized but logically inactive slot is safe in release.
        values[3] = 44;
        CHECK_EQUAL(constant[3], 44);
#endif
    }

    TEST(edge_boundaries_preserve_size_and_storage)
    {
        Vector values;
        values.resize(values.max_size(), 7);
        CHECK_EQUAL(values.size(), values.max_size());
        CHECK(std::all_of(values.begin(), values.end(), [](int value) { return value == 7; }));

        values.resize(0);
        CHECK(values.empty());
        CHECK(values.begin() == values.end());
        CHECK(values.rbegin() == values.rend());

        CHECK(values.insert(values.size(), 1));
        CHECK(values.insert(values.size(), 2));
        CHECK(values.emplace(values.size(), 3));
        CHECK(values.push_back(4));
        CHECK((values == Vector(1, 2, 3, 4)));

        stack_vector<int, 0> zero;
        CHECK_THROWS_AS(zero.at(0), std::out_of_range);
        CHECK_THROWS_AS(zero.at(std::numeric_limits<std::size_t>::max()), std::out_of_range);
        CHECK(zero.data() != nullptr);
    }

    TEST(const_iterator_supports_every_random_access_operation)
    {
        const Vector values(10, 20, 30, 40);
        Vector::const_iterator default_one;
        Vector::const_iterator default_two;
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
        Vector values(1, 2, 3, 4);
        Vector::iterator default_one;
        Vector::iterator default_two;
        CHECK(default_one == default_two);

        auto iterator = values.begin();
        *iterator = 10;
        iterator.operator->()[1] = 20;
        iterator[2] = 30;
        CHECK_EQUAL(values[0], 10);
        CHECK_EQUAL(values[1], 20);
        CHECK_EQUAL(values[2], 30);

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
    }

    TEST(all_forward_const_and_reverse_traversal_entry_points)
    {
        Vector values(1, 2, 3);
        const Vector& constant = values;

        std::array<int, 3> forward{};
        std::copy(values.begin(), values.end(), forward.begin());
        CHECK((forward == std::array<int, 3>{1, 2, 3}));

        std::array<int, 3> const_forward{};
        std::copy(constant.begin(), constant.end(), const_forward.begin());
        CHECK((const_forward == std::array<int, 3>{1, 2, 3}));
        CHECK_EQUAL(std::distance(constant.cbegin(), constant.cend()), 3);

        std::array<int, 3> reverse{};
        std::copy(values.rbegin(), values.rend(), reverse.begin());
        CHECK((reverse == std::array<int, 3>{3, 2, 1}));

        std::array<int, 3> const_reverse{};
        std::copy(constant.rbegin(), constant.rend(), const_reverse.begin());
        CHECK((const_reverse == std::array<int, 3>{3, 2, 1}));
        CHECK_EQUAL(std::distance(constant.crbegin(), constant.crend()), 3);

        int sum = 0;
        for (int value : values)
            sum += value;
        CHECK_EQUAL(sum, 6);
    }

    TEST(modifiers_cover_growth_shrink_shift_and_no_shift)
    {
        Vector values;
        int one = 1;
        CHECK(values.push_back(one));
        CHECK(values.push_back(2));
        CHECK(values.emplace_back(3));
        CHECK((values == Vector(1, 2, 3)));

        values.pop_back();
        CHECK((values == Vector(1, 2)));
        values.resize(4, 9);
        CHECK((values == Vector(1, 2, 9, 9)));
        values.resize(2, 7);
        CHECK((values == Vector(1, 2)));
        values.resize(2, 8);
        CHECK((values == Vector(1, 2)));
        values.resize(4);
        CHECK((values == Vector(1, 2, 0, 0)));
        values.resize(1);
        CHECK((values == Vector(1)));

        values.clear();
        CHECK(values.empty());

        int twenty = 20;
        CHECK(values.insert(0, twenty));
        CHECK(values.insert(0, 10));
        int five = 5;
        CHECK(values.insert(0, five));
        values.pop_back();
        CHECK(values.emplace(2, 30));
        CHECK(values.emplace(1, 15));
        CHECK((values == Vector(5, 15, 10, 30)));
    }

#ifndef NDEBUG
    TEST(emplacement_propagates_construction_exceptions_without_changing_size)
    {
        CHECK_THROWS_AS((stack_vector<ThrowingValue, 2>(1)), std::runtime_error);
        stack_vector<ThrowingValue, 2> values;
        CHECK_THROWS_AS(values.emplace_back(1), std::runtime_error);
        CHECK_EQUAL(values.size(), 0u);
        CHECK_THROWS_AS(values.emplace(0, 2), std::runtime_error);
        CHECK_EQUAL(values.size(), 0u);
    }
#endif

    TEST(fill_member_and_all_swap_entry_points)
    {
        Vector partially_used(1, 2);
        partially_used.fill(7);
        CHECK_EQUAL(partially_used.size(), 2u);
        CHECK_EQUAL(partially_used[0], 7);
        CHECK_EQUAL(partially_used[1], 7);
        CHECK_EQUAL(get<2>(partially_used), 7);
        CHECK_EQUAL(get<3>(partially_used), 7);

        Vector left(1, 2);
        Vector right(8, 9, 10);
        left.swap(right);
        CHECK((left == Vector(8, 9, 10)));
        CHECK((right == Vector(1, 2)));

        ::swap(left, right);
        CHECK((left == Vector(1, 2)));
        CHECK((right == Vector(8, 9, 10)));

        std::swap(left, right);
        CHECK((left == Vector(8, 9, 10)));
        CHECK((right == Vector(1, 2)));
    }

    TEST(comparisons_cover_size_short_circuit_and_lexicographic_outcomes)
    {
        const Vector one(1);
        const Vector one_again(1);
        const Vector one_two(1, 2);
        const Vector one_three(1, 3);
        const Vector two(2);

        CHECK(one == one_again);
        CHECK(!(one == one_two));
        CHECK(!(one_two == one_three));
        CHECK(!(one != one_again));
        CHECK(one != two);

        CHECK(one < one_two);
        CHECK(one_two < one_three);
        CHECK(!(two < one));
        CHECK(two > one);
        CHECK(!(one > two));
        CHECK(one <= one_again);
        CHECK(one <= two);
        CHECK(!(two <= one));
        CHECK(two >= one);
        CHECK(one >= one_again);
        CHECK(!(one >= two));
    }

    TEST(tuple_get_and_structured_binding_integration)
    {
        stack_vector<int, 3> values(4, 5, 6);
        const stack_vector<int, 3>& constant = values;

        get<0>(values) = 40;
        CHECK_EQUAL(get<0>(values), 40);
        CHECK_EQUAL(get<1>(constant), 5);
        int moved = get<2>(stack_vector<int, 3>(7, 8, 9));
        CHECK_EQUAL(moved, 9);

        std::get<0>(values) = 41;
        CHECK_EQUAL(std::get<0>(values), 41);
        CHECK_EQUAL(std::get<1>(constant), 5);
        moved = std::get<2>(stack_vector<int, 3>(10, 11, 12));
        CHECK_EQUAL(moved, 12);

        auto [first, second, third] = values;
        CHECK_EQUAL(first, 41);
        CHECK_EQUAL(second, 5);
        CHECK_EQUAL(third, 6);

        auto& [first_ref, second_ref, third_ref] = values;
        first_ref = 42;
        second_ref = 50;
        third_ref = 60;
        CHECK_EQUAL(values[0], 42);
        CHECK_EQUAL(values[1], 50);
        CHECK_EQUAL(values[2], 60);

        const auto& [const_first, const_second, const_third] = constant;
        CHECK_EQUAL(const_first, 42);
        CHECK_EQUAL(const_second, 50);
        CHECK_EQUAL(const_third, 60);
    }

#ifdef NDEBUG
    TEST(release_only_capacity_and_position_failures_return_false_safely)
    {
        Vector full(1, 2, 3, 4);
        int two = 2;
        CHECK(!full.push_back(two));
        CHECK(!full.push_back(5));
        CHECK(!full.emplace_back(6));
        CHECK(!full.insert(0, two));
        CHECK(!full.insert(0, 7));
        CHECK(!full.emplace(0, 8));
        CHECK((full == Vector(1, 2, 3, 4)));

        Vector values(1);
        CHECK(!values.insert(2, two));
        CHECK(!values.insert(2, 2));
        CHECK(!values.emplace(2, 2));
        CHECK((values == Vector(1)));

        const std::size_t extreme = std::numeric_limits<std::size_t>::max();
        CHECK(!values.insert(extreme, two));
        CHECK(!values.insert(extreme, 2));
        CHECK(!values.emplace(extreme, 2));

        stack_vector<int, 0> zero;
        CHECK(!zero.push_back(1));
        CHECK(!zero.emplace_back(1));
        CHECK(!zero.insert(0, 1));
        CHECK(!zero.emplace(0, 1));
        CHECK(zero.empty());
    }
#else
    TEST(out_of_bounds_and_undefined_preconditions_abort_in_debug_builds)
    {
        CHECK(DiesFromAssertion([] {
            Vector values(1);
            static_cast<void>(values[1]);
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            static_cast<void>(values.front());
        }));
        CHECK(DiesFromAssertion([] {
            const Vector values;
            static_cast<void>(values.front());
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            static_cast<void>(values.back());
        }));
        CHECK(DiesFromAssertion([] {
            const Vector values;
            static_cast<void>(values.back());
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 0> values;
            static_cast<void>(values[0]);
        }));
        CHECK(DiesFromAssertion([] {
            const Vector values(1);
            static_cast<void>(values[4]);
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 1> values(1);
            int value = 2;
            static_cast<void>(values.push_back(value));
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 1> values(1);
            static_cast<void>(values.push_back(2));
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 1> values(1);
            static_cast<void>(values.emplace_back(2));
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 1> values(1);
            int value = 2;
            static_cast<void>(values.insert(0, value));
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 1> values(1);
            static_cast<void>(values.insert(0, 2));
        }));
        CHECK(DiesFromAssertion([] {
            stack_vector<int, 1> values(1);
            static_cast<void>(values.emplace(0, 2));
        }));
        CHECK(DiesFromAssertion([] {
            Vector values(1);
            int value = 2;
            static_cast<void>(values.insert(2, value));
        }));
        CHECK(DiesFromAssertion([] {
            Vector values(1);
            static_cast<void>(values.insert(2, 2));
        }));
        CHECK(DiesFromAssertion([] {
            Vector values(1);
            static_cast<void>(values.emplace(2, 2));
        }));
        CHECK(DiesFromAssertion([] {
            Vector values(1);
            static_cast<void>(values.emplace(std::numeric_limits<std::size_t>::max(), 2));
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            values.pop_back();
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            values.resize(5, 1);
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            values.resize(5);
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            values.resize(std::numeric_limits<std::size_t>::max(), 1);
        }));
        CHECK(DiesFromAssertion([] {
            Vector values;
            values.resize(std::numeric_limits<std::size_t>::max());
        }));
    }
#endif
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
