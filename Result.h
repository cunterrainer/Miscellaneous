/*
    Example:
        Result<std::string> ReadFile(const char* p)
        {
            // open file
            if (file_is_open == false)
                return Err("Failed to open file: %s", p);
            // read content
            return content;
            // return Ok(content); // alternative, if not implicitly convertible Ok<std::string>(content)
        }

        Result<std::string> r = ReadFile("test.txt");
        if (r) // if valid, can also use r.IsOk() or r.IsErr()
        {
            const std::string& str = r.Ok();
        }
        else // warning r.Ok() or r.Err() are not allowed to be used if valid or invalid respectively
        {
            std::cout << r.Err().what() << std::endl;
        }
    Void result:
        The only difference when using a Result<void> is that you have to explicitly return Result<void> e.g.:
        Result<void> ProcessFile(const char* p)
        {
            // open file
            if (file_is_open == false)
                return Err("Failed to open file: %s", p);
            // process
            return Ok(); // or return {}; or return Result<void>();
        }
    Error handling:
        Result<std::string> r = ReadFile("test.txt");
        r.Unwrap(); // If valid returns const T&, otherwise throws Error with generic message
        r.Expect(); // If valid returns const T&, otherwise throws Error with custom message
        r.UnwrapOr(""); // If valid returns const T&, otherwise parameter
        r.UnwrapOrDefault(); // If valid returns const T&, otherwise T()
        r.UnwrapOrElse(RandomString, 0, 10); // If valid returns const T&, otherwise function return value

        Unwrap() and Expect() can throw to catch:
            try
            {
                r.Unwrap();
                r.Expect("Failed to read file: ");
            }
            catch (const Err& e)
            {
                std::cout << e.what() << std::endl;
            }
    Error types:
        If using the default provided Err, you can add enums or numbers when construction Err
        Example:
            enum Errors
            {
                IOError, MemoryError
            };

            Result<std::string> ReadFile(const char* p)
            {
                // open file
                if (file_is_open == false)
                    return Err(IOError, "Failed to open file: %s", p);
                // read content
                return content;
            }

            Result<std::string> r = ReadFile("test.txt");
            if (r.ErrType() == IOError)
            {
                std::cout << r.Err().what() << std::endl;
            }
    Custom error types:
        To use an enum class define your own error type:
        enum class Errors
        {
            IOError, MemoryError
        };
        using OwnErr = Error<Errors>;

        Result<std::string, OwnErr> ReadFile(const char* p)
        {
            // open file
            if (file_is_open == false)
                return Err(Errors::IOError, "Failed to open file: %s", p);
            // read content
            return content;
        }

        Result<std::string, OwnErr> r = ReadFile("test.txt");
        if (r.ErrType() == Errors::IOError)
        {
            std::cout << r.Err().what() << std::endl;
        }
    Custom error structs:
        The easiest solution is to inherit from Err.
        Example:
        struct IOError : Err
        {
            using Err::Err;
        };

        Result<std::string, IOError> str = ReadFile("test.txt");

        If you don't want to inherit from Err, the error struct has to provide:
        struct IOError
        {
            // Mandatory:
                // has to be copiable and movable
                inline IOError(const IOError& other);
                inline IOError(IOError&& other) noexcept;
                IOError& operator=(const IOError& other);
                IOError& operator=(IOError&& other) noexcept;

            // Optional:
                // if you want to use Result<type, Err>::ErrType()
                using Type = ErrorType; (e.g. size_t)
                inline Type type() const noexcept;

                // Result<type, Err>::Expect()
                inline const std::string& what() const noexcept;
        };
        the rest is up to you e.g. providing a constructor for error messages e.g.:
        IOError(const char* what);
        Result<type, IOError>::Unwrap() and Result<type, IOError>::Expect() will throw IOError
*/
#ifndef RESULT_H
#define RESULT_H
#include <cassert>
#include <limits>
#include <cstdio>
#include <string>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <variant>

template <typename ErrorType = std::size_t>
struct Error
{
public:
    using Type = ErrorType;
private:
    static constexpr Type DefaultType() noexcept
    {
        if constexpr (std::is_enum<Type>::value)
            return static_cast<Type>(0);
        else
            return static_cast<Type>(std::numeric_limits<std::size_t>::max());
    }

    std::string m_What;
    Type m_Type = DefaultType();
public:
    template <typename... Args>
    inline explicit Error(const char* what, Args&&... args)
    {
        const int formatted_size = std::snprintf(nullptr, 0, what, std::forward<Args>(args)...);
        if (formatted_size < 0)
            return;

        const std::size_t size = static_cast<std::size_t>(formatted_size);
        m_What.resize(size + 1);
        const int written = std::snprintf(m_What.data(), m_What.size(), what, std::forward<Args>(args)...);
        if (written < 0)
        {
            m_What.clear();
            return;
        }
        const std::size_t written_size = static_cast<std::size_t>(written);
        m_What.resize(written_size < size ? written_size : size);
    }
    inline explicit Error(const char* what) : m_What(what) {}
    inline explicit Error(const std::string& what) : m_What(what) {}
    inline explicit Error(std::string&& what) noexcept : m_What(std::move(what)) {}

    template <typename... Args> inline explicit Error(Type type, const char* what, Args&&... args) : Error(what, std::forward<Args>(args)...) { m_Type = type; }
    inline explicit Error(Type type, const char* what) : m_What(what), m_Type(type) {}
    inline explicit Error(Type type, const std::string& what) : m_What(what), m_Type(type) {}
    inline explicit Error(Type type, std::string&& what) noexcept : m_What(std::move(what)), m_Type(type) {}
    inline explicit Error() = default;

    inline Error(const Error& other) : m_What(other.m_What), m_Type(other.m_Type) {}
    inline Error(Error&& other) noexcept : m_What(std::move(other.m_What)), m_Type(other.m_Type) {}

    Error& operator=(const Error& other)
    {
        if (this != &other)
        {
            m_What = other.m_What;
            m_Type = other.m_Type;
        }
        return *this;
    }

    Error& operator=(Error&& other) noexcept
    {
        if (this != &other)
        {
            m_What = std::move(other.m_What);
            m_Type = other.m_Type;
        }
        return *this;
    }

    inline const std::string& what() const noexcept
    {
        return m_What;
    }

    inline Type type() const noexcept
    {
        return m_Type;
    }
};
using Err = Error<std::size_t>;


namespace ResultUtil
{
    template <typename E, typename = void>
    struct ErrorHasType : std::false_type {};

    template <typename E>
    struct ErrorHasType<E, std::void_t<typename E::Type>> : std::true_type {};

    template <typename E, typename = void>
    struct ErrorHasTypeFunction : std::false_type {};

    template <typename E>
    struct ErrorHasTypeFunction<E, std::void_t<decltype(std::declval<E>().type())>> : std::true_type {};
}

template <typename T, typename E = Err>
class Result;

template <typename T, typename E>
class Result
{
    static_assert(std::is_copy_assignable<E>::value, "Result::Error has to be copy assignable");
    static_assert(std::is_move_assignable<E>::value, "Result::Error type has to be move assignable");
    static_assert(std::is_copy_constructible<E>::value, "Result::Error type has to be copiable");
    static_assert(std::is_move_constructible<E>::value, "Result::Error type has to be movable");
private:
    std::variant<T, E> m_Value;
public:
    inline Result(const E& e) : m_Value(std::in_place_index<1>, e) {}
    inline Result(E&& e) : m_Value(std::in_place_index<1>, std::move(e)) {}
    inline Result(const T& t) : m_Value(std::in_place_index<0>, t) {}
    inline Result(T&& t) : m_Value(std::in_place_index<0>, std::move(t)) {}

    Result(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) = default;
    ~Result() = default;

    inline const T& Ok() const noexcept
    {
        assert(IsOk() && "Don't access the Ok() value if it is an error, use IsOk() to check beforehand!");
        return std::get<0>(m_Value);
    }

    inline const E& Err() const noexcept
    {
        assert(IsErr() && "Don't access the Err() value if it is not an error, use IsErr() to check beforehand!");
        return std::get<1>(m_Value);
    }

    template <typename U = E, std::enable_if_t<ResultUtil::ErrorHasType<U>::value && ResultUtil::ErrorHasTypeFunction<U>::value, int> = 0>
    inline typename U::Type ErrType() const noexcept
    {
        return Err().type();
    }

    explicit operator bool() const noexcept
    {
        return IsOk();
    }

    inline bool IsOk() const noexcept
    {
        return m_Value.index() == 0;
    }

    inline bool IsErr() const noexcept
    {
        return m_Value.index() == 1;
    }

    inline const T& Unwrap() const
    {
        if (IsOk())
            return std::get<0>(m_Value);
        throw std::get<1>(m_Value);
    }

    inline const T& UnwrapOr(const T& t) const
    {
        if (IsOk())
            return std::get<0>(m_Value);
        return t;
    }

    template <typename U = T, typename std::enable_if<std::is_default_constructible<U>::value, int>::type = 0>
    inline T UnwrapOrDefault() const
    {
        if (IsOk())
            return std::get<0>(m_Value);
        return T();
    }

    template <typename Func, typename... Args>
    inline T UnwrapOrElse(const Func& f, Args&&... args) const
    {
        if (IsOk())
            return std::get<0>(m_Value);
        return f(std::forward<Args>(args)...);
    }

    template <typename U = E, typename = std::void_t<decltype(std::declval<U>().what())>>
    inline const T& Expect(const char* msg) const
    {
        if (IsOk())
            return std::get<0>(m_Value);
        throw E(msg + std::get<1>(m_Value).what());
    }
};


template <typename E>
class Result<void, E>
{
    static_assert(std::is_copy_assignable<E>::value, "Result<void>::Error has to be copy assignable");
    static_assert(std::is_move_assignable<E>::value, "Result<void>::Error type has to be move assignable");
    static_assert(std::is_copy_constructible<E>::value, "Result<void>::Error type has to be copiable");
    static_assert(std::is_move_constructible<E>::value, "Result<void>::Error type has to be movable");
private:
    std::variant<std::monostate, E> m_Value;
public:
    inline Result(const E& e) : m_Value(std::in_place_index<1>, e) {}
    inline Result(E&& e) : m_Value(std::in_place_index<1>, std::move(e)) {}
    inline Result() : m_Value(std::in_place_index<0>) {}

    Result(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) = default;
    ~Result() = default;

    inline const E& Err() const noexcept
    {
        assert(IsErr() && "Don't access the Err() value if it is not an error, use IsErr() to check beforehand!");
        return std::get<1>(m_Value);
    }

    template <typename U = E, std::enable_if_t<ResultUtil::ErrorHasType<U>::value && ResultUtil::ErrorHasTypeFunction<U>::value, int> = 0>
    inline typename U::Type ErrType() const noexcept
    {
        return Err().type();
    }

    explicit operator bool() const noexcept
    {
        return IsOk();
    }

    inline bool IsOk() const noexcept
    {
        return m_Value.index() == 0;
    }

    inline bool IsErr() const noexcept
    {
        return m_Value.index() == 1;
    }

    inline void Unwrap() const
    {
        if (IsErr())
            throw std::get<1>(m_Value);
    }

    template <typename Func, typename... Args>
    inline void UnwrapOrElse(const Func& f, Args&&... args) const
    {
        if (IsErr())
            return f(std::forward<Args>(args)...);
    }

    template <typename U = E, typename = std::void_t<decltype(std::declval<U>().what())>>
    inline void Expect(const char* msg) const
    {
        if (IsErr())
            throw E(msg + std::get<1>(m_Value).what());
    }
};

template <typename T, typename E = Err, typename... Args>
inline Result<T, E> Ok(Args&&... args)
{
    return T(std::forward<Args>(args)...);
}

template <typename E = Err>
inline Result<void, E> Ok()
{
    return Result<void, E>();
}
#endif // RESULT_H
