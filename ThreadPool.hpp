#ifndef SRC_UTILITY_THREAD_POOL_HPP
#define SRC_UTILITY_THREAD_POOL_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <limits>
#include <optional>
#include <random>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace Utility
{
    /*
     * ============================================================
     *  Utility::ThreadPool  —  Reference and Usage Guide
     * ============================================================
     *
     * OVERVIEW
     * --------
     * ThreadPool maintains a fixed (or dynamically resized) set of worker
     * threads that execute submitted tasks concurrently.  It eliminates
     * per-task thread creation overhead and provides work-stealing load
     * balancing, graceful and immediate shutdown, and best-effort CPU
     * affinity hints.
     *
     *
     * HOW TO USE
     * ----------
     *
     * 1. Basic: submit tasks and collect results
     *
     *     Utility::ThreadPool pool(4); // 4 worker threads
     *
     *     auto f = pool.Submit([](int x) { return x * x; }, 7);
     *     int result = f.get(); // result == 49
     *
     *     pool.SubmitDetached([] { write_log("done"); });
     *     pool.WaitIdle(); // wait until all tasks finish
     *
     *
     * 2. Choose submission policy at construction
     *
     *     using Policy = Utility::ThreadPool::SubmissionPolicy;
     *
     *     // Round-robin distributes tasks evenly across workers (default)
     *     Utility::ThreadPool pool_rr(8, Policy::RoundRobin);
     *
     *     // Random picks a worker at random — lower contention at high submit rates
     *     Utility::ThreadPool pool_rand(8, Policy::Random);
     *
     *
     * 3. Submit directly to a specific worker's local queue
     *
     *     // Useful when you know a task should run on a particular CPU or
     *     // near data that is already cached on a worker.
     *     auto f = pool.SubmitToWorker(0, compute_shard, shard_data);
     *     pool.SubmitDetachedToWorker(1, flush_shard, other_data);
     *
     *
     * 4. Wait for completion
     *
     *     pool.WaitIdle();                        // blocks until all tasks done
     *     bool ok = pool.WaitIdle(500ms);         // returns false on timeout
     *
     *
     * 5. Resize the pool while running
     *
     *     pool.Resize(16);   // grow: new workers start immediately
     *     pool.Resize(4);    // shrink: excess workers finish current task,
     *                        //         pending local tasks migrate to survivors
     *
     *
     * 6. Pin workers to CPUs (Linux only, best-effort)
     *
     *     Utility::ThreadPool::WorkerTopologyHint hint;
     *     hint.preferred_cpu = 3u;   // request thread affinity to CPU 3
     *     pool.SetWorkerTopologyHint(0, hint);   // applied on worker's next task
     *
     *
     * 7. Shutdown
     *
     *     pool.Shutdown();     // wait for all queued tasks to finish, then stop
     *     pool.ShutdownNow();  // discard queued tasks, let running tasks finish
     *
     *     // The destructor calls Shutdown() automatically.
     *
     *
     * 8. Global singleton pool
     *
     *     // Optional explicit init (must be called before first GlobalThreadPool())
     *     Utility::InitializeGlobalThreadPool(8, Policy::RoundRobin);
     *
     *     // Access from anywhere; lazily created with hardware_concurrency() default
     *     auto& pool = Utility::GlobalThreadPool();
     *     auto f = pool.Submit(my_work);
     *
     *
     * PUBLIC API REFERENCE
     * --------------------
     *
     * Types
     * -----
     *
     *   enum class SubmissionPolicy
     *     RoundRobin  — tasks distributed to workers in rotating order.
     *                   Deterministic, even distribution.  Default.
     *     Random      — tasks sent to a randomly chosen worker.
     *                   Lower mutex contention under high submit throughput.
     *
     *   struct WorkerTopologyHint
     *     preferred_cpu        — optional<uint32_t>: request thread affinity to
     *                            this logical CPU index (Linux: pthread_setaffinity).
     *     preferred_numa_node  — optional<uint32_t>: informational NUMA preference;
     *                            stored but not enforced by the scheduler.
     *
     *
     * Constructors
     * ------------
     *
     *   ThreadPool()
     *   ThreadPool(uint32_t pool_size)
     *   ThreadPool(uint32_t pool_size, SubmissionPolicy policy)
     *   ThreadPool(size_t   pool_size, SubmissionPolicy policy = RoundRobin)
     *     Create a pool with the given number of worker threads and submission
     *     policy.  pool_size == 0 is normalized to 1.  Default pool_size is
     *     std::thread::hardware_concurrency() (min 1).
     *     Throws std::system_error if thread creation fails mid-way; in that
     *     case already-started threads are joined before the exception propagates.
     *
     *   ThreadPool is not copyable or movable.
     *
     *
     * Task Submission
     * ---------------
     *
     *   template<Func, Args...>
     *   std::future<Result> Submit(Func&& func, Args&&... args)
     *     Enqueue a callable with arguments.  Returns a future that becomes
     *     ready when the task finishes.  Call future.get() to retrieve the
     *     return value or re-throw any exception the task threw.
     *     Throws std::runtime_error if the pool is shutting down or shut down.
     *
     *   template<Func, Args...>
     *   std::future<Result> SubmitToWorker(size_t worker_index, Func&&, Args&&...)
     *     Like Submit(), but places the task directly in the local queue of
     *     the worker at the given index.  Use this to co-locate tasks that
     *     share cache-hot data on a specific worker.
     *     Throws std::out_of_range  if worker_index >= pool size.
     *     Throws std::runtime_error if the pool is shutting down or shut down.
     *
     *   template<Func, Args...>
     *   void SubmitDetached(Func&& func, Args&&... args)
     *     Enqueue a fire-and-forget task.  No future is returned; any return
     *     value and any exception the task throws are silently discarded.
     *     Throws std::runtime_error if the pool is shutting down or shut down.
     *
     *   template<Func, Args...>
     *   void SubmitDetachedToWorker(size_t worker_index, Func&&, Args&&...)
     *     Like SubmitDetached(), but targets the local queue of the specified
     *     worker.
     *     Throws std::out_of_range  if worker_index >= pool size.
     *     Throws std::runtime_error if the pool is shutting down or shut down.
     *
     *
     * Policy and Topology
     * -------------------
     *
     *   void SetSubmissionPolicy(SubmissionPolicy policy)
     *     Change the distribution policy used by Submit() and SubmitDetached()
     *     for future submissions.  Thread-safe; takes effect immediately.
     *
     *   SubmissionPolicy GetSubmissionPolicy() const
     *     Return the current submission policy.
     *
     *   bool SetWorkerTopologyHint(size_t worker_index, WorkerTopologyHint hint)
     *     Store a CPU/NUMA preference for the worker at the given index.
     *     The hint is applied on the worker's next task execution.
     *     On Linux, preferred_cpu triggers pthread_setaffinity_np; failure is
     *     non-fatal.  preferred_numa_node is stored but not enforced.
     *     Returns true on success, false if worker_index is out of range.
     *
     *
     * Inspection
     * ----------
     *
     *   size_t Size() const
     *     Number of active worker threads currently in the pool.
     *
     *   size_t PendingTasks() const
     *     Approximate count of tasks queued but not yet started.  This
     *     decrements when a worker picks up a task, before execution finishes.
     *
     *   size_t ActiveWorkers() const
     *     Number of workers currently executing a task.
     *
     *   bool IsStopping() const
     *     True once Shutdown() or ShutdownNow() has been called.
     *
     *   bool IsAcceptingSubmissions() const
     *     True while the pool is healthy and not shutting down.
     *
     *
     * Waiting
     * -------
     *
     *   void WaitIdle()
     *     Block the calling thread until all pending and active tasks complete.
     *     Safe to call from any thread, including from within a task itself
     *     (though calling from a task with no idle capacity will deadlock).
     *
     *   bool WaitIdle(std::chrono::milliseconds timeout)
     *     Same as WaitIdle() but returns after at most `timeout` milliseconds.
     *     Returns true if idle was reached, false if timed out.
     *
     *
     * Lifecycle
     * ---------
     *
     *   void Resize(size_t new_size)
     *     Grow or shrink the pool to new_size workers (min 1).
     *     - Grow: new threads start immediately and begin stealing/receiving tasks.
     *     - Shrink: excess workers are signalled to retire after finishing their
     *       current task; their pending local tasks are migrated to remaining workers
     *       via the global injection queue before the threads are joined.
     *     Throws std::runtime_error if called after Shutdown()/ShutdownNow().
     *     Blocks until all retiring threads have exited when shrinking.
     *
     *   void Shutdown()
     *     Signal all workers to stop and block until every queued task has
     *     completed and all threads have been joined.  New submissions after
     *     this call throw std::runtime_error.  Safe to call multiple times
     *     (second and subsequent calls are no-ops).
     *     The destructor calls Shutdown() automatically.
     *
     *   void ShutdownNow()
     *     Immediately discard all tasks that have not yet started (queued tasks
     *     are erased from all queues), signal workers to exit, and block until
     *     all threads have been joined.  Tasks that are already executing are
     *     allowed to finish normally.  The associated futures for discarded tasks
     *     will have broken_promise state.
     *     Safe to call multiple times.  Called automatically by the destructor
     *     only when Shutdown() has not been called first.
     *
     *
     * SCHEDULING INTERNALS
     * --------------------
     * - Each worker owns a private std::deque<Task>.
     * - Owner pops from the back (LIFO: cache-warm, low interference).
     * - Thieves steal from the front (FIFO: avoids touching recently pushed items).
     * - External submitters distribute tasks across worker queues via SubmissionPolicy.
     * - Nested Submit() calls from within a running task route directly to the same
     *   worker's local queue when the worker has not been marked for retirement.
     * - A global injection deque receives tasks migrated from retiring workers; all
     *   remaining workers compete for those.
     *
     *
     * CONCURRENCY GUARANTEES
     * ----------------------
     * - All public methods are thread-safe.
     * - Task code never executes while any pool-internal lock is held.
     * - Shutdown() drains tasks in submission order on a best-effort basis
     *   (exact ordering depends on steal activity).
     */
    class ThreadPool
    {
    public:
        enum class SubmissionPolicy
        {
            RoundRobin,
            Random
        };

        struct WorkerTopologyHint
        {
            std::optional<std::uint32_t> preferred_cpu;
            std::optional<std::uint32_t> preferred_numa_node;
        };

    private:
        using Task = std::function<void()>;

        struct WorkerState
        {
            mutable std::mutex mutex;
            std::deque<Task> local_queue;
            std::atomic<bool> retire_requested = false;
            WorkerTopologyHint topology_hint;
            std::atomic<std::uint64_t> topology_generation = 0;
        };

        struct WorkerSlot
        {
            std::shared_ptr<WorkerState> state;
            std::thread thread;
        };

        template <class Callable, class... StoredArgs>
        struct InvocationState
        {
            Callable callable;
            std::tuple<StoredArgs...> arguments;

            InvocationState(Callable&& provided_callable, StoredArgs&&... provided_arguments)
                : callable(std::move(provided_callable)),
                  arguments(std::forward<StoredArgs>(provided_arguments)...)
            {
            }
        };

        mutable std::shared_mutex m_WorkersMutex;
        std::vector<std::unique_ptr<WorkerSlot>> m_Workers;

        mutable std::mutex m_GlobalQueueMutex;
        std::deque<Task> m_GlobalQueue;

        mutable std::mutex m_WaitMutex;
        std::condition_variable m_WorkAvailableCondition;
        std::condition_variable m_IdleCondition;

        mutable std::mutex m_SubmissionMutex;
        mutable std::mutex m_ControlMutex;

        std::atomic<std::size_t> m_PendingTasks = 0;
        std::atomic<std::size_t> m_ActiveWorkers = 0;
        std::atomic<std::size_t> m_RoundRobinIndex = 0;
        std::atomic<std::size_t> m_StealCursor = 0;
        std::atomic<bool> m_AcceptSubmissions = true;
        std::atomic<bool> m_StopRequested = false;
        std::atomic<bool> m_ImmediateStopRequested = false;
        std::atomic<SubmissionPolicy> m_SubmissionPolicy = SubmissionPolicy::RoundRobin;
        std::atomic<bool> m_ShutdownCompleted = false;

        static thread_local ThreadPool* s_CurrentPool;
        static thread_local WorkerState* s_CurrentWorkerState;

    private:
        static std::size_t NormalizeSize(std::size_t requested_size)
        {
            if (requested_size == 0)
            {
                return 1;
            }

            return requested_size;
        }

        static std::size_t NormalizeSize(std::uint32_t requested_size)
        {
            return NormalizeSize(static_cast<std::size_t>(requested_size));
        }

        bool TryPopLocalTask(WorkerState& worker, Task& task)
        {
            std::lock_guard lock(worker.mutex);

            if (worker.local_queue.empty())
            {
                return false;
            }

            task = std::move(worker.local_queue.back());
            worker.local_queue.pop_back();
            return true;
        }

        bool TryPopGlobalTask(Task& task)
        {
            std::lock_guard lock(m_GlobalQueueMutex);

            if (m_GlobalQueue.empty())
            {
                return false;
            }

            task = std::move(m_GlobalQueue.front());
            m_GlobalQueue.pop_front();
            return true;
        }

        bool TryStealTask(WorkerState& thief, Task& task)
        {
            std::vector<std::shared_ptr<WorkerState>> snapshot;

            {
                std::shared_lock workers_lock(m_WorkersMutex);
                snapshot.reserve(m_Workers.size());

                for (const std::unique_ptr<WorkerSlot>& slot : m_Workers)
                {
                    snapshot.push_back(slot->state);
                }
            }

            if (snapshot.size() <= 1)
            {
                return false;
            }

            const std::size_t start = m_StealCursor.fetch_add(1, std::memory_order_relaxed);

            for (std::size_t i = 0; i < snapshot.size(); ++i)
            {
                std::shared_ptr<WorkerState>& victim = snapshot[(start + i) % snapshot.size()];

                if (!victim || victim.get() == &thief)
                {
                    continue;
                }

                std::lock_guard lock(victim->mutex);

                if (victim->local_queue.empty())
                {
                    continue;
                }

                task = std::move(victim->local_queue.front());
                victim->local_queue.pop_front();
                return true;
            }

            return false;
        }

        bool TryAcquireTask(WorkerState& worker, Task& task)
        {
            return TryPopLocalTask(worker, task)
                || TryPopGlobalTask(task)
                || TryStealTask(worker, task);
        }

        std::size_t WorkerCountUnsafe() const
        {
            return m_Workers.size();
        }

        std::size_t PickExternalQueueIndex(std::size_t worker_count)
        {
            if (worker_count == 1)
            {
                return 0;
            }

            if (m_SubmissionPolicy.load(std::memory_order_relaxed) == SubmissionPolicy::RoundRobin)
            {
                return m_RoundRobinIndex.fetch_add(1, std::memory_order_relaxed) % worker_count;
            }

            thread_local std::mt19937_64 random_engine(std::random_device{}());
            std::uniform_int_distribution<std::size_t> distribution(0, worker_count - 1);
            return distribution(random_engine);
        }

        void EnsureAcceptingSubmissionsOrThrow() const
        {
            if (!m_AcceptSubmissions.load(std::memory_order_acquire))
            {
                throw std::runtime_error("Cannot submit task to stopped ThreadPool");
            }
        }

        void EnqueueTaskUnlocked(Task&& task)
        {
            WorkerState* target_worker = nullptr;

            if (s_CurrentPool == this && s_CurrentWorkerState != nullptr && !s_CurrentWorkerState->retire_requested.load(std::memory_order_acquire))
            {
                target_worker = s_CurrentWorkerState;
            }
            else
            {
                std::shared_lock workers_lock(m_WorkersMutex);

                const std::size_t worker_count = WorkerCountUnsafe();

                const std::size_t index = PickExternalQueueIndex(worker_count);
                target_worker = m_Workers[index]->state.get();
            }

            std::lock_guard worker_lock(target_worker->mutex);
            target_worker->local_queue.emplace_back(std::move(task));
            {
                // The predicate observed by WorkerLoop must be changed while
                // holding the mutex used by the condition variable.  Without
                // this lock a worker can test the predicate, miss this notify,
                // and then sleep despite there being queued work.
                std::lock_guard wait_lock(m_WaitMutex);
                m_PendingTasks.fetch_add(1, std::memory_order_release);
            }
            m_WorkAvailableCondition.notify_one();
        }

        void EnqueueTaskToWorkerUnlocked(std::size_t worker_index, Task&& task)
        {
            std::shared_ptr<WorkerState> target_worker;
            {
                std::shared_lock workers_lock(m_WorkersMutex);
                if (worker_index >= m_Workers.size())
                {
                    throw std::out_of_range("Worker index out of range for local queue submission");
                }
                target_worker = m_Workers[worker_index]->state;
            }

            std::lock_guard worker_lock(target_worker->mutex);
            target_worker->local_queue.emplace_back(std::move(task));
            {
                std::lock_guard wait_lock(m_WaitMutex);
                m_PendingTasks.fetch_add(1, std::memory_order_release);
            }
            m_WorkAvailableCondition.notify_one();
        }

        static bool ApplyBestEffortAffinity(const WorkerTopologyHint& hint)
        {
#if defined(__linux__)
            if (hint.preferred_cpu.has_value())
            {
                if (*hint.preferred_cpu >= static_cast<std::uint32_t>(CPU_SETSIZE))
                {
                    return false;
                }

                cpu_set_t cpu_set;
                CPU_ZERO(&cpu_set);
                CPU_SET(static_cast<int>(*hint.preferred_cpu), &cpu_set);

                return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) == 0;
            }
#else
            (void)hint;
#endif

            return true;
        }

        template <class Func, class... Args>
        static auto MakeInvocationState(Func&& func, Args&&... args)
        {
            using Callable = std::decay_t<Func>;
            using State = InvocationState<Callable, std::decay_t<Args>...>;

            return std::make_shared<State>(
                Callable(std::forward<Func>(func)),
                std::decay_t<Args>(std::forward<Args>(args))...);
        }

        void WorkerLoop(std::shared_ptr<WorkerState> worker_state)
        {
            s_CurrentPool = this;
            s_CurrentWorkerState = worker_state.get();

            std::uint64_t applied_topology_generation = std::numeric_limits<std::uint64_t>::max();

            while (true)
            {
                if (m_ImmediateStopRequested.load(std::memory_order_acquire))
                {
                    break;
                }

                Task task;

                if (TryAcquireTask(*worker_state, task))
                {
                    const std::uint64_t generation = worker_state->topology_generation.load(std::memory_order_acquire);
                    if (generation != applied_topology_generation)
                    {
                        WorkerTopologyHint hint;
                        {
                            std::lock_guard worker_lock(worker_state->mutex);
                            hint = worker_state->topology_hint;
                        }

                        ApplyBestEffortAffinity(hint);
                        applied_topology_generation = generation;
                    }

                    {
                        std::lock_guard wait_lock(m_WaitMutex);
                        m_ActiveWorkers.fetch_add(1, std::memory_order_acq_rel);
                    }
                    task();
                    {
                        // Serialize the transition to idle with WaitIdle's
                        // predicate check so its notification cannot be lost.
                        std::lock_guard wait_lock(m_WaitMutex);
                        m_ActiveWorkers.fetch_sub(1, std::memory_order_acq_rel);
                        m_PendingTasks.fetch_sub(1, std::memory_order_acq_rel);
                    }
                    m_IdleCondition.notify_all();
                    continue;
                }

                if (worker_state->retire_requested.load(std::memory_order_acquire)
                    && m_PendingTasks.load(std::memory_order_acquire) == 0)
                {
                    break;
                }

                if (m_StopRequested.load(std::memory_order_acquire)
                    && m_PendingTasks.load(std::memory_order_acquire) == 0)
                {
                    break;
                }

                std::unique_lock wait_lock(m_WaitMutex);
                m_WorkAvailableCondition.wait(wait_lock, [this, worker_state]
                    {
                        return m_ImmediateStopRequested.load(std::memory_order_acquire)
                            || m_PendingTasks.load(std::memory_order_acquire) != 0
                            || (worker_state->retire_requested.load(std::memory_order_acquire)
                                && m_PendingTasks.load(std::memory_order_acquire) == 0)
                            || (m_StopRequested.load(std::memory_order_acquire)
                                && m_PendingTasks.load(std::memory_order_acquire) == 0);
                    });
            }

            s_CurrentWorkerState = nullptr;
            s_CurrentPool = nullptr;
        }

        std::unique_ptr<WorkerSlot> CreateWorkerSlot()
        {
            std::shared_ptr<WorkerState> state = std::make_shared<WorkerState>();
            std::thread thread(&ThreadPool::WorkerLoop, this, state);

            auto slot = std::make_unique<WorkerSlot>();
            slot->state = std::move(state);
            slot->thread = std::move(thread);
            return slot;
        }

        static std::size_t MoveLocalQueueToGlobal(WorkerState& state, std::deque<Task>& global_queue)
        {
            std::lock_guard worker_lock(state.mutex);

            const std::size_t moved = state.local_queue.size();

            while (!state.local_queue.empty())
            {
                global_queue.emplace_back(std::move(state.local_queue.front()));
                state.local_queue.pop_front();
            }

            return moved;
        }

        void JoinAll(std::vector<std::unique_ptr<WorkerSlot>>&& slots)
        {
            for (std::unique_ptr<WorkerSlot>& slot : slots)
            {
                if (slot && slot->thread.joinable())
                {
                    slot->thread.join();
                }
            }
        }

    public:
        explicit ThreadPool(std::uint32_t pool_size = std::thread::hardware_concurrency())
            : ThreadPool(static_cast<std::size_t>(pool_size), SubmissionPolicy::RoundRobin)
        {
        }

        ThreadPool(std::uint32_t pool_size, SubmissionPolicy policy)
            : ThreadPool(static_cast<std::size_t>(pool_size), policy)
        {
        }

        explicit ThreadPool(std::size_t pool_size, SubmissionPolicy policy = SubmissionPolicy::RoundRobin)
        {
            m_SubmissionPolicy.store(policy, std::memory_order_release);
            Resize(NormalizeSize(pool_size));
        }

        ~ThreadPool()
        {
            Shutdown();
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        template <class Func, class... Args>
        auto Submit(Func&& func, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
        {
            using Result = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;
            auto state = MakeInvocationState(std::forward<Func>(func), std::forward<Args>(args)...);

            auto task = std::make_shared<std::packaged_task<Result()>>(
                [state]() mutable -> Result
                {
                    return std::apply(
                        [&state](auto&&... unpacked_arguments) -> Result
                        {
                            return std::invoke(
                                std::move(state->callable),
                                std::forward<decltype(unpacked_arguments)>(unpacked_arguments)...);
                        },
                        std::move(state->arguments));
                });

            std::future<Result> result = task->get_future();

            std::lock_guard submission_lock(m_SubmissionMutex);
            EnsureAcceptingSubmissionsOrThrow();

            EnqueueTaskUnlocked([task]
                {
                    (*task)();
                });

            return result;
        }

        template <class Func, class... Args>
        auto SubmitToWorker(std::size_t worker_index, Func&& func, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
        {
            using Result = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;
            auto state = MakeInvocationState(std::forward<Func>(func), std::forward<Args>(args)...);

            auto task = std::make_shared<std::packaged_task<Result()>>(
                [state]() mutable -> Result
                {
                    return std::apply(
                        [&state](auto&&... unpacked_arguments) -> Result
                        {
                            return std::invoke(
                                std::move(state->callable),
                                std::forward<decltype(unpacked_arguments)>(unpacked_arguments)...);
                        },
                        std::move(state->arguments));
                });

            std::future<Result> result = task->get_future();
            std::lock_guard submission_lock(m_SubmissionMutex);
            EnsureAcceptingSubmissionsOrThrow();

            EnqueueTaskToWorkerUnlocked(
                worker_index,
                [task]
                {
                    (*task)();
                });

            return result;
        }

        template <class Func, class... Args>
        void SubmitDetached(Func&& func, Args&&... args)
        {
            auto state = MakeInvocationState(std::forward<Func>(func), std::forward<Args>(args)...);
            std::lock_guard submission_lock(m_SubmissionMutex);
            EnsureAcceptingSubmissionsOrThrow();

            EnqueueTaskUnlocked(
                [state]() mutable
                {
                    std::apply(
                        [&state](auto&&... unpacked_arguments)
                        {
                            std::invoke(
                                std::move(state->callable),
                                std::forward<decltype(unpacked_arguments)>(unpacked_arguments)...);
                        },
                        std::move(state->arguments));
                });
        }

        template <class Func, class... Args>
        void SubmitDetachedToWorker(std::size_t worker_index, Func&& func, Args&&... args)
        {
            auto state = MakeInvocationState(std::forward<Func>(func), std::forward<Args>(args)...);
            std::lock_guard submission_lock(m_SubmissionMutex);
            EnsureAcceptingSubmissionsOrThrow();

            EnqueueTaskToWorkerUnlocked(
                worker_index,
                [state]() mutable
                {
                    std::apply(
                        [&state](auto&&... unpacked_arguments)
                        {
                            std::invoke(
                                std::move(state->callable),
                                std::forward<decltype(unpacked_arguments)>(unpacked_arguments)...);
                        },
                        std::move(state->arguments));
                });
        }

        void SetSubmissionPolicy(SubmissionPolicy policy) noexcept
        {
            m_SubmissionPolicy.store(policy, std::memory_order_release);
        }

        SubmissionPolicy GetSubmissionPolicy() const noexcept
        {
            return m_SubmissionPolicy.load(std::memory_order_acquire);
        }

        bool SetWorkerTopologyHint(std::size_t worker_index, WorkerTopologyHint hint)
        {
            std::shared_lock workers_lock(m_WorkersMutex);

            if (worker_index >= m_Workers.size())
            {
                return false;
            }

            std::lock_guard worker_lock(m_Workers[worker_index]->state->mutex);
            m_Workers[worker_index]->state->topology_hint = std::move(hint);
            m_Workers[worker_index]->state->topology_generation.fetch_add(1, std::memory_order_release);
            return true;
        }

        std::size_t Size() const
        {
            std::shared_lock workers_lock(m_WorkersMutex);
            return m_Workers.size();
        }

        std::size_t PendingTasks() const noexcept
        {
            return m_PendingTasks.load(std::memory_order_acquire);
        }

        std::size_t ActiveWorkers() const noexcept
        {
            return m_ActiveWorkers.load(std::memory_order_acquire);
        }

        bool IsStopping() const noexcept
        {
            return m_StopRequested.load(std::memory_order_acquire)
                || m_ImmediateStopRequested.load(std::memory_order_acquire);
        }

        bool IsAcceptingSubmissions() const noexcept
        {
            return m_AcceptSubmissions.load(std::memory_order_acquire);
        }

        bool WaitIdle(std::chrono::milliseconds timeout)
        {
            std::unique_lock wait_lock(m_WaitMutex);
            return m_IdleCondition.wait_for(wait_lock, timeout, [this]
                {
                    return m_PendingTasks.load(std::memory_order_acquire) == 0
                        && m_ActiveWorkers.load(std::memory_order_acquire) == 0;
                });
        }

        void WaitIdle()
        {
            std::unique_lock wait_lock(m_WaitMutex);
            m_IdleCondition.wait(wait_lock, [this]
                {
                    return m_PendingTasks.load(std::memory_order_acquire) == 0
                        && m_ActiveWorkers.load(std::memory_order_acquire) == 0;
                });
        }

        void Resize(std::size_t new_size)
        {
            std::lock_guard control_lock(m_ControlMutex);

            if (IsStopping())
            {
                throw std::runtime_error("Cannot resize stopping ThreadPool");
            }

            new_size = NormalizeSize(new_size);
            std::vector<std::unique_ptr<WorkerSlot>> retiring_slots;

            {
                std::unique_lock workers_lock(m_WorkersMutex);

                const std::size_t current_size = m_Workers.size();

                if (new_size > current_size)
                {
                    m_Workers.reserve(new_size);

                    for (std::size_t i = current_size; i < new_size; ++i)
                    {
                        m_Workers.emplace_back(CreateWorkerSlot());
                    }
                }
                else if (new_size < current_size)
                {
                    {
                        std::lock_guard global_lock(m_GlobalQueueMutex);

                        for (std::size_t i = new_size; i < current_size; ++i)
                        {
                            WorkerState& state = *m_Workers[i]->state;
                            {
                                std::lock_guard wait_lock(m_WaitMutex);
                                state.retire_requested.store(true, std::memory_order_release);
                            }
                            MoveLocalQueueToGlobal(state, m_GlobalQueue);
                        }
                    }

                    retiring_slots.reserve(current_size - new_size);

                    while (m_Workers.size() > new_size)
                    {
                        retiring_slots.emplace_back(std::move(m_Workers.back()));
                        m_Workers.pop_back();
                    }
                }
            }

            if (!retiring_slots.empty())
            {
                m_WorkAvailableCondition.notify_all();
                JoinAll(std::move(retiring_slots));
                m_IdleCondition.notify_all();
            }
        }

        void Shutdown()
        {
            std::vector<std::unique_ptr<WorkerSlot>> slots;

            {
                std::lock_guard control_lock(m_ControlMutex);

                if (m_ShutdownCompleted.load(std::memory_order_acquire))
                {
                    return;
                }

                {
                    std::lock_guard submission_lock(m_SubmissionMutex);
                    m_AcceptSubmissions.store(false, std::memory_order_release);
                }

                {
                    std::lock_guard wait_lock(m_WaitMutex);
                    m_StopRequested.store(true, std::memory_order_release);
                }
                m_WorkAvailableCondition.notify_all();

                {
                    std::unique_lock workers_lock(m_WorkersMutex);
                    slots.swap(m_Workers);
                }
            }

            JoinAll(std::move(slots));
            m_ShutdownCompleted.store(true, std::memory_order_release);
            m_IdleCondition.notify_all();
        }

        void ShutdownNow()
        {
            std::vector<std::unique_ptr<WorkerSlot>> slots;
            std::size_t removed_tasks = 0;

            {
                std::lock_guard control_lock(m_ControlMutex);

                if (m_ShutdownCompleted.load(std::memory_order_acquire))
                {
                    return;
                }

                {
                    std::lock_guard submission_lock(m_SubmissionMutex);
                    m_AcceptSubmissions.store(false, std::memory_order_release);
                }

                {
                    std::lock_guard wait_lock(m_WaitMutex);
                    m_StopRequested.store(true, std::memory_order_release);
                    m_ImmediateStopRequested.store(true, std::memory_order_release);
                }

                {
                    std::lock_guard global_lock(m_GlobalQueueMutex);
                    removed_tasks += m_GlobalQueue.size();
                    m_GlobalQueue.clear();
                }

                {
                    std::unique_lock workers_lock(m_WorkersMutex);

                    for (const std::unique_ptr<WorkerSlot>& slot : m_Workers)
                    {
                        std::lock_guard worker_lock(slot->state->mutex);
                        removed_tasks += slot->state->local_queue.size();
                        slot->state->local_queue.clear();
                        slot->state->retire_requested.store(true, std::memory_order_release);
                    }

                    slots.swap(m_Workers);
                }

                {
                    std::lock_guard wait_lock(m_WaitMutex);
                    const std::size_t pending_before = m_PendingTasks.load(std::memory_order_acquire);
                    const std::size_t pending_after = pending_before > removed_tasks ? (pending_before - removed_tasks) : 0;
                    m_PendingTasks.store(pending_after, std::memory_order_release);
                }
                m_WorkAvailableCondition.notify_all();
            }

            JoinAll(std::move(slots));
            m_ShutdownCompleted.store(true, std::memory_order_release);
            m_IdleCondition.notify_all();
        }
    };

    inline thread_local ThreadPool* ThreadPool::s_CurrentPool = nullptr;
    inline thread_local ThreadPool::WorkerState* ThreadPool::s_CurrentWorkerState = nullptr;

    namespace Detail
    {
        inline std::mutex& GlobalThreadPoolMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        inline std::unique_ptr<ThreadPool>& GlobalThreadPoolStorage()
        {
            static std::unique_ptr<ThreadPool> storage;
            return storage;
        }

        inline std::size_t& GlobalThreadPoolConfiguredSize()
        {
            static std::size_t configured_size = 0;
            return configured_size;
        }

        inline ThreadPool::SubmissionPolicy& GlobalThreadPoolConfiguredPolicy()
        {
            static ThreadPool::SubmissionPolicy configured_policy = ThreadPool::SubmissionPolicy::RoundRobin;
            return configured_policy;
        }
    }

    inline void InitializeGlobalThreadPool(
        std::size_t pool_size = std::thread::hardware_concurrency(),
        ThreadPool::SubmissionPolicy policy = ThreadPool::SubmissionPolicy::RoundRobin)
    {
        std::lock_guard lock(Detail::GlobalThreadPoolMutex());
        const std::size_t normalized_size = pool_size == 0 ? 1 : pool_size;
        std::unique_ptr<ThreadPool>& storage = Detail::GlobalThreadPoolStorage();

        if (storage)
        {
            const bool same_size = Detail::GlobalThreadPoolConfiguredSize() == normalized_size;
            const bool same_policy = Detail::GlobalThreadPoolConfiguredPolicy() == policy;

            if (!same_size || !same_policy)
            {
                throw std::runtime_error("GlobalThreadPool already initialized with different configuration");
            }

            return;
        }

        storage = std::make_unique<ThreadPool>(normalized_size, policy);
        Detail::GlobalThreadPoolConfiguredSize() = normalized_size;
        Detail::GlobalThreadPoolConfiguredPolicy() = policy;
    }

    inline ThreadPool& GlobalThreadPool()
    {
        InitializeGlobalThreadPool();
        return *Detail::GlobalThreadPoolStorage();
    }
}

#endif // SRC_UTILITY_THREAD_POOL_HPP
