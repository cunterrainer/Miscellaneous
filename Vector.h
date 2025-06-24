#ifndef VECTOR_H
#define VECTOR_H

/*
    Complete Vector2, Vector3 and Vector4 implementations
*/

#include <cmath>
#include <utility>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <type_traits>

#define VECTOR_INLINE    constexpr
#define VECTOR_NAMESPACE Vec

namespace VECTOR_NAMESPACE
{
    // ------------------------------------ Vector2 ------------------------------------
    template <typename T>
    class Vector2
    {
    private:
        // result_t type: double for ints otherwise float, double, long double
        using result_t = std::conditional_t<std::is_integral_v<T>, double, T>;
    private:
        static constexpr float FloatEpsilon() noexcept
        {
            return 0.000001f;
        }
    public:
        T x;
        T y;
    public:
        VECTOR_INLINE Vector2() noexcept : x(static_cast<T>(0)), y(static_cast<T>(0)) {}
        VECTOR_INLINE Vector2(T d) noexcept : x(d), y(d) {}
        VECTOR_INLINE Vector2(T x, T y) noexcept : x(x), y(y) {}

        inline Vector2(Vector2&&) = default;
        inline Vector2(const Vector2&) = default;
        inline Vector2& operator=(Vector2&&) = default;
        inline Vector2& operator=(const Vector2&) = default;

        // Conversion constructors
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector2(const Vector2<U>& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector2(Vector2<U>&& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)) {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector2& operator=(Vector2<U>&& other) noexcept
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            return *this;
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector2& operator=(const Vector2<U>& other) noexcept
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            return *this;
        }

        // Vector operators
        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector2<R> operator+(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x + other.x, y + other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector2<R> operator-(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x - other.x, y - other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector2<R> operator*(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x * other.x, y * other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector2<R> operator/(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x / other.x, y / other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector2<R> operator%(const Vector2<U>& other) const noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector2<R>(std::fmod(x, other.x), std::fmod(y, other.y));
            else
                return Vector2<R>(x % other.x, y % other.y);
        }

        // Scalar operations
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2<R> operator+(U scalar) const noexcept
        {
            return Vector2<R>(x + scalar, y + scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2<R> operator-(U scalar) const noexcept
        {
            return Vector2<R>(x - scalar, y - scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2<R> operator*(U scalar) const noexcept
        {
            return Vector2<R>(x * scalar, y * scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2<R> operator/(U scalar) const noexcept
        {
            return Vector2<R>(x / scalar, y / scalar);
        }
        
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2 operator%(U scalar) const noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector2<R>(std::fmod(x, scalar), std::fmod(y, scalar));
            else
                return Vector2<R>(x % scalar, y % scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator+(U scalar, const Vector2& v) noexcept
        {
            return Vector2<R>(scalar + v.x, scalar + v.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator-(U scalar, const Vector2& v) noexcept
        {
            return Vector2<R>(scalar - v.x, scalar - v.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator*(U scalar, const Vector2& v) noexcept
        {
            return Vector2<R>(scalar * v.x, scalar * v.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator/(U scalar, const Vector2& v) noexcept
        {
            return Vector2<R>(scalar / v.x, scalar / v.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator%(U scalar, const Vector2& v) noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector2<R>(std::fmod(scalar, v.x), std::fmod(scalar, v.y));
            else
                return Vector2<R>(scalar % v.x, scalar % v.y);
        }

        // Compound assignment
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector2& operator+=(const Vector2<U>& other) noexcept
        {
            x += static_cast<T>(other.x);
            y += static_cast<T>(other.y);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector2& operator-=(const Vector2<U>& other) noexcept
        {
            x -= static_cast<T>(other.x);
            y -= static_cast<T>(other.y);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector2& operator*=(const Vector2<U>& other) noexcept
        {
            x *= static_cast<T>(other.x);
            y *= static_cast<T>(other.y);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector2& operator/=(const Vector2<U>& other) noexcept
        {
            x /= static_cast<T>(other.x);
            y /= static_cast<T>(other.y);
            return *this;
        }
        
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector2& operator%=(const Vector2<U>& other) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                x = std::fmod(x, static_cast<T>(other.x));
                y = std::fmod(y, static_cast<T>(other.y));
            }
            else
            {
                x %= static_cast<T>(other.x);
                y %= static_cast<T>(other.y);
            }
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2& operator+=(U scalar) noexcept
        {
            x += static_cast<T>(scalar);
            y += static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2& operator-=(U scalar) noexcept
        {
            x -= static_cast<T>(scalar);
            y -= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2& operator*=(U scalar) noexcept
        {
            x *= static_cast<T>(scalar);
            y *= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2& operator/=(U scalar) noexcept
        {
            x /= static_cast<T>(scalar);
            y /= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector2& operator%=(U scalar) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                x = std::fmod(x, static_cast<T>(scalar));
                y = std::fmod(y, static_cast<T>(scalar));
            }
            else
            {
                x %= static_cast<T>(scalar);
                y %= static_cast<T>(scalar);
            }
            return *this;
        }

        // Unary operators
        VECTOR_INLINE Vector2 operator-() const noexcept
        {
            return Vector2(-x, -y);
        }

        VECTOR_INLINE Vector2 operator+() const noexcept
        {
            return *this;
        }

        // Checks if x and y are zero
        VECTOR_INLINE explicit operator bool() const noexcept
        {
            return x != T(0) || y != T(0);
        }

        // Comparison
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator==(const Vector2<U>& other) const noexcept
        {
            const Vector2 cmp = other;

            if constexpr (std::is_floating_point_v<T>)
            {
                return std::fabs(x - cmp.x) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(x), std::fabs(cmp.x))) &&
                       std::fabs(y - cmp.y) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(y), std::fabs(cmp.y)));
            }
            else
            {
                return x == cmp.x && y == cmp.y;
            }
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator!=(const Vector2<U>& other) const noexcept
        {
            return !(*this == other);
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator<(const Vector2<U>& other) const noexcept
        {
            return LengthSqr() < other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator<=(const Vector2<U>& other) const noexcept
        {
            return LengthSqr() <= other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator>(const Vector2<U>& other) const noexcept
        {
            return LengthSqr() > other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator>=(const Vector2<U>& other) const noexcept
        {
            return LengthSqr() >= other.LengthSqr();
        }

        // Bitwise AND
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2<R> operator&(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x & other.x, y & other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2<R> operator&(U other) const noexcept
        {
            return Vector2<R>(x & other, y & other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator&(U other, const Vector2& v) noexcept
        {
            return Vector2<R>(other & v.x, other & v.y);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2& operator&=(const Vector2<U>& other) noexcept
        {
            x &= other.x;
            y &= other.y;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2& operator&=(U other) noexcept
        {
            x &= other;
            y &= other;
            return *this;
        }

        // Bitwise OR
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2<R> operator|(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x | other.x, y | other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2<R> operator|(U other) const noexcept
        {
            return Vector2<R>(x | other, y | other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator|(U other, const Vector2& v) noexcept
        {
            return Vector2<R>(other | v.x, other | v.y);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2& operator|=(const Vector2<U>& other) noexcept
        {
            x |= other.x;
            y |= other.y;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2& operator|=(U other) noexcept
        {
            x |= other;
            y |= other;
            return *this;
        }

        // Bitwise XOR
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2<R> operator^(const Vector2<U>& other) const noexcept
        {
            return Vector2<R>(x ^ other.x, y ^ other.y);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2<R> operator^(U other) const noexcept
        {
            return Vector2<R>(x ^ other, y ^ other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector2<R> operator^(U other, const Vector2& v) noexcept
        {
            return Vector2<R>(other ^ v.x, other ^ v.y);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2& operator^=(const Vector2<U>& other) noexcept
        {
            x ^= other.x;
            y ^= other.y;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector2& operator^=(U other) noexcept
        {
            x ^= other;
            y ^= other;
            return *this;
        }

        // Bitwise NOT
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector2 operator~() const noexcept
        {
            return Vector2(~x, ~y);
        }

        // Shift left
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector2 operator<<(int shift) const noexcept
        {
            return Vector2(x << shift, y << shift);
        }

        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector2& operator<<=(int shift) noexcept
        {
            x <<= shift;
            y <<= shift;
            return *this;
        }

        // Shift right
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector2 operator>>(int shift) const noexcept
        {
            return Vector2(x >> shift, y >> shift);
        }

        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector2& operator>>=(int shift) noexcept
        {
            x >>= shift;
            y >>= shift;
            return *this;
        }

        // Output stream
        inline friend std::ostream& operator<<(std::ostream& os, const Vector2& v)
        {
            return os << "(" << v.x << ", " << v.y << ")";
        }

        // Utility
        template <typename U, std::enable_if_t<std::is_convertible_v<T, U>, int> = 0>
        VECTOR_INLINE Vector2<U> Convert() const noexcept
        {
            return Vector2<U>(static_cast<U>(x), static_cast<U>(y));
        }

        inline result_t Length() const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            return std::sqrt((static_cast<result_t>(x)*x) + (static_cast<result_t>(y)*y));
        }

        VECTOR_INLINE T LengthSqr() const noexcept
        {
            return (x*x) + (y*y);
        }

        VECTOR_INLINE T DotProduct(const Vector2& other) const noexcept
        {
            return x * other.x + y * other.y;
        }

        inline result_t Distance(const Vector2& other) const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            return std::sqrt((static_cast<result_t>(x) - other.x) * (static_cast<result_t>(x) - other.x) + (static_cast<result_t>(y) - other.y) * (static_cast<result_t>(y) - other.y));
        }

        VECTOR_INLINE T DistanceSqr(const Vector2& other) const noexcept
        {
            return (x - other.x) * (x - other.x) + (y - other.y) * (y - other.y);
        }

        // Calculate angle between two vectors in radians
        // NOTE: Angle is calculated from origin point (0, 0)
        inline result_t Angle(const Vector2& other) const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            const T dot = DotProduct(other);
            const T det = x*other.y - y*other.x;
            return std::atan2(det, dot);
        }

        Vector2<result_t> Normalize() const noexcept
        {
            Vector2<result_t> result;
            const result_t length = Length();

            if (length > 0)
            {
                const result_t ilength = static_cast<result_t>(1) / length;
                result.x = x * ilength;
                result.y = y * ilength;
            }

            return result;
        }

        Vector2& NormalizeInPlace() noexcept
        {
            *this = Normalize();
            return *this;
        }

        // Calculate linear interpolation between two vectors
        VECTOR_INLINE Vector2<result_t> Lerp(const Vector2& other, result_t amount) const noexcept
        {
            Vector2<result_t> result;

            result.x = x + amount * (other.x - x);
            result.y = y + amount * (other.y - y);
            return result;
        }

        // Calculate reflected vector to normal
        VECTOR_INLINE Vector2 Reflect(const Vector2& normal) const noexcept
        {
            const T dot = DotProduct(normal);
            
            Vector2 result;
            result.x = x - (static_cast<T>(2) * normal.x) * dot;
            result.y = y - (static_cast<T>(2) * normal.y) * dot;
            return result;
        }

        VECTOR_INLINE Vector2& ReflectInPlace(const Vector2& normal) noexcept
        {
            *this = Reflect(normal);
            return *this;
        }

        // Rotate vector by angle in radians
        Vector2<result_t> Rotate(result_t angle) const noexcept
        {
            const result_t cosres = std::cos(angle);
            const result_t sinres = std::sin(angle);
            
            Vector2<result_t> result;
            result.x = x*cosres - y*sinres;
            result.y = x*sinres + y*cosres;
            return result;
        }

        Vector2& RotateInPlace(result_t angle) noexcept
        {
            *this = Rotate(angle);
            return *this;
        }

        // Move Vector towards target
        Vector2<result_t> MoveTowards(const Vector2& target, result_t maxDistance) const noexcept
        {
            const result_t dx = target.x - x;
            const result_t dy = target.y - y;
            const result_t value = (dx*dx) + (dy*dy);

            if (value == 0 || ((maxDistance >= 0) && value <= maxDistance * maxDistance)) return target;

            Vector2<result_t> result;
            const result_t dist = std::sqrt(value);
            result.x = x + dx/dist*maxDistance;
            result.y = y + dy/dist*maxDistance;
            return result;
        }

        Vector2& MoveTowardsInPlace(const Vector2& target, result_t maxDistance) noexcept
        {
            *this = MoveTowards(target, maxDistance);
            return *this;
        }

        VECTOR_INLINE Vector2<result_t> Invert() const noexcept
        {
            return Vector2<result_t>(result_t(1) / x, result_t(1) / y);
        }

        VECTOR_INLINE Vector2& InvertInPlace() noexcept
        {
            *this = Invert();
            return *this;
        }

        VECTOR_INLINE Vector2 Clamp(const Vector2& min, const Vector2& max) const noexcept
        {
            Vector2 result;
            result.x = std::clamp(x, min.x, max.x);
            result.y = std::clamp(y, min.y, max.y);
            return result;
        }

        VECTOR_INLINE Vector2& ClampInPlace(const Vector2& min, const Vector2& max) noexcept
        {
            *this = Clamp(min, max);
            return *this;
        }

        Vector2<result_t> ClampMagnitude(result_t min, result_t max) const noexcept
        {
            Vector2<result_t> result(x, y);

            result_t length = LengthSqr();
            if (length > result_t(0))
            {
                length = std::sqrt(length);

                result_t scale = result_t(1);
                if (length < min)
                {
                    scale = min / length;
                }
                else if (length > max)
                {
                    scale = max / length;
                }

                result.x = x * scale;
                result.y = y * scale;
            }
        
            return result;
        }

        Vector2<T>& ClampMagnitudeInPlace(result_t min, result_t max) noexcept
        {
            *this = ClampMagnitude(min, max);
            return *this;
        }
    };

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector2<R> Min(const Vector2<T1>& v1, const Vector2<T2>& v2) noexcept
    {
        return Vector2<R>(std::min(static_cast<R>(v1.x), static_cast<R>(v2.x)), std::min(static_cast<R>(v1.y), static_cast<R>(v2.y)));
    }

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector2<R> Max(const Vector2<T1>& v1, const Vector2<T2>& v2) noexcept
    {
        return Vector2<R>(std::max(static_cast<R>(v1.x), static_cast<R>(v2.x)), std::max(static_cast<R>(v1.y), static_cast<R>(v2.y)));
    }

    using Vector2i8  = Vector2<std::int8_t>;
    using Vector2i16 = Vector2<std::int16_t>;
    using Vector2i32 = Vector2<std::int32_t>;
    using Vector2i64 = Vector2<std::int64_t>;

    using Vector2u8  = Vector2<std::uint8_t>;
    using Vector2u16 = Vector2<std::uint16_t>;
    using Vector2u32 = Vector2<std::uint32_t>;
    using Vector2u64 = Vector2<std::uint64_t>;
    
    using Vector2f  = Vector2<float>;
    using Vector2d  = Vector2<double>;
    using Vector2ld = Vector2<long double>;

    // ------------------------------------ Vector3 ------------------------------------
    template <typename T>
    class Vector3
    {
    private:
        // result_t type: double for ints otherwise float, double, long double
        using result_t = std::conditional_t<std::is_integral_v<T>, double, T>;
    private:
        static constexpr float FloatEpsilon() noexcept
        {
            return 0.000001f;
        }
    public:
        T x;
        T y;
        T z;
    public:
        VECTOR_INLINE Vector3() noexcept : x(static_cast<T>(0)), y(static_cast<T>(0)), z(static_cast<T>(0)) {}
        VECTOR_INLINE Vector3(T d) noexcept : x(d), y(d), z(d) {}
        VECTOR_INLINE Vector3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}

        inline Vector3(Vector3&&) = default;
        inline Vector3(const Vector3&) = default;
        inline Vector3& operator=(Vector3&&) = default;
        inline Vector3& operator=(const Vector3&) = default;

        // Conversion constructors
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector3(const Vector3<U>& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector3(Vector3<U>&& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)) {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector3& operator=(Vector3<U>&& other) noexcept
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            z = static_cast<T>(other.z);
            return *this;
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector3& operator=(const Vector3<U>& other) noexcept
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            z = static_cast<T>(other.z);
            return *this;
        }

        // Vector operators
        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector3<R> operator+(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x + other.x, y + other.y, z + other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector3<R> operator-(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x - other.x, y - other.y, z - other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector3<R> operator*(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x * other.x, y * other.y, z * other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector3<R> operator/(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x / other.x, y / other.y, z / other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector3<R> operator%(const Vector3<U>& other) const noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector3<R>(std::fmod(x, other.x), std::fmod(y, other.y), std::fmod(z, other.z));
            else
                return Vector3<R>(x % other.x, y % other.y, z % other.z);
        }

        // Scalar operations
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3<R> operator+(U scalar) const noexcept
        {
            return Vector3<R>(x + scalar, y + scalar, z + scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3<R> operator-(U scalar) const noexcept
        {
            return Vector3<R>(x - scalar, y - scalar, z - scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3<R> operator*(U scalar) const noexcept
        {
            return Vector3<R>(x * scalar, y * scalar, z * scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3<R> operator/(U scalar) const noexcept
        {
            return Vector3<R>(x / scalar, y / scalar, z / scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3<R> operator%(U scalar) const noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector3<R>(std::fmod(x, scalar), std::fmod(y, scalar), std::fmod(z, scalar));
            else
                return Vector3<R>(x % scalar, y % scalar, z % scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator+(U scalar, const Vector3& v) noexcept
        {
            return Vector3<R>(scalar + v.x, scalar + v.y, scalar + v.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator-(U scalar, const Vector3& v) noexcept
        {
            return Vector3<R>(scalar - v.x, scalar - v.y, scalar - v.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator*(U scalar, const Vector3& v) noexcept
        {
            return Vector3<R>(scalar * v.x, scalar * v.y, scalar * v.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator/(U scalar, const Vector3& v) noexcept
        {
            return Vector3<R>(scalar / v.x, scalar / v.y, scalar / v.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator%(U scalar, const Vector3& v) noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector3<R>(std::fmod(scalar, v.x), std::fmod(scalar, v.y), std::fmod(scalar, v.z));
            else
                return Vector3<R>(scalar % v.x, scalar % v.y, scalar % v.z);
        }

        // Compound assignment
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector3& operator+=(const Vector3<U>& other) noexcept
        {
            x += static_cast<T>(other.x);
            y += static_cast<T>(other.y);
            z += static_cast<T>(other.z);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector3& operator-=(const Vector3<U>& other) noexcept
        {
            x -= static_cast<T>(other.x);
            y -= static_cast<T>(other.y);
            z -= static_cast<T>(other.z);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector3& operator*=(const Vector3<U>& other) noexcept
        {
            x *= static_cast<T>(other.x);
            y *= static_cast<T>(other.y);
            z *= static_cast<T>(other.z);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector3& operator/=(const Vector3<U>& other) noexcept
        {
            x /= static_cast<T>(other.x);
            y /= static_cast<T>(other.y);
            z /= static_cast<T>(other.z);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector3& operator%=(const Vector3<U>& other) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                x = std::fmod(x, static_cast<T>(other.x));
                y = std::fmod(y, static_cast<T>(other.y));
                z = std::fmod(z, static_cast<T>(other.z));
            }
            else
            {
                x %= static_cast<T>(other.x);
                y %= static_cast<T>(other.y);
                z %= static_cast<T>(other.z);
            }
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3& operator+=(U scalar) noexcept
        {
            x += static_cast<T>(scalar);
            y += static_cast<T>(scalar);
            z += static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3& operator-=(U scalar) noexcept
        {
            x -= static_cast<T>(scalar);
            y -= static_cast<T>(scalar);
            z -= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3& operator*=(U scalar) noexcept
        {
            x *= static_cast<T>(scalar);
            y *= static_cast<T>(scalar);
            z *= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3& operator/=(U scalar) noexcept
        {
            x /= static_cast<T>(scalar);
            y /= static_cast<T>(scalar);
            z /= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector3& operator%=(U scalar) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                x = std::fmod(x, static_cast<T>(scalar));
                y = std::fmod(y, static_cast<T>(scalar));
                z = std::fmod(z, static_cast<T>(scalar));
            }
            else
            {
                x %= static_cast<T>(scalar);
                y %= static_cast<T>(scalar);
                z %= static_cast<T>(scalar);
            }
            return *this;
        }

        // Unary operators
        VECTOR_INLINE Vector3 operator-() const noexcept
        {
            return Vector3(-x, -y, -z);
        }

        VECTOR_INLINE Vector3 operator+() const noexcept
        {
            return *this;
        }

        // Checks if x, y and z are zero
        VECTOR_INLINE explicit operator bool() const noexcept
        {
            return x != T(0) || y != T(0) || z != T(0);
        }

        // Comparison
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator==(const Vector3<U>& other) const noexcept
        {
            const Vector3 cmp = other;

            if constexpr (std::is_floating_point_v<T>)
            {
                return std::fabs(x - cmp.x) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(x), std::fabs(cmp.x))) &&
                    std::fabs(y - cmp.y) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(y), std::fabs(cmp.y))) &&
                    std::fabs(z - cmp.z) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(z), std::fabs(cmp.z)));
            }
            else
            {
                return x == cmp.x && y == cmp.y && z == cmp.z;
            }
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator!=(const Vector3<U>& other) const noexcept
        {
            return !(*this == other);
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator<(const Vector3<U>& other) const noexcept
        {
            return LengthSqr() < other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator<=(const Vector3<U>& other) const noexcept
        {
            return LengthSqr() <= other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator>(const Vector3<U>& other) const noexcept
        {
            return LengthSqr() > other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator>=(const Vector3<U>& other) const noexcept
        {
            return LengthSqr() >= other.LengthSqr();
        }

        // Bitwise AND
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3<R> operator&(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x & other.x, y & other.y, z & other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3<R> operator&(U other) const noexcept
        {
            return Vector3<R>(x & other, y & other, z & other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator&(U other, const Vector3& v) noexcept
        {
            return Vector3<R>(other & v.x, other & v.y, other & v.z);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3& operator&=(const Vector3<U>& other) noexcept
        {
            x &= other.x;
            y &= other.y;
            z &= other.z;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3& operator&=(U other) noexcept
        {
            x &= other;
            y &= other;
            z &= other;
            return *this;
        }

        // Bitwise OR
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3<R> operator|(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x | other.x, y | other.y, z | other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3<R> operator|(U other) const noexcept
        {
            return Vector3<R>(x | other, y | other, z | other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator|(U other, const Vector3& v) noexcept
        {
            return Vector3<R>(other | v.x, other | v.y, other | v.z);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3& operator|=(const Vector3<U>& other) noexcept
        {
            x |= other.x;
            y |= other.y;
            z |= other.z;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3& operator|=(U other) noexcept
        {
            x |= other;
            y |= other;
            z |= other;
            return *this;
        }

        // Bitwise XOR
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3<R> operator^(const Vector3<U>& other) const noexcept
        {
            return Vector3<R>(x ^ other.x, y ^ other.y, z ^ other.z);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3<R> operator^(U other) const noexcept
        {
            return Vector3<R>(x ^ other, y ^ other, z ^ other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector3<R> operator^(U other, const Vector3& v) noexcept
        {
            return Vector3<R>(other ^ v.x, other ^ v.y, other ^ v.z);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3& operator^=(const Vector3<U>& other) noexcept
        {
            x ^= other.x;
            y ^= other.y;
            z ^= other.z;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector3& operator^=(U other) noexcept
        {
            x ^= other;
            y ^= other;
            z ^= other;
            return *this;
        }

        // Bitwise NOT
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector3 operator~() const noexcept
        {
            return Vector3(~x, ~y, ~z);
        }

        // Shift left
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector3 operator<<(int shift) const noexcept
        {
            return Vector3(x << shift, y << shift, z << shift);
        }

        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector3& operator<<=(int shift) noexcept
        {
            x <<= shift;
            y <<= shift;
            z <<= shift;
            return *this;
        }

        // Shift right
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector3 operator>>(int shift) const noexcept
        {
            return Vector3(x >> shift, y >> shift, z >> shift);
        }

        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector3& operator>>=(int shift) noexcept
        {
            x >>= shift;
            y >>= shift;
            z >>= shift;
            return *this;
        }

        // Output stream
        inline friend std::ostream& operator<<(std::ostream& os, const Vector3& v)
        {
            return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        }

        // Utility
        template <typename U, std::enable_if_t<std::is_convertible_v<T, U>, int> = 0>
        VECTOR_INLINE Vector3<U> Convert() const noexcept
        {
            return Vector3<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z));
        }

        VECTOR_INLINE Vector3 CrossProduct(const Vector3& other) const noexcept
        {
            return Vector3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
        }

        VECTOR_INLINE Vector3& CrossProductInPlace(const Vector3& other) noexcept
        {
            *this = CrossProduct(other);
            return *this;
        }

        // Calculate one vector perpendicular vector
        VECTOR_INLINE Vector3<result_t> Perpendicular() const noexcept
        {
            result_t min = std::fabs(x);
            Vector3<result_t> cardinalAxis(1, 0, 0);

            if (std::fabs(y) < min)
            {
                min = std::fabs(y);
                cardinalAxis = Vector3<result_t>(0, 1, 0);
            }

            if (std::fabs(z) < min)
            {
                cardinalAxis = Vector3<result_t>(0, 0, 1);
            }

            return CrossProduct(cardinalAxis);
        }

        VECTOR_INLINE Vector3& PerpendicularInPlace() noexcept
        {
            *this = Perpendicular();
            return *this;
        }

        inline result_t Length() const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            return std::sqrt((static_cast<result_t>(x)*x) + (static_cast<result_t>(y)*y) + (static_cast<result_t>(z)*z));
        }

        VECTOR_INLINE T LengthSqr() const noexcept
        {
            return (x*x) + (y*y) + (z*z);
        }

        VECTOR_INLINE T DotProduct(const Vector3& other) const noexcept
        {
            return x * other.x + y * other.y + z * other.z;
        }

        inline result_t Distance(const Vector3& other) const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            const result_t dx = static_cast<result_t>(other.x) - x;
            const result_t dy = static_cast<result_t>(other.y) - y;
            const result_t dz = static_cast<result_t>(other.z) - z;
            return std::sqrt(dx*dx + dy*dy + dz*dz);
        }

        VECTOR_INLINE T DistanceSqr(const Vector3& other) const noexcept
        {
            const T dx = other.x - x;
            const T dy = other.y - y;
            const T dz = other.z - z;

            return dx * dx + dy * dy + dz * dz;
        }

        inline result_t Angle(const Vector3& other) const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            const Vector3 cross = CrossProduct(other);
            const result_t length = cross.Length();
            const result_t dot = DotProduct(other);

            return std::atan2(length, dot);
        }

        Vector3<result_t> Normalize() const noexcept
        {
            Vector3<result_t> result = *this;
            const result_t length = Length();

            if (length > 0)
            {
                const result_t ilength = static_cast<result_t>(1) / length;
                result.x = x * ilength;
                result.y = y * ilength;
                result.z = z * ilength;
            }

            return result;
        }

        Vector3& NormalizeInPlace() noexcept
        {
            *this = Normalize();
            return *this;
        }

        //Calculate the projection of this vector on to other
        VECTOR_INLINE Vector3<result_t> Project(const Vector3& other) const noexcept
        {
            const result_t v1dv2 = (x*other.x + y*other.y + *other.z);
            const result_t v2dv2 = (other.x*other.x + other.y*other.y + other.z*other.z);

            const result_t mag = v1dv2 / v2dv2;

            Vector3<result_t> result;
            result.x = x - (other.x*mag);
            result.y = y - (other.y*mag);
            result.z = z - (other.z*mag);
            return result;
        }

        VECTOR_INLINE Vector3& ProjectInPlace(const Vector3& other) noexcept
        {
            *this = Project(other);
            return *this;
        }

        // Orthonormalize provided vectors
        // Makes vectors normalized and orthogonal to each other
        // Gram-Schmidt function implementation
        inline std::pair<Vector3<result_t>, Vector3<result_t>> OrthoNormalize(const Vector3& other) const noexcept
        {
            const Vector3<result_t> newThis = this->Normalize();
            const Vector3<result_t> cross = newThis.CrossProduct(other).Normalize();
            const Vector3<result_t> newOther = cross.CrossProduct(newThis);

            return { newThis, newOther };
        }

        inline Vector3& OrthoNormalizeInPlace(Vector3* other) noexcept
        {
            const std::pair<Vector3<result_t>, Vector3<result_t>> vecs = OrthoNormalize(other);
            *this = vecs.first;
            *other = vecs.second;
            return *this;
        }

        // Rotates a vector around an axis
        Vector3<result_t> RotateByAxisAngle(const Vector3& axis, result_t angle) const noexcept
        {
            // Using Euler-Rodrigues Formula
            // Ref.: https://en.wikipedia.org/w/index.php?title=Euler%E2%80%93Rodrigues_formula

            const auto axisNorm = axis.Normalize();

            angle /= result_t(2);
            const result_t a = std::sin(angle);
            const result_t b = axisNorm.x*a;
            const result_t c = axisNorm.y*a;
            const result_t d = axisNorm.z*a;
            a = std::cos(angle);

            const Vector3<result_t> w(b, c, d);
            Vector3<result_t> wv = w.CrossProduct(*this);
            Vector3<result_t> wwv = w.CrossProduct(wv);
            wv = wv * (2 * a);
            wwv *= 2;

            Vector3<result_t> result(*this);
            result.x += wv.x;
            result.y += wv.y;
            result.z += wv.z;

            result.x += wwv.x;
            result.y += wwv.y;
            result.z += wwv.z;

            return result;
        }

        inline Vector3& RotateByAxisAngleInPlace(const Vector3& axis, result_t angle) noexcept
        {
            *this = RotateByAxisAngle(axis, angle);
            return *this;
        }

        // Move Vector towards target
        Vector3<result_t> MoveTowards(const Vector3& target, result_t maxDistance) const noexcept
        {
            const result_t dx = target.x - x;
            const result_t dy = target.y - y;
            const result_t dz = target.z - z;
            const result_t value = (dx*dx) + (dy*dy) + (dz*dz);

            if (value == 0 || ((maxDistance >= 0) && value <= maxDistance * maxDistance)) return target;

            Vector3<result_t> result;
            const result_t dist = std::sqrt(value);
            result.x = x + dx/dist*maxDistance;
            result.y = y + dy/dist*maxDistance;
            result.z = z + dz/dist*maxDistance;
            return result;
        }

        inline Vector3& MoveTowardsInPlace(const Vector3& target, result_t maxDistance) noexcept
        {
            *this = MoveTowards(target, maxDistance);
            return *this;
        }

        // Calculate linear interpolation between two vectors
        VECTOR_INLINE Vector3<result_t> Lerp(const Vector3& other, result_t amount) const noexcept
        {
            Vector3<result_t> result;

            result.x = x + amount * (other.x - x);
            result.y = y + amount * (other.y - y);
            result.z = z + amount * (other.z - z);
            return result;
        }

        VECTOR_INLINE Vector3& LerpInPlace(const Vector3& other, result_t amount) noexcept
        {
            *this = Lerp(other, amount);
            return *this;
        }

        // Calculate cubic hermite interpolation between two vectors and their tangents
        // as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic
        Vector3<result_t> CubicHermite(const Vector3& tangent1, const Vector3& v2, const Vector3& tangent2, result_t amount) const noexcept
        {
            const result_t amountPow2 = amount*amount;
            const result_t amountPow3 = amount*amount*amount;

            Vector3<result_t> result;
            result.x = (2*amountPow3 - 3*amountPow2 + 1)*x + (amountPow3 - 2*amountPow2 + amount)*tangent1.x + (-2*amountPow3 + 3*amountPow2)*v2.x + (amountPow3 - amountPow2)*tangent2.x;
            result.y = (2*amountPow3 - 3*amountPow2 + 1)*y + (amountPow3 - 2*amountPow2 + amount)*tangent1.y + (-2*amountPow3 + 3*amountPow2)*v2.y + (amountPow3 - amountPow2)*tangent2.y;
            result.z = (2*amountPow3 - 3*amountPow2 + 1)*z + (amountPow3 - 2*amountPow2 + amount)*tangent1.z + (-2*amountPow3 + 3*amountPow2)*v2.z + (amountPow3 - amountPow2)*tangent2.z;

            return result;
        }

        inline Vector3& CubicHermiteInPlace(const Vector3& tangent1, const Vector3& v2, const Vector3& tangent2, result_t amount) noexcept
        {
            *this = CubicHermite(tangent1, v2, tangent2, amount);
            return *this;
        }

        // Calculate reflected vector to normal
        VECTOR_INLINE Vector3 Reflect(const Vector3& normal) const noexcept
        {
            const T dot = DotProduct(normal);
            
            Vector3 result;
            result.x = x - (static_cast<T>(2) * normal.x) * dot;
            result.y = y - (static_cast<T>(2) * normal.y) * dot;
            result.z = z - (static_cast<T>(2) * normal.z) * dot;
            return result;
        }

        VECTOR_INLINE Vector3& ReflectInPlace(const Vector3& normal) noexcept
        {
            *this = Reflect(normal);
            return *this;
        }

        // Compute barycenter coordinates (u, v, w) for point this with respect to triangle (a, b, c)
        // NOTE: Assumes Point this is on the plane of the triangle
        Vector3<result_t> Barycenter(const Vector3& a, const Vector3& b, const Vector3& c) const noexcept
        {
            Vector3<result_t> v0 = { b.x - a.x, b.y - a.y, b.z - a.z };   // Vector3Subtract(b, a)
            Vector3<result_t> v1 = { c.x - a.x, c.y - a.y, c.z - a.z };   // Vector3Subtract(c, a)
            Vector3<result_t> v2 = { x - a.x, y - a.y, z - a.z };   // Vector3Subtract(p, a)
            const result_t d00 = (v0.x*v0.x + v0.y*v0.y + v0.z*v0.z);    // Vector3DotProduct(v0, v0)
            const result_t d01 = (v0.x*v1.x + v0.y*v1.y + v0.z*v1.z);    // Vector3DotProduct(v0, v1)
            const result_t d11 = (v1.x*v1.x + v1.y*v1.y + v1.z*v1.z);    // Vector3DotProduct(v1, v1)
            const result_t d20 = (v2.x*v0.x + v2.y*v0.y + v2.z*v0.z);    // Vector3DotProduct(v2, v0)
            const result_t d21 = (v2.x*v1.x + v2.y*v1.y + v2.z*v1.z);    // Vector3DotProduct(v2, v1)

            const result_t denom = d00*d11 - d01*d01;

            Vector3<result_t> result;
            result.y = (d11*d20 - d01*d21)/denom;
            result.z = (d00*d21 - d01*d20)/denom;
            result.x = result_t(1) - (result.z + result.y);

            return result;
        }

        inline Vector3& BarycenterInPlace(const Vector3& a, const Vector3& b, const Vector3& c) noexcept
        {
            *this = Barycenter(a, b, c);
            return *this;
        }

        VECTOR_INLINE Vector3<result_t> Invert() const noexcept
        {
            return Vector3<result_t>(result_t(1) / x, result_t(1) / y, result_t(1) / z);
        }

        VECTOR_INLINE Vector3& InvertInPlace() noexcept
        {
            *this = Invert();
            return *this;
        }

        VECTOR_INLINE Vector3 Clamp(const Vector3& min, const Vector3& max) const noexcept
        {
            Vector3 result;
            result.x = std::clamp(x, min.x, max.x);
            result.y = std::clamp(y, min.y, max.y);
            result.z = std::clamp(z, min.z, max.z);
            return result;
        }

        VECTOR_INLINE Vector3& ClampInPlace(const Vector3& min, const Vector3& max) noexcept
        {
            *this = Clamp(min, max);
            return *this;
        }

        Vector3<result_t> ClampMagnitude(result_t min, result_t max) const noexcept
        {
            Vector3<result_t> result(x, y);

            result_t length = LengthSqr();
            if (length > result_t(0))
            {
                length = std::sqrt(length);

                result_t scale = result_t(1);
                if (length < min)
                {
                    scale = min / length;
                }
                else if (length > max)
                {
                    scale = max / length;
                }

                result.x = x * scale;
                result.y = y * scale;
                result.z = z * scale;
            }
        
            return result;
        }

        Vector3& ClampMagnitudeInPlace(result_t min, result_t max) noexcept
        {
            *this = ClampMagnitude(min, max);
            return *this;
        }
    };

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector3<R> Min(const Vector3<T1>& v1, const Vector3<T2>& v2) noexcept
    {
        return Vector3<R>(
            std::min(static_cast<R>(v1.x), static_cast<R>(v2.x)),
            std::min(static_cast<R>(v1.y), static_cast<R>(v2.y)),
            std::min(static_cast<R>(v1.z), static_cast<R>(v2.z))
        );
    }

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector3<R> Max(const Vector3<T1>& v1, const Vector3<T2>& v2) noexcept
    {
        return Vector3<R>(
            std::max(static_cast<R>(v1.x), static_cast<R>(v2.x)),
            std::max(static_cast<R>(v1.y), static_cast<R>(v2.y)),
            std::max(static_cast<R>(v1.z), static_cast<R>(v2.z))
        );
    }

    using Vector3i8  = Vector3<std::int8_t>;
    using Vector3i16 = Vector3<std::int16_t>;
    using Vector3i32 = Vector3<std::int32_t>;
    using Vector3i64 = Vector3<std::int64_t>;

    using Vector3u8  = Vector3<std::uint8_t>;
    using Vector3u16 = Vector3<std::uint16_t>;
    using Vector3u32 = Vector3<std::uint32_t>;
    using Vector3u64 = Vector3<std::uint64_t>;
    
    using Vector3f  = Vector3<float>;
    using Vector3d  = Vector3<double>;
    using Vector3ld = Vector3<long double>;


    // ------------------------------------ Vector4 ------------------------------------
    template <typename T>
    class Vector4
    {
    private:
        // result_t type: double for ints otherwise float, double, long double
        using result_t = std::conditional_t<std::is_integral_v<T>, double, T>;
    private:
        static constexpr float FloatEpsilon() noexcept
        {
            return 0.000001f;
        }
    public:
        T x;
        T y;
        T z;
        T w;
    public:
        // Constructors
        VECTOR_INLINE Vector4() noexcept : x(static_cast<T>(0)), y(static_cast<T>(0)), z(static_cast<T>(0)), w(static_cast<T>(0)) {}
        VECTOR_INLINE Vector4(T d) noexcept : x(d), y(d), z(d), w(d) {}
        VECTOR_INLINE Vector4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}

        // Copy & Move
        inline Vector4(Vector4&&) = default;
        inline Vector4(const Vector4&) = default;
        inline Vector4& operator=(Vector4&&) = default;
        inline Vector4& operator=(const Vector4&) = default;

        // Conversion constructors
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector4(const Vector4<U>& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector4(Vector4<U>&& other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y)), z(static_cast<T>(other.z)), w(static_cast<T>(other.w)) {}

        // Assignment operators
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector4& operator=(Vector4<U>&& other) noexcept
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            z = static_cast<T>(other.z);
            w = static_cast<T>(other.w);
            return *this;
        }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector4& operator=(const Vector4<U>& other) noexcept
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            z = static_cast<T>(other.z);
            w = static_cast<T>(other.w);
            return *this;
        }

        // Vector operators
        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector4<R> operator+(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x + other.x, y + other.y, z + other.z, w + other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector4<R> operator-(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x - other.x, y - other.y, z - other.z, w - other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector4<R> operator*(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x * other.x, y * other.y, z * other.z, w * other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector4<R> operator/(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x / other.x, y / other.y, z / other.z, w / other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>>
        VECTOR_INLINE Vector4<R> operator%(const Vector4<U>& other) const noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector4<R>(std::fmod(x, other.x), std::fmod(y, other.y), std::fmod(z, other.z), std::fmod(w, other.w));
            else
                return Vector4<R>(x % other.x, y % other.y, z % other.z, w % other.w);
        }

        // Scalar operations
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4<R> operator+(U scalar) const noexcept
        {
            return Vector4<R>(x + scalar, y + scalar, z + scalar, w + scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4<R> operator-(U scalar) const noexcept
        {
            return Vector4<R>(x - scalar, y - scalar, z - scalar, w - scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4<R> operator*(U scalar) const noexcept
        {
            return Vector4<R>(x * scalar, y * scalar, z * scalar, w * scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4<R> operator/(U scalar) const noexcept
        {
            return Vector4<R>(x / scalar, y / scalar, z / scalar, w / scalar);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4<R> operator%(U scalar) const noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector4<R>(std::fmod(x, scalar), std::fmod(y, scalar), std::fmod(z, scalar), std::fmod(w, scalar));
            else
                return Vector4<R>(x % scalar, y % scalar, z % scalar, w % scalar);
        }

        // Scalar on left-hand side
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator+(U scalar, const Vector4& v) noexcept
        {
            return Vector4<R>(scalar + v.x, scalar + v.y, scalar + v.z, scalar + v.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator-(U scalar, const Vector4& v) noexcept
        {
            return Vector4<R>(scalar - v.x, scalar - v.y, scalar - v.z, scalar - v.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator*(U scalar, const Vector4& v) noexcept
        {
            return Vector4<R>(scalar * v.x, scalar * v.y, scalar * v.z, scalar * v.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator/(U scalar, const Vector4& v) noexcept
        {
            return Vector4<R>(scalar / v.x, scalar / v.y, scalar / v.z, scalar / v.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_arithmetic_v<U>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator%(U scalar, const Vector4& v) noexcept
        {
            if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>)
                return Vector4<R>(std::fmod(scalar, v.x), std::fmod(scalar, v.y), std::fmod(scalar, v.z), std::fmod(scalar, v.w));
            else
                return Vector4<R>(scalar % v.x, scalar % v.y, scalar % v.z, scalar % v.w);
        }

        // Compound assignment for Vector4
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector4& operator+=(const Vector4<U>& other) noexcept
        {
            x += static_cast<T>(other.x);
            y += static_cast<T>(other.y);
            z += static_cast<T>(other.z);
            w += static_cast<T>(other.w);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector4& operator-=(const Vector4<U>& other) noexcept
        {
            x -= static_cast<T>(other.x);
            y -= static_cast<T>(other.y);
            z -= static_cast<T>(other.z);
            w -= static_cast<T>(other.w);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector4& operator*=(const Vector4<U>& other) noexcept
        {
            x *= static_cast<T>(other.x);
            y *= static_cast<T>(other.y);
            z *= static_cast<T>(other.z);
            w *= static_cast<T>(other.w);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector4& operator/=(const Vector4<U>& other) noexcept
        {
            x /= static_cast<T>(other.x);
            y /= static_cast<T>(other.y);
            z /= static_cast<T>(other.z);
            w /= static_cast<T>(other.w);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE Vector4& operator%=(const Vector4<U>& other) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                x = std::fmod(x, static_cast<T>(other.x));
                y = std::fmod(y, static_cast<T>(other.y));
                z = std::fmod(z, static_cast<T>(other.z));
                w = std::fmod(w, static_cast<T>(other.w));
            }
            else
            {
                x %= static_cast<T>(other.x);
                y %= static_cast<T>(other.y);
                z %= static_cast<T>(other.z);
                w %= static_cast<T>(other.w);
            }
            return *this;
        }

        // Scalar compound assignment
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4& operator+=(U scalar) noexcept
        {
            x += static_cast<T>(scalar);
            y += static_cast<T>(scalar);
            z += static_cast<T>(scalar);
            w += static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4& operator-=(U scalar) noexcept
        {
            x -= static_cast<T>(scalar);
            y -= static_cast<T>(scalar);
            z -= static_cast<T>(scalar);
            w -= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4& operator*=(U scalar) noexcept
        {
            x *= static_cast<T>(scalar);
            y *= static_cast<T>(scalar);
            z *= static_cast<T>(scalar);
            w *= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4& operator/=(U scalar) noexcept
        {
            x /= static_cast<T>(scalar);
            y /= static_cast<T>(scalar);
            z /= static_cast<T>(scalar);
            w /= static_cast<T>(scalar);
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T> && std::is_arithmetic_v<U>, int> = 0>
        VECTOR_INLINE Vector4& operator%=(U scalar) noexcept
        {
            if constexpr (std::is_floating_point_v<T>)
            {
                x = std::fmod(x, static_cast<T>(scalar));
                y = std::fmod(y, static_cast<T>(scalar));
                z = std::fmod(z, static_cast<T>(scalar));
                w = std::fmod(w, static_cast<T>(scalar));
            }
            else
            {
                x %= static_cast<T>(scalar);
                y %= static_cast<T>(scalar);
                z %= static_cast<T>(scalar);
                w %= static_cast<T>(scalar);
            }
            return *this;
        }

        // Unary operators
        VECTOR_INLINE Vector4 operator-() const noexcept
        {
            return Vector4(-x, -y, -z, -w);
        }

        VECTOR_INLINE Vector4 operator+() const noexcept
        {
            return *this;
        }

        // Checks if x, y, z, and w are zero
        VECTOR_INLINE explicit operator bool() const noexcept
        {
            return x != T(0) || y != T(0) || z != T(0) || w != T(0);
        }

        // Comparison
        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator==(const Vector4<U>& other) const noexcept
        {
            const Vector4 cmp = other;

            if constexpr (std::is_floating_point_v<T>)
            {
                return std::fabs(x - cmp.x) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(x), std::fabs(cmp.x))) &&
                    std::fabs(y - cmp.y) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(y), std::fabs(cmp.y))) &&
                    std::fabs(z - cmp.z) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(z), std::fabs(cmp.z))) &&
                    std::fabs(w - cmp.w) <= FloatEpsilon() * std::max(T(1), std::max(std::fabs(w), std::fabs(cmp.w)));
            }
            else
            {
                return x == cmp.x && y == cmp.y && z == cmp.z && w == cmp.w;
            }
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator!=(const Vector4<U>& other) const noexcept
        {
            return !(*this == other);
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator<(const Vector4<U>& other) const noexcept
        {
            return LengthSqr() < other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator<=(const Vector4<U>& other) const noexcept
        {
            return LengthSqr() <= other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator>(const Vector4<U>& other) const noexcept
        {
            return LengthSqr() > other.LengthSqr();
        }

        template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        VECTOR_INLINE bool operator>=(const Vector4<U>& other) const noexcept
        {
            return LengthSqr() >= other.LengthSqr();
        }

        // Bitwise AND
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4<R> operator&(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x & other.x, y & other.y, z & other.z, w & other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4<R> operator&(U other) const noexcept
        {
            return Vector4<R>(x & other, y & other, z & other, w & other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator&(U other, const Vector4& v) noexcept
        {
            return Vector4<R>(other & v.x, other & v.y, other & v.z, other & v.w);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4& operator&=(const Vector4<U>& other) noexcept
        {
            x &= other.x;
            y &= other.y;
            z &= other.z;
            w &= other.w;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4& operator&=(U other) noexcept
        {
            x &= other;
            y &= other;
            z &= other;
            w &= other;
            return *this;
        }

        // Bitwise OR
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4<R> operator|(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x | other.x, y | other.y, z | other.z, w | other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4<R> operator|(U other) const noexcept
        {
            return Vector4<R>(x | other, y | other, z | other, w | other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator|(U other, const Vector4& v) noexcept
        {
            return Vector4<R>(other | v.x, other | v.y, other | v.z, other | v.w);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4& operator|=(const Vector4<U>& other) noexcept
        {
            x |= other.x;
            y |= other.y;
            z |= other.z;
            w |= other.w;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4& operator|=(U other) noexcept
        {
            x |= other;
            y |= other;
            z |= other;
            w |= other;
            return *this;
        }

        // Bitwise XOR
        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4<R> operator^(const Vector4<U>& other) const noexcept
        {
            return Vector4<R>(x ^ other.x, y ^ other.y, z ^ other.z, w ^ other.w);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4<R> operator^(U other) const noexcept
        {
            return Vector4<R>(x ^ other, y ^ other, z ^ other, w ^ other);
        }

        template <typename U, typename R = std::common_type_t<T, U>, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        friend VECTOR_INLINE Vector4<R> operator^(U other, const Vector4& v) noexcept
        {
            return Vector4<R>(other ^ v.x, other ^ v.y, other ^ v.z, other ^ v.w);
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4& operator^=(const Vector4<U>& other) noexcept
        {
            x ^= other.x;
            y ^= other.y;
            z ^= other.z;
            w ^= other.w;
            return *this;
        }

        template <typename U, std::enable_if_t<std::is_integral_v<U> && std::is_integral_v<T>, int> = 0>
        VECTOR_INLINE Vector4& operator^=(U other) noexcept
        {
            x ^= other;
            y ^= other;
            z ^= other;
            w ^= other;
            return *this;
        }

        // Bitwise NOT
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector4 operator~() const noexcept
        {
            return Vector4(~x, ~y, ~z, ~w);
        }

        // Left shift
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector4 operator<<(int shift) const noexcept
        {
            return Vector4(x << shift, y << shift, z << shift, w << shift);
        }

        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector4& operator<<=(int shift) noexcept
        {
            x <<= shift;
            y <<= shift;
            z <<= shift;
            w <<= shift;
            return *this;
        }

        // Right shift
        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector4 operator>>(int shift) const noexcept
        {
            return Vector4(x >> shift, y >> shift, z >> shift, w >> shift);
        }

        template <typename D = T, std::enable_if_t<std::is_integral_v<D>, int> = 0>
        VECTOR_INLINE Vector4& operator>>=(int shift) noexcept
        {
            x >>= shift;
            y >>= shift;
            z >>= shift;
            w >>= shift;
            return *this;
        }

        // Output stream
        inline friend std::ostream& operator<<(std::ostream& os, const Vector4& v)
        {
            return os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        }

        // Utility
        template <typename U, std::enable_if_t<std::is_convertible_v<T, U>, int> = 0>
        VECTOR_INLINE Vector4<U> Convert() const noexcept
        {
            return Vector4<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w));
        }

        inline result_t Length() const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            return std::sqrt((static_cast<result_t>(x)*x) + (static_cast<result_t>(y)*y) + (static_cast<result_t>(z)*z) + (static_cast<result_t>(w)*w));
        }

        VECTOR_INLINE T LengthSqr() const noexcept
        {
            return (x*x) + (y*y) + (z*z) + (w*w);
        }

        VECTOR_INLINE T DotProduct(const Vector4& other) const noexcept
        {
            return x * other.x + y * other.y + z * other.z + w * other.w;
        }

        inline result_t Distance(const Vector4& other) const noexcept
        {
            // Return type: double for ints otherwise float, double, long double
            const result_t dx = static_cast<result_t>(other.x) - x;
            const result_t dy = static_cast<result_t>(other.y) - y;
            const result_t dz = static_cast<result_t>(other.z) - z;
            const result_t dw = static_cast<result_t>(other.w) - w;
            return std::sqrt(dx*dx + dy*dy + dz*dz + dw*dw);
        }

        VECTOR_INLINE T DistanceSqr(const Vector4& other) const noexcept
        {
            const T dx = other.x - x;
            const T dy = other.y - y;
            const T dz = other.z - z;
            const T dw = other.w - w;

            return dx * dx + dy * dy + dz * dz + dw * dw;
        }

        Vector4<result_t> Normalize() const noexcept
        {
            Vector4<result_t> result = *this;
            const result_t length = Length();

            if (length > 0)
            {
                const result_t ilength = static_cast<result_t>(1) / length;
                result.x = x * ilength;
                result.y = y * ilength;
                result.z = z * ilength;
                result.w = w * ilength;
            }

            return result;
        }

        Vector4& NormalizeInPlace() noexcept
        {
            *this = Normalize();
            return *this;
        }

        // Move Vector towards target
        Vector4<result_t> MoveTowards(const Vector4& target, result_t maxDistance) const noexcept
        {
            const result_t dx = target.x - x;
            const result_t dy = target.y - y;
            const result_t dz = target.z - z;
            const result_t dw = target.w - w;
            const result_t value = (dx*dx) + (dy*dy) + (dz*dz) + (dw*dw);

            if (value == 0 || ((maxDistance >= 0) && value <= maxDistance * maxDistance)) return target;

            Vector4<result_t> result;
            const result_t dist = std::sqrt(value);
            result.x = x + dx/dist*maxDistance;
            result.y = y + dy/dist*maxDistance;
            result.z = z + dz/dist*maxDistance;
            result.w = w + dw/dist*maxDistance;
            return result;
        }

        inline Vector4& MoveTowardsInPlace(const Vector4& target, result_t maxDistance) noexcept
        {
            *this = MoveTowards(target, maxDistance);
            return *this;
        }

        // Calculate linear interpolation between two vectors
        VECTOR_INLINE Vector4<result_t> Lerp(const Vector4& other, result_t amount) const noexcept
        {
            Vector4<result_t> result;

            result.x = x + amount * (other.x - x);
            result.y = y + amount * (other.y - y);
            result.z = z + amount * (other.z - z);
            result.w = w + amount * (other.w - w);
            return result;
        }

        VECTOR_INLINE Vector4& LerpInPlace(const Vector4& other, result_t amount) noexcept
        {
            *this = Lerp(other, amount);
            return *this;
        }

        VECTOR_INLINE Vector4<result_t> Invert() const noexcept
        {
            return Vector4<result_t>(result_t(1) / x, result_t(1) / y, result_t(1) / z, result_t(1) / w);
        }

        VECTOR_INLINE Vector4& InvertInPlace() noexcept
        {
            *this = Invert();
            return *this;
        }

        VECTOR_INLINE Vector4 Clamp(const Vector4& min, const Vector4& max) const noexcept
        {
            Vector4 result;
            result.x = std::clamp(x, min.x, max.x);
            result.y = std::clamp(y, min.y, max.y);
            result.z = std::clamp(z, min.z, max.z);
            result.w = std::clamp(w, min.w, max.w);
            return result;
        }

        VECTOR_INLINE Vector4& ClampInPlace(const Vector4& min, const Vector4& max) noexcept
        {
            *this = Clamp(min, max);
            return *this;
        }

        Vector4<result_t> ClampMagnitude(result_t min, result_t max) const noexcept
        {
            Vector4<result_t> result(x, y);

            result_t length = LengthSqr();
            if (length > result_t(0))
            {
                length = std::sqrt(length);

                result_t scale = result_t(1);
                if (length < min)
                {
                    scale = min / length;
                }
                else if (length > max)
                {
                    scale = max / length;
                }

                result.x = x * scale;
                result.y = y * scale;
                result.z = z * scale;
                result.w = w * scale;
            }
        
            return result;
        }

        Vector4& ClampMagnitudeInPlace(result_t min, result_t max) noexcept
        {
            *this = ClampMagnitude(min, max);
            return *this;
        }
    };

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector4<R> Min(const Vector4<T1>& v1, const Vector4<T2>& v2) noexcept
    {
        return Vector4<R>(
            std::min(static_cast<R>(v1.x), static_cast<R>(v2.x)),
            std::min(static_cast<R>(v1.y), static_cast<R>(v2.y)),
            std::min(static_cast<R>(v1.z), static_cast<R>(v2.z)),
            std::min(static_cast<R>(v1.w), static_cast<R>(v2.w))
        );
    }

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector4<R> Max(const Vector4<T1>& v1, const Vector4<T2>& v2) noexcept
    {
        return Vector4<R>(
            std::max(static_cast<R>(v1.x), static_cast<R>(v2.x)),
            std::max(static_cast<R>(v1.y), static_cast<R>(v2.y)),
            std::max(static_cast<R>(v1.z), static_cast<R>(v2.z)),
            std::max(static_cast<R>(v1.w), static_cast<R>(v2.w))
        );
    }

    using Vector4i8  = Vector4<std::int8_t>;
    using Vector4i16 = Vector4<std::int16_t>;
    using Vector4i32 = Vector4<std::int32_t>;
    using Vector4i64 = Vector4<std::int64_t>;

    using Vector4u8  = Vector4<std::uint8_t>;
    using Vector4u16 = Vector4<std::uint16_t>;
    using Vector4u32 = Vector4<std::uint32_t>;
    using Vector4u64 = Vector4<std::uint64_t>;
    
    using Vector4f  = Vector4<float>;
    using Vector4d  = Vector4<double>;
    using Vector4ld = Vector4<long double>;

} // VECTOR_NAMESPACE

namespace std
{
    template <typename T>
    struct hash<VECTOR_NAMESPACE::Vector2<T>>
    {
        std::size_t operator()(const VECTOR_NAMESPACE::Vector2<T>& v) const noexcept
        {
            std::size_t h1 = std::hash<T>{}(v.x);
            std::size_t h2 = std::hash<T>{}(v.y);

            std::size_t seed = h1;
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };

    template <typename T>
    struct hash<VECTOR_NAMESPACE::Vector3<T>>
    {
        std::size_t operator()(const VECTOR_NAMESPACE::Vector3<T>& v) const noexcept
        {
            std::size_t h1 = std::hash<T>{}(v.x);
            std::size_t h2 = std::hash<T>{}(v.y);
            std::size_t h3 = std::hash<T>{}(v.z);

            std::size_t seed = h1;
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };

    template <typename T>
    struct hash<VECTOR_NAMESPACE::Vector4<T>>
    {
        std::size_t operator()(const VECTOR_NAMESPACE::Vector4<T>& v) const noexcept
        {
            std::size_t h1 = std::hash<T>{}(v.x);
            std::size_t h2 = std::hash<T>{}(v.y);
            std::size_t h3 = std::hash<T>{}(v.z);
            std::size_t h4 = std::hash<T>{}(v.w);

            std::size_t seed = h1;
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

#undef VECTOR_NAMESPACE
#undef VECTOR_INLINE
#endif // VECTOR_H