#if defined(_FORTIFY_SOURCE)
#undef _FORTIFY_SOURCE
#endif

#include "../Result.h"
#include "../Test.h"

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#ifndef NDEBUG
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cstdarg>

namespace SnprintfTest
{
    enum class Behavior
    {
        PassThrough,
        FailFirst,
        FailSecond,
        ShortSecond
    };

    Behavior behavior = Behavior::PassThrough;
    int calls = 0;

    void Reset(Behavior next)
    {
        behavior = next;
        calls = 0;
    }

    const char* RuntimeFormat(const char* fallback)
    {
        const char* const override_format = std::getenv("RESULT_TEST_SNPRINTF_FORMAT_OVERRIDE");
        return override_format == nullptr ? fallback : override_format;
    }
}

extern "C" int __wrap_snprintf(char* buffer, std::size_t size, const char* format, ...)
{
    ++SnprintfTest::calls;
    if ((SnprintfTest::behavior == SnprintfTest::Behavior::FailFirst && SnprintfTest::calls == 1) ||
        (SnprintfTest::behavior == SnprintfTest::Behavior::FailSecond && SnprintfTest::calls == 2))
    {
        return -1;
    }

    if (SnprintfTest::behavior == SnprintfTest::Behavior::ShortSecond && SnprintfTest::calls == 2)
    {
        if (size >= 3)
        {
            buffer[0] = 'o';
            buffer[1] = 'k';
            buffer[2] = '\0';
        }
        return 2;
    }

    std::va_list arguments;
    va_start(arguments, format);
    const int result = std::vsnprintf(buffer, size, format, arguments);
    va_end(arguments);
    return result;
}

namespace
{
    enum class ErrorCode
    {
        None,
        Read,
        Write
    };

    struct BareError
    {
        explicit BareError(std::string message) : message(std::move(message)) {}

        std::string message;
    };

    struct CustomError
    {
        using Type = ErrorCode;

        CustomError(ErrorCode code, std::string message)
            : code_value(code), message(std::move(message))
        {
        }

        explicit CustomError(std::string message)
            : code_value(ErrorCode::None), message(std::move(message))
        {
        }

        Type type() const noexcept
        {
            return code_value;
        }

        const std::string& what() const noexcept
        {
            return message;
        }

        ErrorCode code_value;
        std::string message;
    };

    struct TypeOnlyError
    {
        using Type = int;
    };

    struct FunctionOnlyError
    {
        int type() const
        {
            return 1;
        }
    };

    struct WhatOnlyError
    {
        explicit WhatOnlyError(std::string value) : value(std::move(value)) {}

        const std::string& what() const noexcept
        {
            return value;
        }

        std::string value;
    };

    struct NoDefaultValue
    {
        explicit NoDefaultValue(int value) : value(value) {}
        int value;
    };

    struct MoveProbe
    {
        explicit MoveProbe(int value) : value(value) {}

        MoveProbe(const MoveProbe& other) : value(other.value)
        {
            ++copies;
        }

        MoveProbe(MoveProbe&& other) noexcept : value(other.value)
        {
            ++moves;
            other.value = -1;
        }

        MoveProbe& operator=(const MoveProbe& other)
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

        static void Reset()
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

    template <typename R, typename = void>
    struct HasErrType : std::false_type
    {
    };

    template <typename R>
    struct HasErrType<R, std::void_t<decltype(std::declval<const R&>().ErrType())>>
        : std::true_type
    {
    };

    template <typename R, typename = void>
    struct HasExpect : std::false_type
    {
    };

    template <typename R>
    struct HasExpect<R, std::void_t<decltype(std::declval<const R&>().Expect("context"))>>
        : std::true_type
    {
    };

    template <typename R, typename = void>
    struct HasUnwrapOrDefault : std::false_type
    {
    };

    template <typename R>
    struct HasUnwrapOrDefault<R, std::void_t<decltype(std::declval<const R&>().UnwrapOrDefault())>>
        : std::true_type
    {
    };

#ifndef NDEBUG
    template <typename Function>
    bool DiesFromAssertion(Function function)
    {
        const pid_t child = fork();
        if (child == 0)
        {
            static_cast<void>(std::freopen("/dev/null", "w", stderr));
            function();
            std::_Exit(0);
        }
        if (child < 0)
        {
            return false;
        }

        int status = 0;
        if (waitpid(child, &status, 0) != child)
        {
            return false;
        }
        return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
    }
#endif

    TEST(error_plain_typed_and_default_constructors)
    {
        const std::size_t sentinel = std::numeric_limits<std::size_t>::max();
        const Error<> empty;
        CHECK(empty.what().empty());
        CHECK_EQUAL(empty.type(), sentinel);

        const Error<> literal("literal");
        CHECK_EQUAL(literal.what(), std::string("literal"));
        CHECK_EQUAL(literal.type(), sentinel);

        const std::string lvalue_message("lvalue");
        const Error<> lvalue(lvalue_message);
        CHECK_EQUAL(lvalue.what(), lvalue_message);
        CHECK_EQUAL(lvalue_message, std::string("lvalue"));

        std::string rvalue_message("rvalue");
        const Error<> rvalue(std::move(rvalue_message));
        CHECK_EQUAL(rvalue.what(), std::string("rvalue"));

        const Error<> typed_literal(17u, "typed literal");
        CHECK_EQUAL(typed_literal.type(), 17u);
        CHECK_EQUAL(typed_literal.what(), std::string("typed literal"));

        const std::string typed_lvalue_message("typed lvalue");
        const Error<> typed_lvalue(18u, typed_lvalue_message);
        CHECK_EQUAL(typed_lvalue.type(), 18u);
        CHECK_EQUAL(typed_lvalue.what(), typed_lvalue_message);

        std::string typed_rvalue_message("typed rvalue");
        const Error<> typed_rvalue(19u, std::move(typed_rvalue_message));
        CHECK_EQUAL(typed_rvalue.type(), 19u);
        CHECK_EQUAL(typed_rvalue.what(), std::string("typed rvalue"));

        const Error<> formatted("value=%d/%s", 42, "done");
        CHECK_EQUAL(formatted.what(), std::string("value=42/done"));
        CHECK_EQUAL(formatted.type(), sentinel);

        const Error<> typed_formatted(20u, "%s:%d", "line", 9);
        CHECK_EQUAL(typed_formatted.type(), 20u);
        CHECK_EQUAL(typed_formatted.what(), std::string("line:9"));

        const Error<ErrorCode> enum_error(ErrorCode::Write, "write %d", 3);
        CHECK(enum_error.type() == ErrorCode::Write);
        CHECK_EQUAL(enum_error.what(), std::string("write 3"));

        const Error<ErrorCode> default_enum_error;
        CHECK(default_enum_error.type() == ErrorCode::None);
    }

    TEST(null_and_empty_messages_avoid_undefined_library_preconditions)
    {
        const char* const null_message = nullptr;

        const Error<> null_literal(null_message);
        CHECK(null_literal.what().empty());

        const Error<> null_formatted(null_message, 7);
        CHECK(null_formatted.what().empty());

        const Error<> typed_null_literal(100u, null_message);
        CHECK_EQUAL(typed_null_literal.type(), 100u);
        CHECK(typed_null_literal.what().empty());

        const Error<> typed_null_formatted(101u, null_message, 8);
        CHECK_EQUAL(typed_null_formatted.type(), 101u);
        CHECK(typed_null_formatted.what().empty());

        const Error<> empty_formatted("%s", "");
        CHECK(empty_formatted.what().empty());

        const Result<int> error(Err("details"));
        try
        {
            static_cast<void>(error.Expect(nullptr));
            FAIL_CHECK("null context should still throw the stored error");
        }
        catch (const Err& thrown)
        {
            CHECK_EQUAL(thrown.what(), std::string("details"));
        }

        const Result<void> void_error(Err("void details"));
        try
        {
            void_error.Expect(nullptr);
            FAIL_CHECK("null context should still throw the stored void error");
        }
        catch (const Err& thrown)
        {
            CHECK_EQUAL(thrown.what(), std::string("void details"));
        }
    }

    TEST(error_copy_move_and_self_assignment)
    {
        Error<> original(31u, "original");
        const Error<> copied(original);
        CHECK_EQUAL(copied.what(), std::string("original"));
        CHECK_EQUAL(copied.type(), 31u);

        Error<> move_source(32u, "moved");
        const Error<> moved(std::move(move_source));
        CHECK_EQUAL(moved.what(), std::string("moved"));
        CHECK_EQUAL(moved.type(), 32u);

        Error<> copy_assigned(1u, "old");
        copy_assigned = original;
        CHECK_EQUAL(copy_assigned.what(), std::string("original"));
        CHECK_EQUAL(copy_assigned.type(), 31u);
        Error<>* const copy_self = &copy_assigned;
        copy_assigned = *copy_self;
        CHECK_EQUAL(copy_assigned.what(), std::string("original"));

        Error<> move_assigned(2u, "old move");
        Error<> move_assignment_source(33u, "move assigned");
        move_assigned = std::move(move_assignment_source);
        CHECK_EQUAL(move_assigned.what(), std::string("move assigned"));
        CHECK_EQUAL(move_assigned.type(), 33u);
        Error<>* const move_self = &move_assigned;
        move_assigned = std::move(*move_self);
        CHECK_EQUAL(move_assigned.what(), std::string("move assigned"));
        CHECK_EQUAL(move_assigned.type(), 33u);
    }

    TEST(error_formatting_handles_snprintf_failures_and_short_output)
    {
        static_cast<void>(unsetenv("RESULT_TEST_SNPRINTF_FORMAT_OVERRIDE"));
        SnprintfTest::Reset(SnprintfTest::Behavior::PassThrough);
        const Error<> ordinary(SnprintfTest::RuntimeFormat("ordinary %d"), 7);
        CHECK_EQUAL(ordinary.what(), std::string("ordinary 7"));
        CHECK_EQUAL(SnprintfTest::calls, 2);

        SnprintfTest::Reset(SnprintfTest::Behavior::FailFirst);
        const Error<> first_failure(SnprintfTest::RuntimeFormat("ignored %d"), 1);
        CHECK(first_failure.what().empty());
        CHECK_EQUAL(SnprintfTest::calls, 1);

        SnprintfTest::Reset(SnprintfTest::Behavior::FailSecond);
        const Error<> second_failure(SnprintfTest::RuntimeFormat("ignored %d"), 2);
        CHECK(second_failure.what().empty());
        CHECK_EQUAL(SnprintfTest::calls, 2);

        SnprintfTest::Reset(SnprintfTest::Behavior::ShortSecond);
        const Error<> short_output(SnprintfTest::RuntimeFormat("long output %d"), 3);
        CHECK_EQUAL(short_output.what(), std::string("ok"));
        CHECK_EQUAL(SnprintfTest::calls, 2);

        SnprintfTest::Reset(SnprintfTest::Behavior::PassThrough);
    }

    TEST(result_value_and_error_observers)
    {
        const int source = 42;
        const Result<int> value(source);
        CHECK(value.IsOk());
        CHECK(!value.IsErr());
        CHECK(static_cast<bool>(value));
        CHECK_EQUAL(value.Ok(), 42);
        CHECK_EQUAL(value.Unwrap(), 42);
    }

    TEST(result_error_observers_and_unwrap)
    {
        const Err source(55u, "failure");
        const Result<int> error(source);
        CHECK(!error.IsOk());
        CHECK(error.IsErr());
        CHECK(!static_cast<bool>(error));
        CHECK_EQUAL(error.Err().what(), std::string("failure"));
        CHECK_EQUAL(error.ErrType(), 55u);

        try
        {
            static_cast<void>(error.Unwrap());
            FAIL_CHECK("Unwrap should throw the stored error");
        }
        catch (const Err& thrown)
        {
            CHECK_EQUAL(thrown.what(), std::string("failure"));
            CHECK_EQUAL(thrown.type(), 55u);
        }
    }

    TEST(result_fallback_operations_cover_both_states)
    {
        const Result<std::string> value(std::string("kept"));
        const Result<std::string> error(Err("missing"));
        const std::string fallback("fallback");

        CHECK_EQUAL(value.UnwrapOr(fallback), std::string("kept"));
        CHECK_EQUAL(error.UnwrapOr(fallback), fallback);
        CHECK_EQUAL(value.UnwrapOrDefault(), std::string("kept"));
        CHECK_EQUAL(error.UnwrapOrDefault(), std::string());

        int callback_calls = 0;
        const auto callback = [&callback_calls](std::string prefix, int number) {
            ++callback_calls;
            return prefix + std::to_string(number);
        };
        CHECK_EQUAL(value.UnwrapOrElse(callback, std::string("unused"), 1), std::string("kept"));
        CHECK_EQUAL(callback_calls, 0);
        CHECK_EQUAL(error.UnwrapOrElse(callback, std::string("made"), 7), std::string("made7"));
        CHECK_EQUAL(callback_calls, 1);
    }

    TEST(result_expect_covers_value_and_error_states)
    {
        const Result<int> value(8);
        CHECK_EQUAL(value.Expect("unused: "), 8);

        const Result<int> error(Err("details"));
        try
        {
            static_cast<void>(error.Expect("context: "));
            FAIL_CHECK("Expect should throw a contextual error");
        }
        catch (const Err& thrown)
        {
            CHECK_EQUAL(thrown.what(), std::string("context: details"));
        }
    }

    TEST(result_copy_move_assignment_and_state_changes)
    {
        Result<std::string> value(std::string("value"));
        Result<std::string> error(Err(60u, "error"));

        const Result<std::string> copied_value(value);
        CHECK_EQUAL(copied_value.Ok(), std::string("value"));
        const Result<std::string> copied_error(error);
        CHECK_EQUAL(copied_error.Err().what(), std::string("error"));

        Result<std::string> move_source(std::string("move value"));
        const Result<std::string> moved_value(std::move(move_source));
        CHECK_EQUAL(moved_value.Ok(), std::string("move value"));
        Result<std::string> move_error_source(Err("move error"));
        const Result<std::string> moved_error(std::move(move_error_source));
        CHECK_EQUAL(moved_error.Err().what(), std::string("move error"));

        value = error;
        CHECK(value.IsErr());
        CHECK_EQUAL(value.ErrType(), 60u);
        error = Result<std::string>(std::string("replacement"));
        CHECK(error.IsOk());
        CHECK_EQUAL(error.Ok(), std::string("replacement"));

        Result<std::string>* const copy_self = &value;
        value = *copy_self;
        CHECK(value.IsErr());
        Result<std::string>* const move_self = &value;
        value = std::move(*move_self);
        CHECK(value.IsErr());
    }

    TEST(result_rvalue_constructors_and_ok_helper_preserve_moves)
    {
        MoveProbe::Reset();
        Result<MoveProbe> direct(MoveProbe(70));
        CHECK_EQUAL(direct.Ok().value, 70);
        CHECK_EQUAL(MoveProbe::copies, 0);
        CHECK_EQUAL(MoveProbe::moves, 1);

        Result<std::unique_ptr<int>> move_only(std::make_unique<int>(73));
        CHECK(move_only.IsOk());
        CHECK(move_only.Ok() != nullptr);
        CHECK_EQUAL(*move_only.Ok(), 73);

        MoveProbe::Reset();
        Result<MoveProbe> helper = Ok<MoveProbe>(71);
        CHECK_EQUAL(helper.Ok().value, 71);
        CHECK_EQUAL(MoveProbe::copies, 0);
        CHECK_GREATER_THAN_OR_EQUAL(MoveProbe::moves, 1);

        MoveProbe::Reset();
        Result<int, MoveProbe> error(MoveProbe(72));
        CHECK_EQUAL(error.Err().value, 72);
        CHECK_EQUAL(MoveProbe::copies, 0);
        CHECK_EQUAL(MoveProbe::moves, 1);
    }

    TEST(result_custom_errors_and_sfinae_traits)
    {
        static_assert(ResultUtil::ErrorHasType<CustomError>::value, "Type is detected");
        static_assert(ResultUtil::ErrorHasTypeFunction<CustomError>::value, "type() is detected");
        static_assert(ResultUtil::ErrorHasType<TypeOnlyError>::value, "Type-only is detected");
        static_assert(!ResultUtil::ErrorHasTypeFunction<TypeOnlyError>::value, "missing type() is detected");
        static_assert(!ResultUtil::ErrorHasType<FunctionOnlyError>::value, "missing Type is detected");
        static_assert(ResultUtil::ErrorHasTypeFunction<FunctionOnlyError>::value, "type() is detected");
        static_assert(!ResultUtil::ErrorHasType<BareError>::value, "bare errors have no Type");
        static_assert(!ResultUtil::ErrorHasTypeFunction<BareError>::value, "bare errors have no type()");

        static_assert(HasErrType<Result<int, CustomError>>::value, "typed error enables ErrType");
        static_assert(!HasErrType<Result<int, BareError>>::value, "bare error disables ErrType");
        static_assert(HasExpect<Result<int, CustomError>>::value, "what() enables Expect");
        static_assert(HasExpect<Result<int, WhatOnlyError>>::value, "only what() is required by Expect");
        static_assert(!HasExpect<Result<int, BareError>>::value, "missing what() disables Expect");
        static_assert(HasUnwrapOrDefault<Result<int>>::value, "default construction enables fallback");
        static_assert(!HasUnwrapOrDefault<Result<NoDefaultValue>>::value, "non-default values disable fallback");

        const Result<int, CustomError> typed(CustomError(ErrorCode::Read, "custom"));
        CHECK(typed.ErrType() == ErrorCode::Read);
        CHECK_EQUAL(typed.Err().what(), std::string("custom"));
        const Result<int, CustomError> typed_value(91);
        CHECK_EQUAL(typed_value.Expect("unused: "), 91);
        try
        {
            static_cast<void>(typed.Expect("while reading: "));
            FAIL_CHECK("custom Expect should throw");
        }
        catch (const CustomError& thrown)
        {
            CHECK(thrown.type() == ErrorCode::None);
            CHECK_EQUAL(thrown.what(), std::string("while reading: custom"));
        }
    }

    TEST(void_result_states_observers_and_operations)
    {
        const Result<void> value;
        CHECK(value.IsOk());
        CHECK(!value.IsErr());
        CHECK(static_cast<bool>(value));
        CHECK_NOTHROW(value.Unwrap());
        CHECK_NOTHROW(value.Expect("unused: "));

        int calls = 0;
        const auto callback = [&calls](int amount) { calls += amount; };
        value.UnwrapOrElse(callback, 1);
        CHECK_EQUAL(calls, 0);

        const Err error_source(80u, "void error");
        const Result<void> error(error_source);
        CHECK(!error.IsOk());
        CHECK(error.IsErr());
        CHECK(!static_cast<bool>(error));
        CHECK_EQUAL(error.Err().what(), std::string("void error"));
        CHECK_EQUAL(error.ErrType(), 80u);
        error.UnwrapOrElse(callback, 3);
        CHECK_EQUAL(calls, 3);

        try
        {
            error.Unwrap();
            FAIL_CHECK("void Unwrap should throw");
        }
        catch (const Err& thrown)
        {
            CHECK_EQUAL(thrown.what(), std::string("void error"));
        }

        try
        {
            error.Expect("context: ");
            FAIL_CHECK("void Expect should throw");
        }
        catch (const Err& thrown)
        {
            CHECK_EQUAL(thrown.what(), std::string("context: void error"));
        }
    }

    TEST(void_result_copy_move_assignment_helpers_and_nondefault_error)
    {
        Result<void, CustomError> error(CustomError(ErrorCode::Write, "custom void"));
        const Result<void, CustomError> copied(error);
        CHECK(copied.IsErr());
        CHECK(copied.ErrType() == ErrorCode::Write);

        Result<void, CustomError> move_source(CustomError(ErrorCode::Read, "move void"));
        const Result<void, CustomError> moved(std::move(move_source));
        CHECK(moved.IsErr());
        CHECK_EQUAL(moved.Err().what(), std::string("move void"));

        Result<void, CustomError> assigned;
        assigned = error;
        CHECK(assigned.IsErr());
        CHECK(assigned.ErrType() == ErrorCode::Write);
        assigned = Result<void, CustomError>();
        CHECK(assigned.IsOk());
        Result<void, CustomError>* const copy_self = &assigned;
        assigned = *copy_self;
        CHECK(assigned.IsOk());
        Result<void, CustomError>* const move_self = &assigned;
        assigned = std::move(*move_self);
        CHECK(assigned.IsOk());

        const Result<void> default_ok = Ok();
        CHECK(default_ok.IsOk());
        const Result<void, CustomError> custom_ok = Ok<CustomError>();
        CHECK(custom_ok.IsOk());
    }

#ifndef NDEBUG
    TEST(invalid_state_access_that_would_terminate_aborts_in_debug_builds)
    {
        CHECK(DiesFromAssertion([] {
            const Result<int> error(Err("error"));
            static_cast<void>(error.Ok());
        }));
        CHECK(DiesFromAssertion([] {
            const Result<int> value(1);
            static_cast<void>(value.Err());
        }));
        CHECK(DiesFromAssertion([] {
            const Result<void> value;
            static_cast<void>(value.Err());
        }));
        CHECK(DiesFromAssertion([] {
            const Result<int> value(1);
            static_cast<void>(value.ErrType());
        }));
        CHECK(DiesFromAssertion([] {
            const Result<void> value;
            static_cast<void>(value.ErrType());
        }));
    }
#endif
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
