#include "ThreadPool.hpp"
#include "Test.h"

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

#if defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define THREAD_POOL_TEST_WITH_TSAN 1
#  endif
#endif
#if defined(__SANITIZE_THREAD__)
#  define THREAD_POOL_TEST_WITH_TSAN 1
#endif

namespace
{
    std::atomic<long long> g_fail_allocation_after = -1;
}

#if !defined(THREAD_POOL_TEST_WITH_TSAN)
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
#endif

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


    TEST(construct_with_zero_normalizes_to_one)
    {
        Utility::ThreadPool pool(0u);
        REQUIRE_EQUAL(pool.Size(), 1u);
        REQUIRE(pool.IsAcceptingSubmissions());
    }

    TEST(submit_returns_values_and_handles_arguments)
    {
        Utility::ThreadPool pool(4u);
        auto future = pool.Submit(AddThree, 3, 4, 5);
        auto future_one = pool.Submit(ReturnOne);

        REQUIRE_EQUAL(future.get(), 12);
        REQUIRE_EQUAL(future_one.get(), 1);
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

        REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    }

    TEST(submit_packaging_covers_success_and_throw_branch)
    {
        Utility::ThreadPool pool(2u);
        ToggleThrowMove::throw_on_move.store(false, std::memory_order_release);
        auto success = pool.Submit(ConsumeToggleThrowMove, ToggleThrowMove{77});
        REQUIRE_EQUAL(success.get(), 77);

        ToggleThrowMove::throw_on_move.store(true, std::memory_order_release);
        CHECK_THROWS_AS(
            pool.Submit(ConsumeToggleThrowMove, ToggleThrowMove{13}),
            std::runtime_error);
        ToggleThrowMove::throw_on_move.store(false, std::memory_order_release);
    }

#if !defined(THREAD_POOL_TEST_WITH_TSAN)
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

    TEST(construction_allocation_failure_cleans_up_started_workers)
    {
        bool threw_bad_alloc = false;

        try
        {
            // With the unsafe creation order this fails while allocating the
            // worker slot after its thread has started, which destroys a
            // joinable std::thread and terminates the process.
            SetFailAllocationAfter(3);
            Utility::ThreadPool pool(1u);
        }
        catch (const std::bad_alloc&)
        {
            SetFailAllocationAfter(-1);
            threw_bad_alloc = true;
        }
        catch (...)
        {
            SetFailAllocationAfter(-1);
            throw;
        }

        SetFailAllocationAfter(-1);
        REQUIRE(threw_bad_alloc);
    }
#endif

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
        REQUIRE_EQUAL(value.load(std::memory_order_relaxed), 1000);
    }

    TEST(detached_task_exceptions_do_not_kill_workers)
    {
        Utility::ThreadPool pool(2u);
        std::atomic<int> completed = 0;

        pool.SubmitDetached(
            []
            {
                throw std::runtime_error("detached failure");
            });
        pool.SubmitDetachedToWorker(
            1u,
            []
            {
                throw 42;
            });

        auto generic_follow_up = pool.Submit(
            [&completed]
            {
                completed.fetch_add(1, std::memory_order_relaxed);
                return 11;
            });
        auto targeted_follow_up = pool.SubmitToWorker(
            1u,
            [&completed]
            {
                completed.fetch_add(1, std::memory_order_relaxed);
                return 22;
            });

        REQUIRE_EQUAL(generic_follow_up.get(), 11);
        REQUIRE_EQUAL(targeted_follow_up.get(), 22);
        REQUIRE(pool.WaitIdle(1s));
        REQUIRE_EQUAL(completed.load(std::memory_order_relaxed), 2);
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
        REQUIRE_EQUAL(pool.ActiveWorkers(), 0u);
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
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
        REQUIRE_EQUAL(pool.ActiveWorkers(), 0u);
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
        REQUIRE_EQUAL(sum.load(std::memory_order_relaxed), 256);
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

        REQUIRE_THROWS_AS(
            Utility::InitializeGlobalThreadPool(pool.Size(), other_policy),
            std::runtime_error);
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
        REQUIRE_EQUAL(counter.load(std::memory_order_relaxed), 200);
    }

    TEST(constructor_accepts_submission_policy)
    {
        Utility::ThreadPool pool(4u, Utility::ThreadPool::SubmissionPolicy::Random);
        REQUIRE(pool.GetSubmissionPolicy() == Utility::ThreadPool::SubmissionPolicy::Random);
        REQUIRE_EQUAL(pool.Size(), 4u);
    }

    TEST(local_queue_submit_and_detached_submit)
    {
        Utility::ThreadPool pool(2u, Utility::ThreadPool::SubmissionPolicy::RoundRobin);
        g_detached_counter.store(0, std::memory_order_relaxed);

        auto value_future = pool.SubmitToWorker(0u, MultiplyTwo, 6, 7);
        REQUIRE_EQUAL(value_future.get(), 42);

        for (int i = 0; i < 128; ++i)
        {
            pool.SubmitDetachedToWorker(1u, IncrementGlobalDetachedCounter);
        }

        pool.WaitIdle();
        REQUIRE_EQUAL(g_detached_counter.load(std::memory_order_relaxed), 128);
    }

    TEST(local_queue_submit_invalid_worker_index_throws)
    {
        Utility::ThreadPool pool(2u);
        REQUIRE_THROWS_AS(
            pool.SubmitToWorker(99u, MultiplyTwo, 1, 2),
            std::out_of_range);
        REQUIRE_THROWS_AS(
            pool.SubmitDetachedToWorker(99u, IncrementGlobalDetachedCounter),
            std::out_of_range);
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
        REQUIRE_EQUAL(future.get(), 7);
    }

    TEST(set_worker_topology_hint_valid_cpu_executes_on_single_worker)
    {
        Utility::ThreadPool pool(1u);
        Utility::ThreadPool::WorkerTopologyHint hint;
        hint.preferred_cpu = 0u;
        REQUIRE(pool.SetWorkerTopologyHint(0u, hint));
        auto future = pool.Submit([] { return 11; });
        REQUIRE_EQUAL(future.get(), 11);
    }

    TEST(set_worker_topology_hint_invalid_cpu_executes_on_single_worker)
    {
        Utility::ThreadPool pool(1u);
        Utility::ThreadPool::WorkerTopologyHint hint;
        hint.preferred_cpu = 1'000'000u;
        REQUIRE(pool.SetWorkerTopologyHint(0u, hint));
        auto future = pool.Submit([] { return 12; });
        REQUIRE_EQUAL(future.get(), 12);
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

    TEST(task_counters_report_outstanding_and_active_work)
    {
        Utility::ThreadPool pool(1u);
        std::promise<void> release_running_task;
        std::shared_future<void> release = release_running_task.get_future().share();

        auto running = pool.Submit(
            [release]
            {
                release.wait();
                return 1;
            });

        while (pool.ActiveWorkers() != 1u)
        {
            std::this_thread::yield();
        }

        auto queued = pool.Submit([] { return 2; });
        REQUIRE_EQUAL(pool.ActiveWorkers(), 1u);
        REQUIRE_EQUAL(pool.ExecutingTasks(), 1u);
        REQUIRE_EQUAL(pool.PendingTasks(), 2u);
        REQUIRE_EQUAL(pool.QueuedTasks(), 1u);

        release_running_task.set_value();
        REQUIRE_EQUAL(running.get(), 1);
        REQUIRE_EQUAL(queued.get(), 2);
        REQUIRE(pool.WaitIdle(1s));
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
        REQUIRE_EQUAL(pool.QueuedTasks(), 0u);
        REQUIRE_EQUAL(pool.ExecutingTasks(), 0u);
        REQUIRE_EQUAL(pool.ActiveWorkers(), 0u);
    }

    TEST(all_concurrent_idle_waiters_are_released_on_last_completion)
    {
        Utility::ThreadPool pool(2u);
        std::promise<void> release_task;
        std::shared_future<void> release = release_task.get_future().share();
        std::atomic<int> waiting = 0;
        std::atomic<int> returned = 0;

        pool.SubmitDetached([release] { release.wait(); });

        std::vector<std::thread> waiters;
        for (int i = 0; i < 12; ++i)
        {
            waiters.emplace_back(
                [&pool, &waiting, &returned]
                {
                    waiting.fetch_add(1, std::memory_order_release);
                    pool.WaitIdle();
                    returned.fetch_add(1, std::memory_order_release);
                });
        }

        while (waiting.load(std::memory_order_acquire) != 12)
        {
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(10ms);
        const int returned_while_busy = returned.load(std::memory_order_acquire);
        release_task.set_value();

        for (std::thread& waiter : waiters)
        {
            waiter.join();
        }

        REQUIRE_EQUAL(returned_while_busy, 0);
        REQUIRE_EQUAL(returned.load(std::memory_order_acquire), 12);
    }

    TEST(direct_worker_submissions_wake_sleeping_target_workers)
    {
        Utility::ThreadPool pool(4u);
        std::atomic<int> completed = 0;

        for (int round = 0; round < 100; ++round)
        {
            REQUIRE(pool.WaitIdle(500ms));
            for (std::size_t worker = 0; worker < pool.Size(); ++worker)
            {
                pool.SubmitDetachedToWorker(
                    worker,
                    [&completed]
                    {
                        completed.fetch_add(1, std::memory_order_relaxed);
                    });
            }
        }

        REQUIRE(pool.WaitIdle(2s));
        REQUIRE_EQUAL(completed.load(std::memory_order_relaxed), 400);
    }

    TEST(resize_grow_and_shrink_preserves_work)
    {
        Utility::ThreadPool pool(2u);
        REQUIRE_EQUAL(pool.Size(), 2u);

        pool.Resize(8u);
        REQUIRE_EQUAL(pool.Size(), 8u);

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
        REQUIRE_EQUAL(pool.Size(), 3u);
        pool.WaitIdle();
        REQUIRE_EQUAL(counter.load(std::memory_order_relaxed), 2000);
    }

    TEST(resize_to_zero_normalizes_to_one)
    {
        Utility::ThreadPool pool(4u);
        pool.Resize(0u);
        REQUIRE_EQUAL(pool.Size(), 1u);
    }

    TEST(resize_to_same_size_is_noop)
    {
        Utility::ThreadPool pool(3u);
        pool.Resize(3u);
        REQUIRE_EQUAL(pool.Size(), 3u);
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
        REQUIRE_EQUAL(counter.load(std::memory_order_relaxed), 100);

        REQUIRE_THROWS_AS(pool.Submit(ReturnOne), std::runtime_error);
        REQUIRE_THROWS_AS(pool.SubmitDetached(NoopTask), std::runtime_error);
        REQUIRE_THROWS_AS(
            pool.SubmitToWorker(0u, MultiplyTwo, 2, 2),
            std::runtime_error);
        REQUIRE_THROWS_AS(
            pool.SubmitDetachedToWorker(0u, IncrementGlobalDetachedCounter),
            std::runtime_error);
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

    TEST(concurrent_shutdown_callers_wait_for_running_workers)
    {
        Utility::ThreadPool pool(2u);
        std::promise<void> release_tasks;
        std::shared_future<void> release = release_tasks.get_future().share();
        std::atomic<int> started = 0;
        std::atomic<int> shutdown_returns = 0;

        for (int i = 0; i < 2; ++i)
        {
            pool.SubmitDetached(
                [release, &started]
                {
                    started.fetch_add(1, std::memory_order_release);
                    release.wait();
                });
        }
        while (started.load(std::memory_order_acquire) != 2)
        {
            std::this_thread::yield();
        }

        std::thread first([&pool, &shutdown_returns]
            {
                pool.Shutdown();
                shutdown_returns.fetch_add(1, std::memory_order_release);
            });
        std::thread second([&pool, &shutdown_returns]
            {
                pool.Shutdown();
                shutdown_returns.fetch_add(1, std::memory_order_release);
            });

        std::this_thread::sleep_for(20ms);
        const int returned_before_release = shutdown_returns.load(std::memory_order_acquire);
        release_tasks.set_value();
        first.join();
        second.join();

        REQUIRE_EQUAL(returned_before_release, 0);
        REQUIRE_EQUAL(shutdown_returns.load(std::memory_order_acquire), 2);
    }

    TEST(graceful_and_immediate_shutdown_are_serialized)
    {
        Utility::ThreadPool pool(1u);
        std::promise<void> release_task;
        std::shared_future<void> release = release_task.get_future().share();
        std::atomic<bool> started = false;
        std::atomic<int> shutdown_returns = 0;

        pool.SubmitDetached(
            [release, &started]
            {
                started.store(true, std::memory_order_release);
                release.wait();
            });
        while (!started.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        std::thread graceful([&pool, &shutdown_returns]
            {
                pool.Shutdown();
                shutdown_returns.fetch_add(1, std::memory_order_release);
            });
        std::thread immediate([&pool, &shutdown_returns]
            {
                pool.ShutdownNow();
                shutdown_returns.fetch_add(1, std::memory_order_release);
            });

        std::this_thread::sleep_for(20ms);
        const int returned_before_release = shutdown_returns.load(std::memory_order_acquire);
        release_task.set_value();
        graceful.join();
        immediate.join();

        REQUIRE_EQUAL(returned_before_release, 0);
        REQUIRE_EQUAL(shutdown_returns.load(std::memory_order_acquire), 2);
        REQUIRE(!pool.IsAcceptingSubmissions());
    }

    TEST(resize_after_shutdown_throws)
    {
        Utility::ThreadPool pool(2u);
        pool.Shutdown();

        REQUIRE_THROWS_AS(pool.Resize(3u), std::runtime_error);
    }

    TEST(worker_initiated_lifecycle_operations_are_rejected)
    {
        Utility::ThreadPool pool(1u);

        auto rejected_operations = pool.Submit(
            [&pool]
            {
                int rejected = 0;

                try
                {
                    pool.Resize(2u);
                }
                catch (const std::logic_error&)
                {
                    ++rejected;
                }

                try
                {
                    pool.Shutdown();
                }
                catch (const std::logic_error&)
                {
                    ++rejected;
                }

                try
                {
                    pool.ShutdownNow();
                }
                catch (const std::logic_error&)
                {
                    ++rejected;
                }

                return rejected;
            });

        REQUIRE_EQUAL(rejected_operations.get(), 3);
        REQUIRE_EQUAL(pool.Size(), 1u);
        REQUIRE_EQUAL(pool.Submit(ReturnOne).get(), 1);
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
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
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
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
        REQUIRE_EQUAL(executed.load(std::memory_order_relaxed), 12'000);
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
        REQUIRE_EQUAL(completed.load(std::memory_order_relaxed), fanout);
        REQUIRE_EQUAL(pool.ActiveWorkers(), 0u);
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
        REQUIRE_EQUAL(count.load(std::memory_order_relaxed), 256);
    }

    TEST(nested_submission_to_a_different_pool_uses_the_destination_pool)
    {
        Utility::ThreadPool source(1u);
        Utility::ThreadPool destination(1u);

        auto outer = source.Submit(
            [&destination]
            {
                auto inner = destination.Submit([] { return std::this_thread::get_id(); });
                return std::pair{std::this_thread::get_id(), inner.get()};
            });

        const auto [source_thread, destination_thread] = outer.get();
        REQUIRE(source_thread != destination_thread);
        REQUIRE(source.WaitIdle(1s));
        REQUIRE(destination.WaitIdle(1s));
    }

    TEST(topology_updates_racing_with_resize_remain_safe)
    {
        Utility::ThreadPool pool(4u);
        std::atomic<bool> stop = false;
        std::atomic<int> updates = 0;
        std::atomic<int> executed = 0;

        std::thread updater(
            [&pool, &stop, &updates]
            {
                Utility::ThreadPool::WorkerTopologyHint hint;
                hint.preferred_cpu = 1'000'000u;
                while (!stop.load(std::memory_order_acquire))
                {
                    const std::size_t size = pool.Size();
                    if (pool.SetWorkerTopologyHint(size - 1u, hint))
                    {
                        updates.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });

        for (int round = 0; round < 80; ++round)
        {
            pool.Resize(static_cast<std::size_t>((round % 6) + 1));
            pool.SubmitDetached(
                [&executed]
                {
                    executed.fetch_add(1, std::memory_order_relaxed);
                });
        }

        stop.store(true, std::memory_order_release);
        updater.join();
        REQUIRE(pool.WaitIdle(5s));
        REQUIRE(updates.load(std::memory_order_relaxed) > 0);
        REQUIRE_EQUAL(executed.load(std::memory_order_relaxed), 80);
    }

    TEST(wait_idle_and_shutdown_now_agree_after_queue_cancellation)
    {
        Utility::ThreadPool pool(1u);
        std::promise<void> release_task;
        std::shared_future<void> release = release_task.get_future().share();
        std::atomic<bool> running = false;
        std::atomic<bool> waiter_returned = false;

        pool.SubmitDetached(
            [release, &running]
            {
                running.store(true, std::memory_order_release);
                release.wait();
            });
        while (!running.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        std::vector<std::future<int>> canceled;
        for (int i = 0; i < 64; ++i)
        {
            canceled.emplace_back(pool.Submit([i] { return i; }));
        }

        std::thread waiter(
            [&pool, &waiter_returned]
            {
                pool.WaitIdle();
                waiter_returned.store(true, std::memory_order_release);
            });
        std::thread stopper([&pool] { pool.ShutdownNow(); });

        while (pool.IsAcceptingSubmissions())
        {
            std::this_thread::yield();
        }
        std::this_thread::sleep_for(10ms);
        const bool returned_while_task_active = waiter_returned.load(std::memory_order_acquire);
        release_task.set_value();
        stopper.join();
        waiter.join();

        std::size_t broken_promises = 0;
        for (std::future<int>& future : canceled)
        {
            try
            {
                (void)future.get();
            }
            catch (const std::future_error& error)
            {
                if (error.code() == std::make_error_code(std::future_errc::broken_promise))
                {
                    ++broken_promises;
                }
            }
        }

        REQUIRE(!returned_while_task_active);
        REQUIRE_EQUAL(broken_promises, canceled.size());
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
        REQUIRE_EQUAL(pool.ActiveWorkers(), 0u);
    }

    TEST(active_worker_submission_is_rejected_once_shutdown_begins)
    {
        Utility::ThreadPool pool(1u);
        std::promise<void> attempt_submission;
        std::shared_future<void> attempt = attempt_submission.get_future().share();
        std::atomic<bool> worker_started = false;
        std::atomic<bool> rejected = false;

        pool.SubmitDetached(
            [&pool, attempt, &worker_started, &rejected]
            {
                worker_started.store(true, std::memory_order_release);
                attempt.wait();
                try
                {
                    pool.SubmitDetached(NoopTask);
                }
                catch (const std::runtime_error&)
                {
                    rejected.store(true, std::memory_order_release);
                }
            });
        while (!worker_started.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        std::thread stopper([&pool] { pool.Shutdown(); });
        while (pool.IsAcceptingSubmissions())
        {
            std::this_thread::yield();
        }
        attempt_submission.set_value();
        stopper.join();

        REQUIRE(rejected.load(std::memory_order_acquire));
        REQUIRE_EQUAL(pool.PendingTasks(), 0u);
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
            REQUIRE_EQUAL(count.load(std::memory_order_relaxed), 100);
        }
    }
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
