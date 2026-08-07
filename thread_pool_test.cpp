#include "ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <mutex>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <cstdlib>

namespace
{
    std::atomic<long long> g_fail_allocation_after = -1;
}

void* operator new(std::size_t size)
{
    long long remaining = g_fail_allocation_after.load(std::memory_order_acquire);

    while (remaining >= 0)
    {
        if (g_fail_allocation_after.compare_exchange_weak(
                remaining,
                remaining - 1,
                std::memory_order_acq_rel,
                std::memory_order_acquire))
        {
            if (remaining == 0)
            {
                throw std::bad_alloc();
            }
            break;
        }
    }

    if (void* memory = std::malloc(size))
    {
        return memory;
    }

    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete[](void* memory) noexcept
{
    ::operator delete(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    ::operator delete(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    ::operator delete[](memory);
}

namespace
{
    using namespace std::chrono_literals;
    std::atomic<int> g_detached_counter = 0;

    int AddThree(int a, int b, int c)
    {
        return a + b + c;
    }

    int MultiplyTwo(int a, int b)
    {
        return a * b;
    }

    int ReturnOne()
    {
        return 1;
    }

    void IncrementGlobalDetachedCounter()
    {
        g_detached_counter.fetch_add(1, std::memory_order_relaxed);
    }

    void NoopTask()
    {
    }

    struct ToggleThrowMove
    {
        static std::atomic<bool> throw_on_move;
        int payload = 0;

        explicit ToggleThrowMove(int value)
            : payload(value)
        {
        }

        ToggleThrowMove(const ToggleThrowMove&) = delete;
        ToggleThrowMove& operator=(const ToggleThrowMove&) = delete;

        ToggleThrowMove(ToggleThrowMove&& other)
            : payload(other.payload)
        {
            if (throw_on_move.load(std::memory_order_acquire))
            {
                throw std::runtime_error("ToggleThrowMove move throw");
            }
        }

        ToggleThrowMove& operator=(ToggleThrowMove&&) = delete;
    };

    std::atomic<bool> ToggleThrowMove::throw_on_move = false;

    int ConsumeToggleThrowMove(ToggleThrowMove value)
    {
        return value.payload;
    }

    void SetFailAllocationAfter(long long allocations_before_throw)
    {
        g_fail_allocation_after.store(allocations_before_throw, std::memory_order_release);
    }


    struct TestFailure : std::runtime_error
    {
        explicit TestFailure(const std::string& message)
            : std::runtime_error(message)
        {
        }
    };

    struct TestCase
    {
        std::string name;
        void (*function)();
    };

    std::vector<TestCase>& Registry()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    struct TestRegistrar
    {
        TestRegistrar(const std::string& name, void (*function)())
        {
            Registry().push_back({name, function});
        }
    };

    #define TEST(name) \
        void name(); \
        TestRegistrar name##_registrar(#name, &name); \
        void name()

    #define REQUIRE(condition) \
        do \
        { \
            if (!(condition)) \
            { \
                throw TestFailure(std::string("REQUIRE failed: ") + #condition + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
            } \
        } while (false)

    #define REQUIRE_EQ(left, right) \
        do \
        { \
            const auto left_value = (left); \
            const auto right_value = (right); \
            if (!(left_value == right_value)) \
            { \
                throw TestFailure(std::string("REQUIRE_EQ failed: ") + #left + " != " + #right + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
            } \
        } while (false)

    TEST(construct_with_zero_normalizes_to_one)
    {
        Utility::ThreadPool pool(0u);
        REQUIRE_EQ(pool.Size(), 1u);
        REQUIRE(pool.IsAcceptingSubmissions());
    }

    TEST(submit_returns_values_and_handles_arguments)
    {
        Utility::ThreadPool pool(4u);
        auto future = pool.Submit(AddThree, 3, 4, 5);
        auto future_one = pool.Submit(ReturnOne);

        REQUIRE_EQ(future.get(), 12);
        REQUIRE_EQ(future_one.get(), 1);
        REQUIRE(pool.WaitIdle(500ms));
    }

    TEST(submit_propagates_exception)
    {
        Utility::ThreadPool pool(2u);
        auto future = pool.Submit(
            []() -> int
            {
                throw std::runtime_error("boom");
            });

        bool caught = false;
        try
        {
            (void)future.get();
        }
        catch (const std::runtime_error&)
        {
            caught = true;
        }

        REQUIRE(caught);
    }

    TEST(submit_packaging_covers_success_and_throw_branch)
    {
        Utility::ThreadPool pool(2u);
        ToggleThrowMove::throw_on_move.store(false, std::memory_order_release);
        auto success = pool.Submit(ConsumeToggleThrowMove, ToggleThrowMove{77});
        REQUIRE_EQ(success.get(), 77);

        ToggleThrowMove::throw_on_move.store(true, std::memory_order_release);
        bool threw = false;
        try
        {
            (void)pool.Submit(ConsumeToggleThrowMove, ToggleThrowMove{13});
        }
        catch (const std::runtime_error&)
        {
            threw = true;
        }

        ToggleThrowMove::throw_on_move.store(false, std::memory_order_release);
        REQUIRE(threw);
    }

    TEST(submit_covers_bad_alloc_on_packaged_task_allocation)
    {
        Utility::ThreadPool pool(1u);
        bool threw_bad_alloc = false;

        try
        {
            SetFailAllocationAfter(1);
            (void)pool.Submit(AddThree, 1, 2, 3);
        }
        catch (const std::bad_alloc&)
        {
            threw_bad_alloc = true;
        }

        SetFailAllocationAfter(-1);
        REQUIRE(threw_bad_alloc);
    }

    TEST(submit_detached_runs_tasks)
    {
        Utility::ThreadPool pool(4u);
        std::atomic<int> value = 0;

        for (int i = 0; i < 1000; ++i)
        {
            pool.SubmitDetached(
                [&value]
                {
                    value.fetch_add(1, std::memory_order_relaxed);
                });
        }

        pool.SubmitDetached(NoopTask);

        pool.WaitIdle();
        REQUIRE_EQ(value.load(std::memory_order_relaxed), 1000);
    }

    TEST(idle_worker_never_misses_new_work_notification)
    {
        Utility::ThreadPool pool(1u);

        // Repeatedly put the sole worker to sleep before submitting the next
        // task.  This exercises the idle-predicate/notification boundary that
        // previously allowed a lost wake-up and an infinite WaitIdle().
        for (int iteration = 0; iteration < 1000; ++iteration)
        {
            REQUIRE(pool.WaitIdle(100ms));
            auto completed = pool.Submit([] { return true; });
            REQUIRE(completed.wait_for(1s) == std::future_status::ready);
            REQUIRE(completed.get());
        }

        REQUIRE(pool.WaitIdle(1s));
        REQUIRE_EQ(pool.PendingTasks(), 0u);
        REQUIRE_EQ(pool.ActiveWorkers(), 0u);
    }

    TEST(nested_submit_from_worker_path)
    {
        Utility::ThreadPool pool(4u);
        std::atomic<int> sum = 0;

        auto root = pool.Submit(
            [&pool, &sum]
            {
                std::vector<std::future<void>> children;
                children.reserve(256);

                for (int i = 0; i < 256; ++i)
                {
                    children.emplace_back(pool.Submit(
                        [&sum]
                        {
                            sum.fetch_add(1, std::memory_order_relaxed);
                        }));
                }

                for (std::future<void>& child : children)
                {
                    child.get();
                }
            });

        root.get();
        pool.WaitIdle();
        REQUIRE_EQ(sum.load(std::memory_order_relaxed), 256);
    }

    TEST(global_thread_pool_singleton_identity)
    {
        Utility::ThreadPool& a = Utility::GlobalThreadPool();
        Utility::ThreadPool& b = Utility::GlobalThreadPool();
        REQUIRE(&a == &b);
        Utility::InitializeGlobalThreadPool(a.Size(), a.GetSubmissionPolicy());
    }

    TEST(global_thread_pool_reinitialize_same_config_noop)
    {
        Utility::ThreadPool& pool = Utility::GlobalThreadPool();
        Utility::InitializeGlobalThreadPool(pool.Size(), pool.GetSubmissionPolicy());
    }

    TEST(global_thread_pool_reinitialize_different_policy_throws)
    {
        Utility::ThreadPool& pool = Utility::GlobalThreadPool();
        const auto current_policy = pool.GetSubmissionPolicy();
        const auto other_policy =
            current_policy == Utility::ThreadPool::SubmissionPolicy::RoundRobin
                ? Utility::ThreadPool::SubmissionPolicy::Random
                : Utility::ThreadPool::SubmissionPolicy::RoundRobin;

        bool threw = false;
        try
        {
            Utility::InitializeGlobalThreadPool(pool.Size(), other_policy);
        }
        catch (const std::runtime_error&)
        {
            threw = true;
        }

        REQUIRE(threw);
    }

    TEST(submission_policy_round_robin_and_random)
    {
        Utility::ThreadPool pool(4u);
        REQUIRE(pool.GetSubmissionPolicy() == Utility::ThreadPool::SubmissionPolicy::RoundRobin);

        pool.SetSubmissionPolicy(Utility::ThreadPool::SubmissionPolicy::Random);
        REQUIRE(pool.GetSubmissionPolicy() == Utility::ThreadPool::SubmissionPolicy::Random);

        std::atomic<int> counter = 0;
        for (int i = 0; i < 200; ++i)
        {
            pool.SubmitDetached(
                [&counter]
                {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
        }

        pool.WaitIdle();
        REQUIRE_EQ(counter.load(std::memory_order_relaxed), 200);
    }

    TEST(constructor_accepts_submission_policy)
    {
        Utility::ThreadPool pool(4u, Utility::ThreadPool::SubmissionPolicy::Random);
        REQUIRE(pool.GetSubmissionPolicy() == Utility::ThreadPool::SubmissionPolicy::Random);
        REQUIRE_EQ(pool.Size(), 4u);
    }

    TEST(local_queue_submit_and_detached_submit)
    {
        Utility::ThreadPool pool(2u, Utility::ThreadPool::SubmissionPolicy::RoundRobin);
        g_detached_counter.store(0, std::memory_order_relaxed);

        auto value_future = pool.SubmitToWorker(0u, MultiplyTwo, 6, 7);
        REQUIRE_EQ(value_future.get(), 42);

        for (int i = 0; i < 128; ++i)
        {
            pool.SubmitDetachedToWorker(1u, IncrementGlobalDetachedCounter);
        }

        pool.WaitIdle();
        REQUIRE_EQ(g_detached_counter.load(std::memory_order_relaxed), 128);
    }

    TEST(local_queue_submit_invalid_worker_index_throws)
    {
        Utility::ThreadPool pool(2u);
        bool submit_threw = false;
        bool detached_threw = false;

        try
        {
            (void)pool.SubmitToWorker(99u, MultiplyTwo, 1, 2);
        }
        catch (const std::out_of_range&)
        {
            submit_threw = true;
        }

        try
        {
            pool.SubmitDetachedToWorker(99u, IncrementGlobalDetachedCounter);
        }
        catch (const std::out_of_range&)
        {
            detached_threw = true;
        }

        REQUIRE(submit_threw);
        REQUIRE(detached_threw);
    }

    TEST(set_worker_topology_hint_valid_and_invalid_index)
    {
        Utility::ThreadPool pool(2u);
        Utility::ThreadPool::WorkerTopologyHint hint;
        hint.preferred_cpu = 0u;
        hint.preferred_numa_node = 0u;

        REQUIRE(pool.SetWorkerTopologyHint(0u, hint));
        REQUIRE(!pool.SetWorkerTopologyHint(99u, hint));

        Utility::ThreadPool::WorkerTopologyHint invalid_hint;
        invalid_hint.preferred_cpu = 1'000'000u;
        REQUIRE(pool.SetWorkerTopologyHint(1u, invalid_hint));
        auto future = pool.Submit([] { return 7; });
        REQUIRE_EQ(future.get(), 7);
    }

    TEST(set_worker_topology_hint_valid_cpu_executes_on_single_worker)
    {
        Utility::ThreadPool pool(1u);
        Utility::ThreadPool::WorkerTopologyHint hint;
        hint.preferred_cpu = 0u;
        REQUIRE(pool.SetWorkerTopologyHint(0u, hint));
        auto future = pool.Submit([] { return 11; });
        REQUIRE_EQ(future.get(), 11);
    }

    TEST(set_worker_topology_hint_invalid_cpu_executes_on_single_worker)
    {
        Utility::ThreadPool pool(1u);
        Utility::ThreadPool::WorkerTopologyHint hint;
        hint.preferred_cpu = 1'000'000u;
        REQUIRE(pool.SetWorkerTopologyHint(0u, hint));
        auto future = pool.Submit([] { return 12; });
        REQUIRE_EQ(future.get(), 12);
    }

    TEST(wait_idle_timeout_then_success)
    {
        Utility::ThreadPool pool(1u);
        std::promise<void> gate;
        std::shared_future<void> gate_future = gate.get_future().share();

        pool.SubmitDetached(
            [gate_future]
            {
                gate_future.wait();
            });

        const bool timed_out = pool.WaitIdle(10ms);
        REQUIRE(!timed_out);

        gate.set_value();
        REQUIRE(pool.WaitIdle(1000ms));
    }

    TEST(wait_idle_returns_immediately_when_idle)
    {
        Utility::ThreadPool pool(2u);
        REQUIRE(pool.WaitIdle(1ms));
    }

    TEST(resize_grow_and_shrink_preserves_work)
    {
        Utility::ThreadPool pool(2u);
        REQUIRE_EQ(pool.Size(), 2u);

        pool.Resize(8u);
        REQUIRE_EQ(pool.Size(), 8u);

        std::atomic<int> counter = 0;
        for (int i = 0; i < 2000; ++i)
        {
            pool.SubmitDetached(
                [&counter]
                {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
        }

        pool.Resize(3u);
        REQUIRE_EQ(pool.Size(), 3u);
        pool.WaitIdle();
        REQUIRE_EQ(counter.load(std::memory_order_relaxed), 2000);
    }

    TEST(resize_to_zero_normalizes_to_one)
    {
        Utility::ThreadPool pool(4u);
        pool.Resize(0u);
        REQUIRE_EQ(pool.Size(), 1u);
    }

    TEST(resize_to_same_size_is_noop)
    {
        Utility::ThreadPool pool(3u);
        pool.Resize(3u);
        REQUIRE_EQ(pool.Size(), 3u);
        REQUIRE(pool.WaitIdle(100ms));
    }

    TEST(shutdown_rejects_new_submissions_and_is_idempotent)
    {
        Utility::ThreadPool pool(2u);

        std::atomic<int> counter = 0;
        for (int i = 0; i < 100; ++i)
        {
            pool.SubmitDetached(
                [&counter]
                {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
        }

        pool.Shutdown();
        REQUIRE(pool.IsStopping());
        REQUIRE(!pool.IsAcceptingSubmissions());
        REQUIRE_EQ(counter.load(std::memory_order_relaxed), 100);

        bool rejected = false;
        try
        {
            (void)pool.Submit(ReturnOne);
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }

        REQUIRE(rejected);

        bool detached_rejected = false;
        try
        {
            pool.SubmitDetached(NoopTask);
        }
        catch (const std::runtime_error&)
        {
            detached_rejected = true;
        }

        REQUIRE(detached_rejected);

        bool local_rejected = false;
        bool local_detached_rejected = false;
        try
        {
            (void)pool.SubmitToWorker(0u, MultiplyTwo, 2, 2);
        }
        catch (const std::runtime_error&)
        {
            local_rejected = true;
        }

        try
        {
            pool.SubmitDetachedToWorker(0u, IncrementGlobalDetachedCounter);
        }
        catch (const std::runtime_error&)
        {
            local_detached_rejected = true;
        }

        REQUIRE(local_rejected);
        REQUIRE(local_detached_rejected);
        pool.Shutdown();
    }

    TEST(shutdown_now_cancels_queued_tasks)
    {
        Utility::ThreadPool pool(1u);

        auto running = pool.Submit(
            []()
            {
                std::this_thread::sleep_for(50ms);
                return 1;
            });

        std::vector<std::future<int>> queued;
        for (int i = 0; i < 32; ++i)
        {
            queued.emplace_back(pool.Submit(
                [i]
                {
                    return i;
                }));
        }
        pool.ShutdownNow();
        pool.ShutdownNow();
        REQUIRE(running.valid());

        bool saw_broken_promise = false;
        for (std::future<int>& future : queued)
        {
            try
            {
                (void)future.get();
            }
            catch (const std::future_error& error)
            {
                if (error.code() == std::make_error_code(std::future_errc::broken_promise))
                {
                    saw_broken_promise = true;
                }
            }
            catch (...)
            {
            }
        }

        REQUIRE(saw_broken_promise);
        REQUIRE(pool.PendingTasks() <= 1u);
        pool.ShutdownNow();
    }

    TEST(resize_after_shutdown_throws)
    {
        Utility::ThreadPool pool(2u);
        pool.Shutdown();

        bool threw = false;
        try
        {
            pool.Resize(3u);
        }
        catch (const std::runtime_error&)
        {
            threw = true;
        }

        REQUIRE(threw);
    }

    TEST(concurrent_submit_under_load_stress)
    {
        Utility::ThreadPool pool(6u);
        std::atomic<std::uint64_t> sum = 0;
        std::vector<std::thread> producers;
        producers.reserve(8);

        for (int i = 0; i < 8; ++i)
        {
            producers.emplace_back(
                [&pool, &sum, i]
                {
                    for (int j = 0; j < 4000; ++j)
                    {
                        pool.SubmitDetached(
                            [&sum, i, j]
                            {
                                sum.fetch_add(static_cast<std::uint64_t>(i + j), std::memory_order_relaxed);
                            });
                    }
                });
        }

        for (std::thread& producer : producers)
        {
            producer.join();
        }

        pool.WaitIdle();
        REQUIRE(sum.load(std::memory_order_relaxed) > 0);
        REQUIRE_EQ(pool.PendingTasks(), 0u);
    }

    TEST(concurrent_resize_and_submit_stress)
    {
        Utility::ThreadPool pool(4u);
        std::atomic<bool> stop = false;
        std::atomic<int> executed = 0;

        std::thread resizer(
            [&pool, &stop]
            {
                std::size_t next = 1;
                while (!stop.load(std::memory_order_acquire))
                {
                    pool.Resize(next);
                    next = (next % 8u) + 1u;
                }
            });

        std::vector<std::thread> submitters;
        for (int i = 0; i < 6; ++i)
        {
            submitters.emplace_back(
                [&pool, &executed]
                {
                    for (int j = 0; j < 2000; ++j)
                    {
                        pool.SubmitDetached(
                            [&executed]
                            {
                                executed.fetch_add(1, std::memory_order_relaxed);
                            });
                    }
                });
        }

        for (std::thread& submitter : submitters)
        {
            submitter.join();
        }

        stop.store(true, std::memory_order_release);
        resizer.join();
        pool.WaitIdle();
        REQUIRE_EQ(pool.PendingTasks(), 0u);
        REQUIRE(executed.load(std::memory_order_relaxed) > 0);
    }

    TEST(concurrent_submit_and_shutdown_race_stress)
    {
        Utility::ThreadPool pool(4u);
        std::atomic<bool> stop_submit = false;
        std::atomic<int> accepted = 0;

        std::thread submitter(
            [&pool, &stop_submit, &accepted]
            {
                while (!stop_submit.load(std::memory_order_acquire))
                {
                    try
                    {
                        pool.SubmitDetached(
                            [&accepted]
                            {
                                accepted.fetch_add(1, std::memory_order_relaxed);
                            });
                    }
                    catch (const std::runtime_error&)
                    {
                        stop_submit.store(true, std::memory_order_release);
                    }
                }
            });

        std::this_thread::sleep_for(20ms);
        pool.Shutdown();
        stop_submit.store(true, std::memory_order_release);
        submitter.join();

        REQUIRE(accepted.load(std::memory_order_relaxed) >= 0);
        REQUIRE(!pool.IsAcceptingSubmissions());
    }

    TEST(work_stealing_heavy_recursive_fanout)
    {
        Utility::ThreadPool pool(8u);
        std::atomic<int> completed = 0;
        constexpr int fanout = 2000;

        auto root = pool.Submit(
            [&pool, &completed]
            {
                std::vector<std::future<void>> futures;
                futures.reserve(fanout);

                for (int i = 0; i < fanout; ++i)
                {
                    futures.emplace_back(pool.Submit(
                        [&completed]
                        {
                            completed.fetch_add(1, std::memory_order_relaxed);
                        }));
                }

                for (std::future<void>& future : futures)
                {
                    future.get();
                }
            });

        root.get();
        pool.WaitIdle();
        REQUIRE_EQ(completed.load(std::memory_order_relaxed), fanout);
        REQUIRE_EQ(pool.ActiveWorkers(), 0u);
    }

    TEST(resize_shrink_migrates_local_queue_to_global_queue)
    {
        Utility::ThreadPool pool(2u);
        std::atomic<int> count = 0;

        auto producer = pool.Submit(
            [&pool, &count]
            {
                for (int i = 0; i < 256; ++i)
                {
                    pool.SubmitDetached(
                        [&count]
                        {
                            std::this_thread::sleep_for(1ms);
                            count.fetch_add(1, std::memory_order_relaxed);
                        });
                }
            });

        producer.get();
        pool.Resize(1u);
        REQUIRE(pool.WaitIdle(5000ms));
        REQUIRE_EQ(count.load(std::memory_order_relaxed), 256);
    }

    TEST(repeated_create_destroy_stress)
    {
        for (int round = 0; round < 50; ++round)
        {
            Utility::ThreadPool pool(2u);
            std::atomic<int> count = 0;

            for (int i = 0; i < 100; ++i)
            {
                pool.SubmitDetached(
                    [&count]
                    {
                        count.fetch_add(1, std::memory_order_relaxed);
                    });
            }

            pool.WaitIdle();
            REQUIRE_EQ(count.load(std::memory_order_relaxed), 100);
        }
    }
}

int main()
{
    int failures = 0;

    for (const TestCase& test : Registry())
    {
        try
        {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const std::exception& exception)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << " :: " << exception.what() << '\n';
        }
        catch (...)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << " :: unknown exception\n";
        }
    }

    if (failures != 0)
    {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All tests passed: " << Registry().size() << '\n';
    return 0;
}
