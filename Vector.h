#ifndef VECTOR_H
#define VECTOR_H

#include <cmath>
#include <iostream>
#include <algorithm>
#include <type_traits>

#define VECTOR_NOEXCEPT noexcept
#define VECTOR_INLINE constexpr

namespace Vec
{
    template <typename T>
    class Vector2
    {
    private:
        // result_t type: double for ints otherwise float, double, long double
        using result_t = std::conditional_t<std::is_integral_v<T>, double, T>;
    public:
        T x;
        T y;
    public:
        VECTOR_INLINE Vector2() VECTOR_NOEXCEPT : x(static_cast<T>(0)), y(static_cast<T>(0)) {}
        VECTOR_INLINE Vector2(T d) VECTOR_NOEXCEPT : x(d), y(d) {}
        VECTOR_INLINE Vector2(T x, T y) VECTOR_NOEXCEPT : x(x), y(y) {}

        inline Vector2(const Vector2&) = default;
        inline Vector2& operator=(const Vector2&) = default;
        inline Vector2(Vector2&& other) = default;
        inline Vector2& operator=(Vector2&& other) = default;

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
        VECTOR_INLINE Vector2& operator=(const Vector2<U>& other) VECTOR_NOEXCEPT
        {
            x = static_cast<T>(other.x);
            y = static_cast<T>(other.y);
            return *this;
        }

        // Vector operators
        VECTOR_INLINE Vector2 operator+(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x + other.x, y + other.y);
        }

        VECTOR_INLINE Vector2 operator-(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x - other.x, y - other.y);
        }

        VECTOR_INLINE Vector2 operator*(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x * other.x, y * other.y);
        }

        VECTOR_INLINE Vector2 operator/(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x / other.x, y / other.y);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator%(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x % other.x, y % other.y);
        }

        // Scalar operations
        VECTOR_INLINE Vector2 operator+(T scalar) const VECTOR_NOEXCEPT
        {
            return Vector2(x + scalar, y + scalar);
        }

        VECTOR_INLINE Vector2 operator-(T scalar) const VECTOR_NOEXCEPT
        {
            return Vector2(x - scalar, y - scalar);
        }

        VECTOR_INLINE Vector2 operator*(T scalar) const VECTOR_NOEXCEPT
        {
            return Vector2(x * scalar, y * scalar);
        }

        VECTOR_INLINE Vector2 operator/(T scalar) const VECTOR_NOEXCEPT
        {
            return Vector2(x / scalar, y / scalar);
        }
        
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator%(T scalar) const VECTOR_NOEXCEPT
        {
            return Vector2(x % scalar, y % scalar);
        }

        // Compound assignment
        VECTOR_INLINE Vector2& operator+=(const Vector2& other) VECTOR_NOEXCEPT
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        VECTOR_INLINE Vector2& operator-=(const Vector2& other) VECTOR_NOEXCEPT
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        VECTOR_INLINE Vector2& operator*=(const Vector2& other) VECTOR_NOEXCEPT
        {
            x *= other.x;
            y *= other.y;
            return *this;
        }

        VECTOR_INLINE Vector2& operator/=(const Vector2& other) VECTOR_NOEXCEPT
        {
            x /= other.x;
            y /= other.y;
            return *this;
        }
        
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2& operator%=(const Vector2& other) VECTOR_NOEXCEPT
        {
            x %= other.x;
            y %= other.y;
            return *this;
        }

        VECTOR_INLINE Vector2& operator+=(T scalar) VECTOR_NOEXCEPT
        {
            x += scalar;
            y += scalar;
            return *this;
        }

        VECTOR_INLINE Vector2& operator-=(T scalar) VECTOR_NOEXCEPT
        {
            x -= scalar;
            y -= scalar;
            return *this;
        }

        VECTOR_INLINE Vector2& operator*=(T scalar) VECTOR_NOEXCEPT
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        VECTOR_INLINE Vector2& operator/=(T scalar) VECTOR_NOEXCEPT
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2& operator%=(T scalar) VECTOR_NOEXCEPT
        {
            x %= scalar;
            y %= scalar;
            return *this;
        }

        // Unary operators
        VECTOR_INLINE Vector2 operator-() const VECTOR_NOEXCEPT
        {
            return Vector2(-x, -y);
        }

        VECTOR_INLINE Vector2 operator+() const VECTOR_NOEXCEPT
        {
            return *this;
        }

        // Comparison
        VECTOR_INLINE bool operator==(const Vector2& other) const VECTOR_NOEXCEPT
        {
            #define FLOAT_EPSILON 0.000001f
            if constexpr (std::is_floating_point_v<T>)
            {
                return std::fabs(x - other.x) <= FLOAT_EPSILON * std::max(T(1), std::max(std::fabs(x), std::fabs(other.x))) &&
                       std::fabs(y - other.y) <= FLOAT_EPSILON * std::max(T(1), std::max(std::fabs(y), std::fabs(other.y)));
            }
            else
            {
                return x == other.x && y == other.y;
            }
            #undef FLOAT_EPSILON
        }

        VECTOR_INLINE bool operator!=(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return !(*this == other);
        }

        // Bitwise AND
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator&(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x & other.x, y & other.y);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator&(T other) const VECTOR_NOEXCEPT
        {
            return Vector2(x & other, y & other);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE friend Vector2 operator&(T other, const Vector2& v) VECTOR_NOEXCEPT
        {
            return Vector2(v.x & other, v.y & other);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator&=(const Vector2& other) const VECTOR_NOEXCEPT
        {
            x &= other.x;
            y &= other.y;
            return *this;
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator&=(T other) const VECTOR_NOEXCEPT
        {
            x &= other;
            y &= other;
            return *this;
        }

        // Bitwise OR
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator|(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x | other.x, y | other.y);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator|(T other) const VECTOR_NOEXCEPT
        {
            return Vector2(x | other, y | other);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE friend Vector2 operator|(T other, const Vector2& v) VECTOR_NOEXCEPT
        {
            return Vector2(v.x | other, v.y | other);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator|=(const Vector2& other) const VECTOR_NOEXCEPT
        {
            x |= other.x;
            y |= other.y;
            return *this;
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator|=(T other) const VECTOR_NOEXCEPT
        {
            x |= other;
            y |= other;
            return *this;
        }

        // Bitwise XOR
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator^(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return Vector2(x ^ other.x, y ^ other.y);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator^(T other) const VECTOR_NOEXCEPT
        {
            return Vector2(x ^ other, y ^ other);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE friend Vector2 operator^(T other, const Vector2& v) VECTOR_NOEXCEPT
        {
            return Vector2(v.x ^ other, v.y ^ other);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator^=(const Vector2& other) const VECTOR_NOEXCEPT
        {
            x ^= other.x;
            y ^= other.y;
            return *this;
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator^=(T other) const VECTOR_NOEXCEPT
        {
            x ^= other;
            y ^= other;
            return *this;
        }

        // Bitwise NOT
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator~() const VECTOR_NOEXCEPT
        {
            return Vector2(~x, ~y);
        }

        // Shift left
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator<<(int shift) const VECTOR_NOEXCEPT
        {
            return Vector2(x << shift, y << shift);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator<<=(int shift) const VECTOR_NOEXCEPT
        {
            x <<= shift;
            y <<= shift;
            return *this;
        }

        // Shift right
        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator>>(int shift) const VECTOR_NOEXCEPT
        {
            return Vector2(x >> shift, y >> shift);
        }

        template <typename D = T, typename = typename std::enable_if<std::is_integral<D>::value>::type>
        VECTOR_INLINE Vector2 operator>>=(int shift) const VECTOR_NOEXCEPT
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

        inline result_t Length() const VECTOR_NOEXCEPT
        {
            // Return type: double for ints otherwise float, double, long double
            return std::sqrt((x*x) + (y*y));
        }

        VECTOR_INLINE T LengthSqr() const VECTOR_NOEXCEPT
        {
            return (x*x) + (y*y);
        }

        VECTOR_INLINE T DotProduct(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return x * other.x + y * other.y;
        }

        inline result_t Distance(const Vector2& other) const VECTOR_NOEXCEPT
        {
            // Return type: double for ints otherwise float, double, long double
            return std::sqrt(DistanceSqr(other));
        }

        VECTOR_INLINE T DistanceSqr(const Vector2& other) const VECTOR_NOEXCEPT
        {
            return (x - other.x) * (x - other.x) + (y - other.y) * (y - other.y);
        }

        // Calculate angle between two vectors in radians
        // NOTE: Angle is calculated from origin point (0, 0)
        inline result_t Angle(const Vector2& other) const VECTOR_NOEXCEPT
        {
            // Return type: double for ints otherwise float, double, long double
            const T dot = DotProduct(other);
            const T det = x*other.y - y*other.x;
            return std::atan2(det, dot);
        }

        Vector2<result_t> Normalize() const VECTOR_NOEXCEPT
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

        Vector2& NormalizeInPlace() VECTOR_NOEXCEPT
        {
            *this = Normalize();
            return *this;
        }

        // Calculate linear interpolation between two vectors
        VECTOR_INLINE Vector2<result_t> Lerp(const Vector2& other, result_t amount) const VECTOR_NOEXCEPT
        {
            Vector2<result_t> result;

            result.x = x + amount * (other.x - x);
            result.y = y + amount * (other.y - y);
            return result;
        }

        // Calculate reflected vector to normal
        VECTOR_INLINE Vector2 Reflect(const Vector2& normal) const VECTOR_NOEXCEPT
        {
            const T dot = DotProduct(normal);
            
            Vector2 result;
            result.x = x - (static_cast<T>(2) * normal.x) * dot;
            result.y = y - (static_cast<T>(2) * normal.y) * dot;
            return result;
        }

        VECTOR_INLINE Vector2& ReflectInPlace(const Vector2& normal) VECTOR_NOEXCEPT
        {
            *this = Reflect(normal);
            return *this;
        }

        // Rotate vector by angle in radians
        Vector2<result_t> Rotate(result_t angle) const VECTOR_NOEXCEPT
        {
            const result_t cosres = std::cos(angle);
            const result_t sinres = std::sin(angle);
            
            Vector2<result_t> result;
            result.x = x*cosres - y*sinres;
            result.y = x*sinres + y*cosres;
            return result;
        }

        Vector2& RotateInPlace(result_t angle) VECTOR_NOEXCEPT
        {
            *this = Rotate(angle);
            return *this;
        }

        // Move Vector towards target
        Vector2<result_t> MoveTowards(const Vector2& target, result_t maxDistance) const VECTOR_NOEXCEPT
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

        Vector2& MoveTowardsInPlace(const Vector2& target, result_t maxDistance) VECTOR_NOEXCEPT
        {
            *this = MoveTowards(target, maxDistance);
            return *this;
        }

        VECTOR_INLINE Vector2<result_t> Invert() const VECTOR_NOEXCEPT
        {
            return Vector2<result_t>(result_t(1) / x, result_t(1) / y);
        }

        VECTOR_INLINE Vector2& InvertInPlace() VECTOR_NOEXCEPT
        {
            *this = Invert();
            return *this;
        }

        VECTOR_INLINE Vector2 Clamp(const Vector2& min, const Vector2& max) const VECTOR_NOEXCEPT
        {
            Vector2 result;
            result.x = std::clamp(x, min.x, max.x);
            result.y = std::clamp(y, min.y, max.y);
            return result;
        }

        VECTOR_INLINE Vector2& ClampInPlace(const Vector2& min, const Vector2& max) VECTOR_NOEXCEPT
        {
            *this = Clamp(min, max);
            return *this;
        }

        Vector2<result_t> ClampMagnitude(result_t min, result_t max) const VECTOR_NOEXCEPT
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

        Vector2& ClampMagnitudeInPlace(result_t min, result_t max) VECTOR_NOEXCEPT
        {
            *this = ClampMagnitude(min, max);
            return *this;
        }
    };

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector2<R> Min(const Vector2<T1>& v1, const Vector2<T2>& v2) VECTOR_NOEXCEPT
    {
        return Vector2<R>(std::min<R>(v1.x, v2.x), std::min<R>(v1.y, v2.y));
    }

    template <typename T1, typename T2, typename R = std::common_type_t<T1, T2>>
    VECTOR_INLINE Vector2<R> Max(const Vector2<T1>& v1, const Vector2<T2>& v2) VECTOR_NOEXCEPT
    {
        return Vector2<R>(std::max<R>(v1.x, v2.x), std::max<R>(v1.y, v2.y));
    }


    // Vector3 -----------------------------------------------------------------------
}

#undef VECTOR_NOEXCEPT
#undef VECTOR_INLINE
#endif // VECTOR_H