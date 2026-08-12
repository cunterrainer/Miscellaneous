#ifndef TEST_H
#define TEST_H

/*
    Test.h - small, self-contained C++17 test harness
    =================================================

    IMPORTANT MAINTENANCE NOTE
    --------------------------
    This comment is the public documentation for Test.h. If any public macro,
    function, command-line option, exit code, or observable behavior changes,
    this documentation MUST be updated in the same change.

    QUICK START
    -----------
        #include "Test.h"

        TEST(addition_works)
        {
            CHECK_EQUAL(2 + 2, 4);
        }

        TEST_CASE("exceptions carry errors")
        {
            REQUIRE_THROWS_AS(ThrowAnError(), std::runtime_error);
        }

        int main(int argc, char** argv)
        {
            return Test::RunAllTests(argc, argv);
        }

    TEST REGISTRATION
    -----------------
    TEST(name)
        Defines and registers a test using a C++ identifier. The identifier is
        also used as the displayed test name.

    TEST_CASE("name")
        Defines and registers a test with an arbitrary string as its displayed
        name. This is useful when a readable name containing spaces is wanted.

    BASIC ASSERTIONS
    ----------------
    CHECK(condition)
        Verifies a condition. A failure is recorded, but execution continues
        in the current test. Use this when later checks remain safe and useful.

    REQUIRE(condition)
        Verifies a condition. A failure records a diagnostic and immediately
        stops the current test. Other registered tests still run.

    CHECK_MSG(condition, message)
    REQUIRE_MSG(condition, message)
        Behave like CHECK and REQUIRE and append a custom diagnostic message.
        The message may be any value that can be written to std::ostream.

    COMPARISON ASSERTIONS
    ---------------------
    Every comparison evaluates its left and right operands exactly once. When
    possible, a failure prints both resulting values. CHECK variants continue
    the current test; REQUIRE variants stop it.

    CHECK_EQUAL(left, right) / REQUIRE_EQUAL(left, right)
        Require left == right.

    CHECK_NOT_EQUAL(left, right) / REQUIRE_NOT_EQUAL(left, right)
        Require left != right.

    CHECK_LESS_THAN(left, right) / REQUIRE_LESS_THAN(left, right)
        Require left < right.

    CHECK_LESS_THAN_OR_EQUAL(left, right)
    REQUIRE_LESS_THAN_OR_EQUAL(left, right)
        Require left <= right.

    CHECK_GREATER_THAN(left, right) / REQUIRE_GREATER_THAN(left, right)
        Require left > right.

    CHECK_GREATER_THAN_OR_EQUAL(left, right)
    REQUIRE_GREATER_THAN_OR_EQUAL(left, right)
        Require left >= right.

    Each comparison also has a custom-message form with the same comparison
    behavior and an additional diagnostic message:
        CHECK_EQUAL_MSG / REQUIRE_EQUAL_MSG
        CHECK_NOT_EQUAL_MSG / REQUIRE_NOT_EQUAL_MSG
        CHECK_LESS_THAN_MSG / REQUIRE_LESS_THAN_MSG
        CHECK_LESS_THAN_OR_EQUAL_MSG / REQUIRE_LESS_THAN_OR_EQUAL_MSG
        CHECK_GREATER_THAN_MSG / REQUIRE_GREATER_THAN_MSG
        CHECK_GREATER_THAN_OR_EQUAL_MSG / REQUIRE_GREATER_THAN_OR_EQUAL_MSG
    These take (left, right, message). Use them when the expressions alone do
    not identify the input, iteration, or operation that failed.

    EXCEPTION ASSERTIONS
    --------------------
    CHECK_THROWS(expression) / REQUIRE_THROWS(expression)
        Require the expression to throw any exception.

    CHECK_THROWS_AS(expression, exception_type)
    REQUIRE_THROWS_AS(expression, exception_type)
        Require the expression to throw exception_type or a type derived from
        it. A different exception type is reported as a failure.

    CHECK_NOTHROW(expression) / REQUIRE_NOTHROW(expression)
        Require the expression to finish without throwing.

    CHECK_THROWS_MSG(expression, message)
    REQUIRE_THROWS_MSG(expression, message)
        Custom-message forms of CHECK_THROWS and REQUIRE_THROWS.

    CHECK_THROWS_AS_MSG(expression, exception_type, message)
    REQUIRE_THROWS_AS_MSG(expression, exception_type, message)
        Custom-message forms of CHECK_THROWS_AS and REQUIRE_THROWS_AS.

    CHECK_NOTHROW_MSG(expression, message)
    REQUIRE_NOTHROW_MSG(expression, message)
        Custom-message forms of CHECK_NOTHROW and REQUIRE_NOTHROW.

    EXPLICIT FAILURES
    -----------------
    FAIL_CHECK(message)
        Records a failure with the supplied message and continues the test.

    FAIL(message)
        Records a failure with the supplied message and stops the current test.

    RUNNER
    ------
    int Test::RunAllTests(int argc, char** argv)
        Runs registered tests and prints one result line per test followed by a
        summary. An exception escaping a test is reported as a test failure.

        Supported command-line forms:
            executable
                Run every registered test.
            executable --list-tests
                Print registered test names without running them.
            executable --filter substring
                Run tests whose names contain substring (case-sensitive).

        Exit codes:
            0 - all selected tests passed, or --list-tests succeeded.
            1 - a selected test failed, or a filter selected no tests.
            2 - command-line arguments were invalid.

    NOTES
    -----
    - Test.h uses only the C++17 standard library and is header-only.
    - Registration order within one translation unit is definition order.
    - Each test executable should call RunAllTests from exactly one main.
    - Failure recording is synchronized, so CHECK assertions may be used by
      worker threads belonging to the current test. REQUIRE throws on the
      calling thread; use it there only when that exception is propagated or
      caught, and prefer CHECK in detached worker threads.
    - A test must finish or join its worker threads before returning. Assertions
      made after the owning test has ended cannot be attributed to that test.
*/

#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Test
{
    namespace Detail
    {
        struct Failure
        {
            std::string file;
            int line;
            std::string assertion;
            std::string details;
            std::string message;
        };

        struct TestCase
        {
            std::string name;
            void (*function)();
        };

        struct AbortTest final
        {
        };

        inline std::vector<TestCase>& Registry()
        {
            static std::vector<TestCase> tests;
            return tests;
        }

        inline std::vector<Failure>*& CurrentFailures()
        {
            static std::vector<Failure>* failures = nullptr;
            return failures;
        }

        inline std::mutex& FailureMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        inline void SetCurrentFailures(std::vector<Failure>* failures)
        {
            const std::lock_guard<std::mutex> lock(FailureMutex());
            CurrentFailures() = failures;
        }

        template <typename T, typename = void>
        struct IsStreamWritable : std::false_type
        {
        };

        template <typename T>
        struct IsStreamWritable<T, std::void_t<decltype(
            std::declval<std::ostream&>() << std::declval<const T&>())>>
            : std::true_type
        {
        };

        inline std::string ValueToString(const char* value)
        {
            return value == nullptr ? "nullptr" : std::string{"\""} + value + "\"";
        }

        inline std::string ValueToString(char* value)
        {
            return ValueToString(static_cast<const char*>(value));
        }

        inline std::string ValueToString(const std::string& value)
        {
            return std::string{"\""} + value + "\"";
        }

        template <typename T>
        inline std::string ValueToString(const T& value)
        {
            if constexpr (IsStreamWritable<T>::value)
            {
                std::ostringstream stream;
                stream << std::boolalpha << value;
                return stream.str();
            }
            else
            {
                return "<unprintable>";
            }
        }

        template <typename T>
        inline std::string MessageToString(T&& message)
        {
            using Message = std::remove_reference_t<T>;
            if constexpr (IsStreamWritable<Message>::value)
            {
                std::ostringstream stream;
                stream << std::boolalpha << std::forward<T>(message);
                return stream.str();
            }
            else
            {
                return "<unprintable>";
            }
        }

        inline std::string MessageToString()
        {
            return {};
        }

        [[noreturn]] inline void Abort()
        {
            throw AbortTest{};
        }

        inline void RecordFailure(
            const char* file,
            int line,
            std::string assertion,
            std::string details,
            std::string message,
            bool fatal)
        {
            {
                const std::lock_guard<std::mutex> lock(FailureMutex());
                if (CurrentFailures() != nullptr)
                {
                    CurrentFailures()->push_back(
                        {file, line, std::move(assertion), std::move(details), std::move(message)});
                }
            }

            if (fatal)
            {
                Abort();
            }
        }

        inline void CheckBoolean(
            bool result,
            const char* expression,
            const char* file,
            int line,
            bool fatal,
            std::string message = {})
        {
            if (!result)
            {
                RecordFailure(file, line, expression, "condition is false", std::move(message), fatal);
            }
        }

        struct Equal
        {
            template <typename Left, typename Right>
            bool operator()(const Left& left, const Right& right) const
            {
                return left == right;
            }
        };

        struct NotEqual
        {
            template <typename Left, typename Right>
            bool operator()(const Left& left, const Right& right) const
            {
                return left != right;
            }
        };

        struct LessThan
        {
            template <typename Left, typename Right>
            bool operator()(const Left& left, const Right& right) const
            {
                return left < right;
            }
        };

        struct LessThanOrEqual
        {
            template <typename Left, typename Right>
            bool operator()(const Left& left, const Right& right) const
            {
                return left <= right;
            }
        };

        struct GreaterThan
        {
            template <typename Left, typename Right>
            bool operator()(const Left& left, const Right& right) const
            {
                return left > right;
            }
        };

        struct GreaterThanOrEqual
        {
            template <typename Left, typename Right>
            bool operator()(const Left& left, const Right& right) const
            {
                return left >= right;
            }
        };

        template <typename Left, typename Right, typename Comparator>
        inline void CheckComparison(
            const Left& left,
            const Right& right,
            const char* leftExpression,
            const char* rightExpression,
            const char* operation,
            const char* file,
            int line,
            bool fatal,
            std::string message,
            Comparator comparator)
        {
            if (!comparator(left, right))
            {
                std::ostringstream details;
                details << "expected " << leftExpression << ' ' << operation << ' '
                        << rightExpression << "\n      left: " << ValueToString(left)
                        << "\n     right: " << ValueToString(right);
                RecordFailure(
                    file,
                    line,
                    std::string{leftExpression} + " " + operation + " " + rightExpression,
                    details.str(),
                    std::move(message),
                    fatal);
            }
        }

        inline std::string DescribeCurrentException()
        {
            try
            {
                throw;
            }
            catch (const std::exception& exception)
            {
                return std::string{"std::exception: "} + exception.what();
            }
            catch (...)
            {
                return "unknown exception";
            }
        }

        inline void ReportFailures(const std::vector<Failure>& failures)
        {
            for (const Failure& failure : failures)
            {
                std::cerr << "  " << failure.file << ':' << failure.line << ": "
                          << failure.assertion << '\n';
                if (!failure.details.empty())
                {
                    std::cerr << "    " << failure.details << '\n';
                }
                if (!failure.message.empty())
                {
                    std::cerr << "    message: " << failure.message << '\n';
                }
            }
        }

        class Registrar
        {
        public:
            Registrar(const char* name, void (*function)())
            {
                Registry().push_back({name, function});
            }
        };
    }

    inline int RunAllTests(int argc, char** argv)
    {
        bool listTests = false;
        std::string filter;

        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index];
            if (argument == "--list-tests" && !listTests && filter.empty())
            {
                listTests = true;
            }
            else if (argument == "--filter" && !listTests && filter.empty() && index + 1 < argc)
            {
                filter = argv[++index];
                if (filter.empty())
                {
                    std::cerr << "error: --filter requires a non-empty substring\n";
                    return 2;
                }
            }
            else
            {
                std::cerr << "error: invalid arguments\n"
                          << "usage: " << argv[0] << " [--list-tests | --filter substring]\n";
                return 2;
            }
        }

        if (listTests)
        {
            for (const Detail::TestCase& test : Detail::Registry())
            {
                std::cout << test.name << '\n';
            }
            return 0;
        }

        std::size_t selected = 0;
        std::size_t passed = 0;
        std::size_t failed = 0;

        for (const Detail::TestCase& test : Detail::Registry())
        {
            if (!filter.empty() && test.name.find(filter) == std::string::npos)
            {
                continue;
            }

            ++selected;
            std::vector<Detail::Failure> failures;
            Detail::SetCurrentFailures(&failures);

            try
            {
                test.function();
            }
            catch (const Detail::AbortTest&)
            {
            }
            catch (const std::exception& exception)
            {
                Detail::RecordFailure(
                    "<test>", 0, "uncaught exception", exception.what(), {}, false);
            }
            catch (...)
            {
                Detail::RecordFailure(
                    "<test>", 0, "uncaught exception", "unknown exception", {}, false);
            }

            Detail::SetCurrentFailures(nullptr);
            if (failures.empty())
            {
                ++passed;
                std::cout << "[PASS] " << test.name << '\n';
            }
            else
            {
                ++failed;
                std::cerr << "[FAIL] " << test.name << " (" << failures.size()
                          << " failure" << (failures.size() == 1 ? "" : "s") << ")\n";
                Detail::ReportFailures(failures);
            }
        }

        if (selected == 0)
        {
            std::cerr << "No tests matched filter: " << filter << '\n';
            return 1;
        }

        std::ostream& summary = failed == 0 ? std::cout : std::cerr;
        summary << "Tests: " << selected << ", passed: " << passed
                << ", failed: " << failed << '\n';
        return failed == 0 ? 0 : 1;
    }
}

#define TEST_DETAIL_JOIN_INNER(left, right) left##right
#define TEST_DETAIL_JOIN(left, right) TEST_DETAIL_JOIN_INNER(left, right)

#define TEST(name) \
    static void name(); \
    static ::Test::Detail::Registrar TEST_DETAIL_JOIN(name, _registrar)(#name, &name); \
    static void name()

#define TEST_DETAIL_CASE(name, identifier) \
    static void TEST_DETAIL_JOIN(test_case_function_, identifier)(); \
    static ::Test::Detail::Registrar TEST_DETAIL_JOIN(test_case_registrar_, identifier)( \
        name, &TEST_DETAIL_JOIN(test_case_function_, identifier)); \
    static void TEST_DETAIL_JOIN(test_case_function_, identifier)()

#define TEST_CASE(name) TEST_DETAIL_CASE(name, __LINE__)

#define TEST_DETAIL_BOOLEAN(condition, fatal, message) \
    do \
    { \
        ::Test::Detail::CheckBoolean( \
            static_cast<bool>(condition), #condition, __FILE__, __LINE__, fatal, message); \
    } while (false)

#define CHECK(condition) TEST_DETAIL_BOOLEAN(condition, false, ::std::string{})
#define REQUIRE(condition) TEST_DETAIL_BOOLEAN(condition, true, ::std::string{})
#define CHECK_MSG(condition, message) \
    TEST_DETAIL_BOOLEAN(condition, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_MSG(condition, message) \
    TEST_DETAIL_BOOLEAN(condition, true, ::Test::Detail::MessageToString(message))

#define TEST_DETAIL_COMPARISON(left, right, operation, comparator, fatal, message) \
    do \
    { \
        auto&& test_detail_left = (left); \
        auto&& test_detail_right = (right); \
        ::Test::Detail::CheckComparison( \
            test_detail_left, test_detail_right, #left, #right, operation, __FILE__, __LINE__, \
            fatal, message, ::Test::Detail::comparator{}); \
    } while (false)

#define CHECK_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "==", Equal, false, ::std::string{})
#define REQUIRE_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "==", Equal, true, ::std::string{})
#define CHECK_NOT_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "!=", NotEqual, false, ::std::string{})
#define REQUIRE_NOT_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "!=", NotEqual, true, ::std::string{})
#define CHECK_LESS_THAN(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "<", LessThan, false, ::std::string{})
#define REQUIRE_LESS_THAN(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "<", LessThan, true, ::std::string{})
#define CHECK_LESS_THAN_OR_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "<=", LessThanOrEqual, false, ::std::string{})
#define REQUIRE_LESS_THAN_OR_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, "<=", LessThanOrEqual, true, ::std::string{})
#define CHECK_GREATER_THAN(left, right) \
    TEST_DETAIL_COMPARISON(left, right, ">", GreaterThan, false, ::std::string{})
#define REQUIRE_GREATER_THAN(left, right) \
    TEST_DETAIL_COMPARISON(left, right, ">", GreaterThan, true, ::std::string{})
#define CHECK_GREATER_THAN_OR_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, ">=", GreaterThanOrEqual, false, ::std::string{})
#define REQUIRE_GREATER_THAN_OR_EQUAL(left, right) \
    TEST_DETAIL_COMPARISON(left, right, ">=", GreaterThanOrEqual, true, ::std::string{})

#define CHECK_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "==", Equal, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "==", Equal, true, ::Test::Detail::MessageToString(message))
#define CHECK_NOT_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "!=", NotEqual, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_NOT_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "!=", NotEqual, true, ::Test::Detail::MessageToString(message))
#define CHECK_LESS_THAN_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "<", LessThan, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_LESS_THAN_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "<", LessThan, true, ::Test::Detail::MessageToString(message))
#define CHECK_LESS_THAN_OR_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "<=", LessThanOrEqual, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_LESS_THAN_OR_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, "<=", LessThanOrEqual, true, ::Test::Detail::MessageToString(message))
#define CHECK_GREATER_THAN_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, ">", GreaterThan, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_GREATER_THAN_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, ">", GreaterThan, true, ::Test::Detail::MessageToString(message))
#define CHECK_GREATER_THAN_OR_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, ">=", GreaterThanOrEqual, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_GREATER_THAN_OR_EQUAL_MSG(left, right, message) \
    TEST_DETAIL_COMPARISON(left, right, ">=", GreaterThanOrEqual, true, ::Test::Detail::MessageToString(message))

#define TEST_DETAIL_THROWS(expression, fatal, message) \
    do \
    { \
        bool test_detail_threw = false; \
        try \
        { \
            static_cast<void>(expression); \
        } \
        catch (...) \
        { \
            test_detail_threw = true; \
        } \
        if (!test_detail_threw) \
        { \
            ::Test::Detail::RecordFailure( \
                __FILE__, __LINE__, #expression, "expected an exception", message, fatal); \
        } \
    } while (false)

#define TEST_DETAIL_THROWS_AS(expression, exception_type, fatal, message) \
    do \
    { \
        bool test_detail_expected_exception = false; \
        bool test_detail_other_exception = false; \
        ::std::string test_detail_exception_description; \
        try \
        { \
            static_cast<void>(expression); \
        } \
        catch (const exception_type&) \
        { \
            test_detail_expected_exception = true; \
        } \
        catch (...) \
        { \
            test_detail_other_exception = true; \
            test_detail_exception_description = ::Test::Detail::DescribeCurrentException(); \
        } \
        if (!test_detail_expected_exception) \
        { \
            ::Test::Detail::RecordFailure( \
                __FILE__, __LINE__, #expression, \
                test_detail_other_exception \
                    ? ::std::string{"expected "} + #exception_type + ", caught " + test_detail_exception_description \
                    : ::std::string{"expected exception type "} + #exception_type, \
                message, fatal); \
        } \
    } while (false)

#define TEST_DETAIL_NOTHROW(expression, fatal, message) \
    do \
    { \
        bool test_detail_threw = false; \
        ::std::string test_detail_exception_description; \
        try \
        { \
            static_cast<void>(expression); \
        } \
        catch (...) \
        { \
            test_detail_threw = true; \
            test_detail_exception_description = ::Test::Detail::DescribeCurrentException(); \
        } \
        if (test_detail_threw) \
        { \
            ::Test::Detail::RecordFailure( \
                __FILE__, __LINE__, #expression, \
                ::std::string{"expected no exception, caught "} + test_detail_exception_description, \
                message, fatal); \
        } \
    } while (false)

#define CHECK_THROWS(expression) TEST_DETAIL_THROWS(expression, false, ::std::string{})
#define REQUIRE_THROWS(expression) TEST_DETAIL_THROWS(expression, true, ::std::string{})
#define CHECK_THROWS_MSG(expression, message) \
    TEST_DETAIL_THROWS(expression, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_THROWS_MSG(expression, message) \
    TEST_DETAIL_THROWS(expression, true, ::Test::Detail::MessageToString(message))
#define CHECK_THROWS_AS(expression, exception_type) \
    TEST_DETAIL_THROWS_AS(expression, exception_type, false, ::std::string{})
#define REQUIRE_THROWS_AS(expression, exception_type) \
    TEST_DETAIL_THROWS_AS(expression, exception_type, true, ::std::string{})
#define CHECK_THROWS_AS_MSG(expression, exception_type, message) \
    TEST_DETAIL_THROWS_AS(expression, exception_type, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_THROWS_AS_MSG(expression, exception_type, message) \
    TEST_DETAIL_THROWS_AS(expression, exception_type, true, ::Test::Detail::MessageToString(message))
#define CHECK_NOTHROW(expression) TEST_DETAIL_NOTHROW(expression, false, ::std::string{})
#define REQUIRE_NOTHROW(expression) TEST_DETAIL_NOTHROW(expression, true, ::std::string{})
#define CHECK_NOTHROW_MSG(expression, message) \
    TEST_DETAIL_NOTHROW(expression, false, ::Test::Detail::MessageToString(message))
#define REQUIRE_NOTHROW_MSG(expression, message) \
    TEST_DETAIL_NOTHROW(expression, true, ::Test::Detail::MessageToString(message))

#define FAIL_CHECK(message) \
    ::Test::Detail::RecordFailure( \
        __FILE__, __LINE__, "explicit failure", {}, ::Test::Detail::MessageToString(message), false)
#define FAIL(message) \
    ::Test::Detail::RecordFailure( \
        __FILE__, __LINE__, "explicit failure", {}, ::Test::Detail::MessageToString(message), true)

#endif
