#include "Test.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    enum class RunnerProbeMode
    {
        Pass,
        OneFailure,
        TwoFailures,
        FatalFailure,
        StandardException,
        UnknownException
    };

    RunnerProbeMode runner_probe_mode = RunnerProbeMode::Pass;

    struct Unprintable
    {
    };

    class ScopedFailureCapture
    {
    public:
        explicit ScopedFailureCapture(std::vector<Test::Detail::Failure>& failures)
            : previous_(Test::Detail::CurrentFailures())
        {
            Test::Detail::SetCurrentFailures(&failures);
        }

        ~ScopedFailureCapture()
        {
            Test::Detail::SetCurrentFailures(previous_);
        }

        ScopedFailureCapture(const ScopedFailureCapture&) = delete;
        ScopedFailureCapture& operator=(const ScopedFailureCapture&) = delete;

    private:
        std::vector<Test::Detail::Failure>* previous_;
    };

    class ScopedOutputCapture
    {
    public:
        ScopedOutputCapture()
            : previous_output_(std::cout.rdbuf(output_.rdbuf())),
              previous_error_(std::cerr.rdbuf(error_.rdbuf()))
        {
        }

        ~ScopedOutputCapture()
        {
            std::cout.rdbuf(previous_output_);
            std::cerr.rdbuf(previous_error_);
        }

        ScopedOutputCapture(const ScopedOutputCapture&) = delete;
        ScopedOutputCapture& operator=(const ScopedOutputCapture&) = delete;

        std::string Output() const
        {
            return output_.str();
        }

        std::string Error() const
        {
            return error_.str();
        }

    private:
        std::ostringstream output_;
        std::ostringstream error_;
        std::streambuf* previous_output_;
        std::streambuf* previous_error_;
    };

    struct NestedRunResult
    {
        int exit_code;
        std::string output;
        std::string error;
    };

    NestedRunResult RunNested(
        std::vector<std::string> arguments,
        RunnerProbeMode mode = RunnerProbeMode::Pass)
    {
        std::vector<char*> argument_pointers;
        argument_pointers.reserve(arguments.size());
        for (std::string& argument : arguments)
        {
            argument_pointers.push_back(argument.data());
        }

        std::vector<Test::Detail::Failure>* previous_failures =
            Test::Detail::CurrentFailures();
        const RunnerProbeMode previous_mode = runner_probe_mode;
        runner_probe_mode = mode;

        ScopedOutputCapture output;
        const int exit_code = Test::RunAllTests(
            static_cast<int>(argument_pointers.size()), argument_pointers.data());
        Test::Detail::SetCurrentFailures(previous_failures);
        runner_probe_mode = previous_mode;
        return {exit_code, output.Output(), output.Error()};
    }

    TEST(runner_probe)
    {
        switch (runner_probe_mode)
        {
            case RunnerProbeMode::Pass:
                return;
            case RunnerProbeMode::OneFailure:
                CHECK(false);
                return;
            case RunnerProbeMode::TwoFailures:
                FAIL_CHECK("first explicit failure");
                FAIL_CHECK("second explicit failure");
                return;
            case RunnerProbeMode::FatalFailure:
                REQUIRE(false);
                return;
            case RunnerProbeMode::StandardException:
                throw std::runtime_error{"uncaught standard exception"};
            case RunnerProbeMode::UnknownException:
                throw 7;
        }
    }

    TEST(public_assertions_succeed)
    {
        CHECK(true);
        REQUIRE(true);
        CHECK_MSG(true, "check context");
        REQUIRE_MSG(true, "require context");

        CHECK_EQUAL(1, 1);
        REQUIRE_EQUAL(1, 1);
        CHECK_EQUAL_MSG(1, 1, "equal check context");
        REQUIRE_EQUAL_MSG(1, 1, "equal require context");
        CHECK_NOT_EQUAL(1, 2);
        REQUIRE_NOT_EQUAL(1, 2);
        CHECK_NOT_EQUAL_MSG(1, 2, "not-equal check context");
        REQUIRE_NOT_EQUAL_MSG(1, 2, "not-equal context");
        CHECK_LESS_THAN(1, 2);
        REQUIRE_LESS_THAN(1, 2);
        CHECK_LESS_THAN_MSG(1, 2, "less-than check context");
        REQUIRE_LESS_THAN_MSG(1, 2, "less-than context");
        CHECK_LESS_THAN_OR_EQUAL(2, 2);
        REQUIRE_LESS_THAN_OR_EQUAL(1, 2);
        CHECK_LESS_THAN_OR_EQUAL_MSG(2, 2, "less-than-or-equal check context");
        REQUIRE_LESS_THAN_OR_EQUAL_MSG(1, 2, "less-than-or-equal context");
        CHECK_GREATER_THAN(2, 1);
        REQUIRE_GREATER_THAN(2, 1);
        CHECK_GREATER_THAN_MSG(2, 1, "greater-than check context");
        REQUIRE_GREATER_THAN_MSG(2, 1, "greater-than context");
        CHECK_GREATER_THAN_OR_EQUAL(2, 2);
        REQUIRE_GREATER_THAN_OR_EQUAL(2, 1);
        CHECK_GREATER_THAN_OR_EQUAL_MSG(2, 2, "greater-than-or-equal check context");
        REQUIRE_GREATER_THAN_OR_EQUAL_MSG(2, 1, "greater-than-or-equal context");

        CHECK_THROWS(throw std::runtime_error{"expected"});
        REQUIRE_THROWS(throw std::runtime_error{"expected"});
        CHECK_THROWS_MSG(throw 1, "throws check context");
        REQUIRE_THROWS_MSG(throw 1, "throws context");
        CHECK_THROWS_AS(throw std::runtime_error{"expected"}, std::runtime_error);
        REQUIRE_THROWS_AS(throw std::runtime_error{"expected"}, std::runtime_error);
        CHECK_THROWS_AS_MSG(
            throw std::runtime_error{"expected"},
            std::exception,
            "throws-as check context");
        REQUIRE_THROWS_AS_MSG(
            throw std::runtime_error{"expected"},
            std::exception,
            "throws-as context");
        CHECK_NOTHROW(static_cast<void>(0));
        REQUIRE_NOTHROW(static_cast<void>(0));
        CHECK_NOTHROW_MSG(static_cast<void>(0), "nothrow check context");
        REQUIRE_NOTHROW_MSG(static_cast<void>(0), "nothrow context");
    }

    TEST_CASE("comparison operands are evaluated exactly once")
    {
        int left_evaluations = 0;
        int right_evaluations = 0;

        CHECK_EQUAL(++left_evaluations, ++right_evaluations);

        REQUIRE(left_evaluations == 1);
        REQUIRE(right_evaluations == 1);
    }

    TEST(nonfatal_failures_accumulate_and_include_context)
    {
        std::vector<Test::Detail::Failure> failures;
        bool continued = false;
        {
            ScopedFailureCapture capture(failures);
            CHECK_MSG(false, "first failure");
            CHECK_EQUAL_MSG(1, 2, "second failure");
            FAIL_CHECK("third failure");
            continued = true;
        }

        REQUIRE(continued);
        REQUIRE(failures.size() == 3u);
        REQUIRE(failures[0].assertion == "false");
        REQUIRE(failures[0].message == "first failure");
        REQUIRE(failures[1].assertion == "1 == 2");
        REQUIRE(failures[1].details.find("left: 1") != std::string::npos);
        REQUIRE(failures[1].details.find("right: 2") != std::string::npos);
        REQUIRE(failures[2].assertion == "explicit failure");
    }

    TEST(fatal_failures_stop_only_the_current_test_body)
    {
        std::vector<Test::Detail::Failure> failures;
        bool caught_abort = false;
        bool continued_after_failure = false;
        {
            ScopedFailureCapture capture(failures);
            try
            {
                REQUIRE_MSG(false, "fatal condition");
                continued_after_failure = true;
            }
            catch (const Test::Detail::AbortTest&)
            {
                caught_abort = true;
            }
        }

        REQUIRE(caught_abort);
        REQUIRE(!continued_after_failure);
        REQUIRE(failures.size() == 1u);
        REQUIRE(failures[0].message == "fatal condition");
    }

    TEST(all_comparison_failures_are_reported)
    {
        std::vector<Test::Detail::Failure> failures;
        {
            ScopedFailureCapture capture(failures);
            CHECK_EQUAL(1, 2);
            CHECK_NOT_EQUAL(1, 1);
            CHECK_LESS_THAN(2, 1);
            CHECK_LESS_THAN_OR_EQUAL(2, 1);
            CHECK_GREATER_THAN(1, 2);
            CHECK_GREATER_THAN_OR_EQUAL(1, 2);
        }

        REQUIRE(failures.size() == 6u);
        REQUIRE(failures[0].assertion == "1 == 2");
        REQUIRE(failures[1].assertion == "1 != 1");
        REQUIRE(failures[2].assertion == "2 < 1");
        REQUIRE(failures[3].assertion == "2 <= 1");
        REQUIRE(failures[4].assertion == "1 > 2");
        REQUIRE(failures[5].assertion == "1 >= 2");
    }

    TEST(exception_failures_describe_the_problem)
    {
        std::vector<Test::Detail::Failure> failures;
        {
            ScopedFailureCapture capture(failures);
            CHECK_THROWS(static_cast<void>(0));
            CHECK_THROWS_AS(throw 1, std::runtime_error);
            CHECK_NOTHROW(throw std::runtime_error{"unexpected"});
        }

        REQUIRE(failures.size() == 3u);
        REQUIRE(failures[0].details.find("expected an exception") != std::string::npos);
        REQUIRE(failures[1].details.find("expected std::runtime_error") != std::string::npos);
        REQUIRE(failures[2].details.find("unexpected") != std::string::npos);
    }

    TEST(fatal_exception_assertions_abort_and_record)
    {
        std::vector<Test::Detail::Failure> failures;
        int aborts = 0;
        {
            ScopedFailureCapture capture(failures);

            try
            {
                REQUIRE_THROWS(static_cast<void>(0));
            }
            catch (const Test::Detail::AbortTest&)
            {
                ++aborts;
            }

            try
            {
                REQUIRE_THROWS_AS(static_cast<void>(0), std::runtime_error);
            }
            catch (const Test::Detail::AbortTest&)
            {
                ++aborts;
            }

            try
            {
                REQUIRE_NOTHROW(throw std::runtime_error{"fatal unexpected exception"});
            }
            catch (const Test::Detail::AbortTest&)
            {
                ++aborts;
            }
        }

        REQUIRE(aborts == 3);
        REQUIRE(failures.size() == 3u);
        REQUIRE(failures[1].details.find("expected exception type") != std::string::npos);
    }

    TEST(value_and_message_rendering_covers_all_fallbacks)
    {
        const char* text = "text";
        const char* null_text = nullptr;
        char mutable_text[] = "mutable";

        REQUIRE(Test::Detail::ValueToString(text) == "\"text\"");
        REQUIRE(Test::Detail::ValueToString(null_text) == "nullptr");
        REQUIRE(Test::Detail::ValueToString(mutable_text) == "\"mutable\"");
        REQUIRE(Test::Detail::ValueToString(std::string{"string"}) == "\"string\"");
        REQUIRE(Test::Detail::ValueToString(42) == "42");
        REQUIRE(Test::Detail::ValueToString(Unprintable{}) == "<unprintable>");
        REQUIRE(Test::Detail::MessageToString(42) == "42");
        REQUIRE(Test::Detail::MessageToString(Unprintable{}) == "<unprintable>");
        REQUIRE(Test::Detail::MessageToString().empty());
    }

    TEST(failure_without_an_active_test_is_ignored)
    {
        std::vector<Test::Detail::Failure>* previous_failures =
            Test::Detail::CurrentFailures();
        Test::Detail::SetCurrentFailures(nullptr);
        Test::Detail::RecordFailure(
            __FILE__, __LINE__, "orphaned failure", {}, {}, false);
        Test::Detail::SetCurrentFailures(previous_failures);
        CHECK(true);
    }

    TEST(explicit_fatal_failure_is_recorded)
    {
        std::vector<Test::Detail::Failure> failures;
        bool caught_abort = false;
        {
            ScopedFailureCapture capture(failures);
            try
            {
                FAIL("explicit fatal failure");
            }
            catch (const Test::Detail::AbortTest&)
            {
                caught_abort = true;
            }
        }

        REQUIRE(caught_abort);
        REQUIRE(failures.size() == 1u);
        REQUIRE(failures[0].assertion == "explicit failure");
        REQUIRE(failures[0].message == "explicit fatal failure");
    }

    TEST(worker_thread_checks_are_synchronized)
    {
        std::vector<Test::Detail::Failure> failures;
        {
            ScopedFailureCapture capture(failures);
            std::thread first([] { CHECK_MSG(false, "first worker"); });
            std::thread second([] { CHECK_MSG(false, "second worker"); });
            first.join();
            second.join();
        }

        REQUIRE(failures.size() == 2u);
        const bool first_then_second =
            failures[0].message == "first worker" && failures[1].message == "second worker";
        const bool second_then_first =
            failures[0].message == "second worker" && failures[1].message == "first worker";
        REQUIRE(first_then_second || second_then_first);
    }

    TEST(runner_covers_success_failure_exception_and_cli_paths)
    {
        const NestedRunResult passing =
            RunNested({"test_harness_test", "--filter", "runner_probe"});
        REQUIRE(passing.exit_code == 0);
        REQUIRE(passing.output.find("[PASS] runner_probe") != std::string::npos);

        const NestedRunResult one_failure = RunNested(
            {"test_harness_test", "--filter", "runner_probe"},
            RunnerProbeMode::OneFailure);
        REQUIRE(one_failure.exit_code == 1);
        REQUIRE(one_failure.error.find("1 failure)") != std::string::npos);
        REQUIRE(one_failure.error.find("condition is false") != std::string::npos);

        const NestedRunResult two_failures = RunNested(
            {"test_harness_test", "--filter", "runner_probe"},
            RunnerProbeMode::TwoFailures);
        REQUIRE(two_failures.exit_code == 1);
        REQUIRE(two_failures.error.find("2 failures)") != std::string::npos);
        REQUIRE(two_failures.error.find("message: first explicit failure") != std::string::npos);

        const NestedRunResult fatal_failure = RunNested(
            {"test_harness_test", "--filter", "runner_probe"},
            RunnerProbeMode::FatalFailure);
        REQUIRE(fatal_failure.exit_code == 1);

        const NestedRunResult standard_exception = RunNested(
            {"test_harness_test", "--filter", "runner_probe"},
            RunnerProbeMode::StandardException);
        REQUIRE(standard_exception.exit_code == 1);
        REQUIRE(standard_exception.error.find("uncaught standard exception") != std::string::npos);

        const NestedRunResult unknown_exception = RunNested(
            {"test_harness_test", "--filter", "runner_probe"},
            RunnerProbeMode::UnknownException);
        REQUIRE(unknown_exception.exit_code == 1);
        REQUIRE(unknown_exception.error.find("unknown exception") != std::string::npos);

        const NestedRunResult listed = RunNested({"test_harness_test", "--list-tests"});
        REQUIRE(listed.exit_code == 0);
        REQUIRE(listed.output.find("runner_probe") != std::string::npos);

        const NestedRunResult unmatched =
            RunNested({"test_harness_test", "--filter", "no-test-has-this-name"});
        REQUIRE(unmatched.exit_code == 1);
        REQUIRE(unmatched.error.find("No tests matched filter") != std::string::npos);

        REQUIRE(RunNested({"test_harness_test", "--unknown"}).exit_code == 2);
        REQUIRE(RunNested({"test_harness_test", "--filter"}).exit_code == 2);
        REQUIRE(RunNested({"test_harness_test", "--filter", ""}).exit_code == 2);
        REQUIRE(RunNested({"test_harness_test", "--list-tests", "--list-tests"}).exit_code == 2);
        REQUIRE(RunNested({"test_harness_test", "--filter", "runner_probe", "--list-tests"}).exit_code == 2);
        REQUIRE(RunNested({"test_harness_test", "--list-tests", "--filter", "runner_probe"}).exit_code == 2);
        REQUIRE(RunNested({"test_harness_test", "--filter", "runner_probe", "--filter", "runner_probe"}).exit_code == 2);
    }
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
