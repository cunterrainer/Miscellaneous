#include "../stack.h"
#include "../Test.h"

#include <cstddef>
#include <deque>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    template <class T>
    class StatefulAllocator
    {
    public:
        using value_type = T;

        template <class U>
        friend class StatefulAllocator;

        StatefulAllocator() noexcept = default;
        explicit StatefulAllocator(int identifier) noexcept : identifier_(identifier) {}

        template <class U>
        StatefulAllocator(const StatefulAllocator<U>& other) noexcept
            : identifier_(other.identifier())
        {
        }

        T* allocate(std::size_t count)
        {
            if (fail_allocations)
                throw std::bad_alloc{};
            return std::allocator<T>{}.allocate(count);
        }

        void deallocate(T* pointer, std::size_t count) noexcept
        {
            std::allocator<T>{}.deallocate(pointer, count);
        }

        int identifier() const noexcept
        {
            return identifier_;
        }

        template <class U>
        bool operator==(const StatefulAllocator<U>& other) const noexcept
        {
            return identifier_ == other.identifier();
        }

        template <class U>
        bool operator!=(const StatefulAllocator<U>& other) const noexcept
        {
            return !(*this == other);
        }

        static bool fail_allocations;

    private:
        int identifier_ = 0;
    };

    template <class T>
    bool StatefulAllocator<T>::fail_allocations = false;

    struct NotAnAllocator
    {
    };

    struct AllocatorShaped
    {
        using value_type = int;

        int* allocate(std::size_t);
        void deallocate(int*, std::size_t);
    };

    struct MoveProbe
    {
        explicit MoveProbe(int initial = 0) noexcept : value(initial) {}

        MoveProbe(const MoveProbe& other) noexcept : value(other.value)
        {
            ++copies;
        }

        MoveProbe(MoveProbe&& other) noexcept : value(other.value)
        {
            ++moves;
            other.value = -1;
        }

        MoveProbe& operator=(const MoveProbe& other) noexcept
        {
            value = other.value;
            ++copies;
            return *this;
        }

        MoveProbe& operator=(MoveProbe&& other) noexcept
        {
            value = other.value;
            ++moves;
            other.value = -1;
            return *this;
        }

        static void reset() noexcept
        {
            copies = 0;
            moves = 0;
        }

        int value;
        static int copies;
        static int moves;
    };

    int MoveProbe::copies = 0;
    int MoveProbe::moves = 0;

    struct MoveOnly
    {
        explicit MoveOnly(int initial) noexcept : value(initial) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) noexcept = default;
        MoveOnly& operator=(MoveOnly&&) noexcept = default;

        int value;
    };

    class InstrumentedContainer
    {
    public:
        using value_type = int;
        using reference = int&;
        using const_reference = const int&;
        using size_type = std::size_t;

        reference back() noexcept
        {
            ++mutable_back_calls;
            return values_.back();
        }

        const_reference back() const noexcept
        {
            ++const_back_calls;
            return values_.back();
        }

        bool empty() const noexcept
        {
            ++empty_calls;
            return values_.empty();
        }

        size_type size() const noexcept
        {
            ++size_calls;
            return values_.size();
        }

        void push_back(const int& value)
        {
            ++copy_push_calls;
            if (throw_on_push)
                throw std::runtime_error("copy push");
            values_.push_back(value);
        }

        void push_back(int&& value)
        {
            ++move_push_calls;
            if (throw_on_push)
                throw std::runtime_error("move push");
            values_.push_back(std::move(value));
        }

        template <class... Args>
        reference emplace_back(Args&&... arguments)
        {
            ++emplace_calls;
            if (throw_on_emplace)
                throw std::runtime_error("emplace");
            return values_.emplace_back(std::forward<Args>(arguments)...);
        }

        void pop_back()
        {
            ++pop_calls;
            if (throw_on_pop)
                throw std::runtime_error("pop");
            values_.pop_back();
        }

        mutable int mutable_back_calls = 0;
        mutable int const_back_calls = 0;
        mutable int empty_calls = 0;
        mutable int size_calls = 0;
        int copy_push_calls = 0;
        int move_push_calls = 0;
        int emplace_calls = 0;
        int pop_calls = 0;
        bool throw_on_push = false;
        bool throw_on_emplace = false;
        bool throw_on_pop = false;

    private:
        std::vector<int> values_;
    };

    struct PotentiallyThrowingMoveContainer : InstrumentedContainer
    {
        PotentiallyThrowingMoveContainer() = default;
        PotentiallyThrowingMoveContainer(const PotentiallyThrowingMoveContainer&) = default;
        PotentiallyThrowingMoveContainer& operator=(const PotentiallyThrowingMoveContainer&) = default;
        PotentiallyThrowingMoveContainer(PotentiallyThrowingMoveContainer&& other) noexcept(false)
            : InstrumentedContainer(static_cast<const InstrumentedContainer&>(other))
        {
        }
        PotentiallyThrowingMoveContainer& operator=(PotentiallyThrowingMoveContainer&& other) noexcept(false)
        {
            InstrumentedContainer::operator=(static_cast<const InstrumentedContainer&>(other));
            return *this;
        }
    };

    using DefaultStack = stack<int>;
    using Allocator = StatefulAllocator<int>;
    using AllocatorContainer = std::vector<int, Allocator>;
    using AllocatorStack = stack<int, AllocatorContainer>;

    static_assert(std::is_same_v<DefaultStack::value_type, int>);
    static_assert(std::is_same_v<DefaultStack::reference, int&>);
    static_assert(std::is_same_v<DefaultStack::const_reference, const int&>);
    static_assert(std::is_same_v<DefaultStack::size_type, std::size_t>);
    static_assert(std::is_same_v<DefaultStack::container_type, std::deque<int>>);

    static_assert(std::is_default_constructible_v<DefaultStack>);
    static_assert(std::is_constructible_v<DefaultStack, const std::deque<int>&>);
    static_assert(std::is_constructible_v<DefaultStack, std::deque<int>&&>);
    static_assert(!std::is_constructible_v<DefaultStack, NotAnAllocator>);
    static_assert(std::is_constructible_v<AllocatorStack, const Allocator&>);
    static_assert(std::is_constructible_v<AllocatorStack, const AllocatorContainer&, const Allocator&>);
    static_assert(std::is_constructible_v<AllocatorStack, AllocatorContainer&&, const Allocator&>);
    static_assert(std::is_constructible_v<AllocatorStack, const AllocatorStack&, const Allocator&>);
    static_assert(std::is_constructible_v<AllocatorStack, AllocatorStack&&, const Allocator&>);
    static_assert(!std::is_constructible_v<AllocatorStack, const NotAnAllocator&>);

    static_assert(is_allocator_v<std::allocator<int>>);
    static_assert(is_allocator_v<Allocator>);
    static_assert(is_allocator_v<AllocatorShaped>);
    static_assert(!is_allocator_v<NotAnAllocator>);
    static_assert(has_allocator_type<AllocatorContainer, Allocator>::value);
    static_assert(!has_allocator_type<std::deque<int>, NotAnAllocator>::value);
    static_assert(std::uses_allocator_v<AllocatorContainer, Allocator>);
    static_assert(uses_allocator<AllocatorContainer, Allocator>::value);
    static_assert(uses_allocator<AllocatorStack, Allocator>::value);
    static_assert(!uses_allocator<DefaultStack, NotAnAllocator>::value);

    static_assert(std::is_same_v<decltype(stack(std::deque<long>{})),
                                 stack<long, std::deque<long>>>);
    static_assert(std::is_same_v<decltype(stack(AllocatorContainer{}, Allocator{})),
                                 AllocatorStack>);
    static_assert(noexcept(std::declval<DefaultStack&>().empty()));
    static_assert(noexcept(std::declval<const DefaultStack&>().size()));
    static_assert(noexcept(std::declval<DefaultStack&>().top()));
    static_assert(noexcept(std::declval<const DefaultStack&>().top()));
    static_assert(noexcept(std::declval<DefaultStack&>().swap(std::declval<DefaultStack&>())));
    static_assert(noexcept(swap(std::declval<DefaultStack&>(), std::declval<DefaultStack&>())));
    static_assert(!noexcept(std::declval<stack<int, PotentiallyThrowingMoveContainer>&>().swap(
        std::declval<stack<int, PotentiallyThrowingMoveContainer>&>())));
    static_assert(std::is_same_v<decltype(std::declval<stack<int, InstrumentedContainer>&>().emplace(1)),
                                 int&>);

    TEST(default_container_and_container_constructors)
    {
        DefaultStack empty;
        CHECK(empty.empty());
        CHECK_EQUAL(empty.size(), 0u);

        const std::deque<int> source{1, 2, 3};
        DefaultStack copied_container(source);
        CHECK_EQUAL(copied_container.size(), 3u);
        CHECK_EQUAL(copied_container.top(), 3);
        CHECK((copied_container._get_container() == source));

        std::deque<int> movable{4, 5, 6};
        DefaultStack moved_container(std::move(movable));
        CHECK_EQUAL(moved_container.size(), 3u);
        CHECK_EQUAL(moved_container.top(), 6);
        CHECK((moved_container._get_container() == std::deque<int>{4, 5, 6}));

        stack deduced(std::deque<short>{7, 8});
        CHECK_EQUAL(deduced.size(), 2u);
        CHECK_EQUAL(deduced.top(), static_cast<short>(8));
    }

    TEST(copy_move_assignment_and_self_assignment)
    {
        DefaultStack original(std::deque<int>{1, 2, 3});
        DefaultStack copied(original);
        CHECK(copied == original);

        copied.top() = 30;
        CHECK_EQUAL(original.top(), 3);
        CHECK_EQUAL(copied.top(), 30);

        DefaultStack moved(std::move(copied));
        CHECK_EQUAL(moved.size(), 3u);
        CHECK_EQUAL(moved.top(), 30);

        DefaultStack copy_assigned;
        copy_assigned = original;
        CHECK(copy_assigned == original);
        DefaultStack* const copy_alias = &copy_assigned;
        copy_assigned = *copy_alias;
        CHECK(copy_assigned == original);

        DefaultStack move_assigned;
        move_assigned = std::move(moved);
        CHECK_EQUAL(move_assigned.size(), 3u);
        CHECK_EQUAL(move_assigned.top(), 30);
        DefaultStack* const move_alias = &move_assigned;
        move_assigned = std::move(*move_alias);
        // Self move-assignment need only leave the underlying standard container valid.
        CHECK_EQUAL(move_assigned.empty(), move_assigned.size() == 0u);
        move_assigned.push(44);
        CHECK_EQUAL(move_assigned.top(), 44);
    }

    TEST(allocator_aware_constructor_matrix_and_deduction)
    {
        const Allocator first(11);
        const Allocator second(22);

        AllocatorStack allocator_only(first);
        CHECK(allocator_only.empty());
        CHECK_EQUAL(allocator_only._get_container().get_allocator().identifier(), 11);

        const AllocatorContainer source({1, 2, 3}, first);
        AllocatorStack copied_container(source, second);
        CHECK_EQUAL(copied_container.top(), 3);
        CHECK_EQUAL(copied_container._get_container().get_allocator().identifier(), 22);

        AllocatorContainer movable({4, 5, 6}, first);
        AllocatorStack moved_container(std::move(movable), second);
        CHECK_EQUAL(moved_container.top(), 6);
        CHECK_EQUAL(moved_container._get_container().get_allocator().identifier(), 22);

        const AllocatorStack copy_source(source, first);
        AllocatorStack copied_stack(copy_source, second);
        CHECK(copied_stack == copy_source);
        CHECK_EQUAL(copied_stack._get_container().get_allocator().identifier(), 22);

        AllocatorStack move_source(AllocatorContainer({7, 8, 9}, first));
        AllocatorStack moved_stack(std::move(move_source), second);
        CHECK_EQUAL(moved_stack.size(), 3u);
        CHECK_EQUAL(moved_stack.top(), 9);
        CHECK_EQUAL(moved_stack._get_container().get_allocator().identifier(), 22);

        stack deduced(AllocatorContainer({10, 11}, first), second);
        CHECK_EQUAL(deduced.top(), 11);
        CHECK_EQUAL(deduced._get_container().get_allocator().identifier(), 22);
    }

    TEST(top_push_emplace_pop_and_boundary_sizes)
    {
        stack<int, InstrumentedContainer> values;
        CHECK(values.empty());
        CHECK_EQUAL(values.size(), 0u);
        CHECK_EQUAL(values._get_container().empty_calls, 1);
        CHECK_EQUAL(values._get_container().size_calls, 1);

        int ten = 10;
        values.push(ten);
        CHECK_EQUAL(values._get_container().copy_push_calls, 1);
        values.push(20);
        CHECK_EQUAL(values._get_container().move_push_calls, 1);
        int& result = values.emplace(30);
        CHECK_EQUAL(result, 30);
        CHECK(&result == &values.top());
        CHECK_EQUAL(values._get_container().emplace_calls, 1);
        CHECK_EQUAL(values.size(), 3u);

        values.top() = 31;
        CHECK_EQUAL(values.top(), 31);
        const auto& constant = values;
        CHECK_EQUAL(constant.top(), 31);
        CHECK_EQUAL(values._get_container().mutable_back_calls, 3);
        CHECK_EQUAL(values._get_container().const_back_calls, 1);

        values.pop();
        CHECK_EQUAL(values._get_container().pop_calls, 1);
        CHECK_EQUAL(values.top(), 20);
        values.pop();
        CHECK_EQUAL(values.top(), 10);
        CHECK_EQUAL(values.size(), 1u);
        values.pop();
        CHECK(values.empty());
        CHECK_EQUAL(values.size(), 0u);
    }

    TEST(copy_and_move_push_preserve_value_categories)
    {
        stack<MoveProbe, std::vector<MoveProbe>> values;
        MoveProbe source(42);

        MoveProbe::reset();
        values.push(source);
        CHECK_EQUAL(MoveProbe::copies, 1);
        CHECK_EQUAL(MoveProbe::moves, 0);
        CHECK_EQUAL(source.value, 42);

        MoveProbe::reset();
        values.push(MoveProbe(73));
        CHECK_EQUAL(MoveProbe::copies, 0);
        CHECK_GREATER_THAN_OR_EQUAL(MoveProbe::moves, 1);
        CHECK_EQUAL(values.top().value, 73);

        stack<MoveOnly, std::vector<MoveOnly>> move_only;
        move_only.push(MoveOnly(5));
        MoveOnly& emplaced = move_only.emplace(6);
        CHECK_EQUAL(emplaced.value, 6);
        CHECK_EQUAL(move_only.top().value, 6);

        auto moved = std::move(move_only);
        CHECK_EQUAL(moved.size(), 2u);
        CHECK_EQUAL(moved.top().value, 6);
    }

    TEST(all_relational_operators_follow_container_lexicographic_order)
    {
        const DefaultStack low(std::deque<int>{1, 2});
        const DefaultStack same(std::deque<int>{1, 2});
        const DefaultStack high(std::deque<int>{1, 3});

        CHECK(low == same);
        CHECK(!(low == high));
        CHECK(low != high);
        CHECK(!(low != same));
        CHECK(low < high);
        CHECK(!(high < low));
        CHECK(high > low);
        CHECK(!(low > high));
        CHECK(low <= high);
        CHECK(low <= same);
        CHECK(!(high <= low));
        CHECK(high >= low);
        CHECK(high >= high);
        CHECK(!(low >= high));
    }

    TEST(member_and_nonmember_swap_exchange_entire_stacks)
    {
        DefaultStack left(std::deque<int>{1, 2});
        DefaultStack right(std::deque<int>{7, 8, 9});

        left.swap(right);
        CHECK((left._get_container() == std::deque<int>{7, 8, 9}));
        CHECK((right._get_container() == std::deque<int>{1, 2}));

        swap(left, right);
        CHECK((left._get_container() == std::deque<int>{1, 2}));
        CHECK((right._get_container() == std::deque<int>{7, 8, 9}));

        left.swap(left);
        CHECK((left._get_container() == std::deque<int>{1, 2}));
        swap(right, right);
        CHECK((right._get_container() == std::deque<int>{7, 8, 9}));
    }

    TEST(container_operation_exceptions_propagate_without_hidden_mutation)
    {
        // Set flags through the original container before moving it into the adaptor.
        InstrumentedContainer throwing_push;
        throwing_push.throw_on_push = true;
        stack<int, InstrumentedContainer> push_stack(std::move(throwing_push));
        CHECK_THROWS_AS(push_stack.push(1), std::runtime_error);
        CHECK(push_stack.empty());

        InstrumentedContainer throwing_emplace;
        throwing_emplace.throw_on_emplace = true;
        stack<int, InstrumentedContainer> emplace_stack(std::move(throwing_emplace));
        CHECK_THROWS_AS(emplace_stack.emplace(2), std::runtime_error);
        CHECK(emplace_stack.empty());

        InstrumentedContainer throwing_pop;
        throwing_pop.push_back(3);
        throwing_pop.throw_on_pop = true;
        stack<int, InstrumentedContainer> pop_stack(std::move(throwing_pop));
        CHECK_THROWS_AS(pop_stack.pop(), std::runtime_error);
        CHECK_EQUAL(pop_stack.size(), 1u);
        CHECK_EQUAL(pop_stack.top(), 3);
    }

    TEST(allocator_failure_from_push_is_observable_and_stack_remains_empty)
    {
        AllocatorStack values(Allocator(41));
        StatefulAllocator<int>::fail_allocations = true;
        CHECK_THROWS_AS(values.push(99), std::bad_alloc);
        StatefulAllocator<int>::fail_allocations = false;
        CHECK(values.empty());
        CHECK_EQUAL(values._get_container().get_allocator().identifier(), 41);

        CHECK_NOTHROW(values.push(100));
        CHECK_EQUAL(values.top(), 100);
    }
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
