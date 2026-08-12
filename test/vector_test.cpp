#include "../Test.h"
#include "../Vector.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace
{
    using namespace Vec;

    constexpr long double Tolerance = 1.0e-5L;

    bool Near(long double left, long double right, long double tolerance = Tolerance)
    {
        return std::fabs(left - right) <= tolerance;
    }

    template <typename T>
    bool ComponentsNear(const Vector2<T>& value, std::array<long double, 2> expected,
                        long double tolerance = Tolerance)
    {
        return Near(static_cast<long double>(value.x), expected[0], tolerance) &&
               Near(static_cast<long double>(value.y), expected[1], tolerance);
    }

    template <typename T>
    bool ComponentsNear(const Vector3<T>& value, std::array<long double, 3> expected,
                        long double tolerance = Tolerance)
    {
        return Near(static_cast<long double>(value.x), expected[0], tolerance) &&
               Near(static_cast<long double>(value.y), expected[1], tolerance) &&
               Near(static_cast<long double>(value.z), expected[2], tolerance);
    }

    template <typename T>
    bool ComponentsNear(const Vector4<T>& value, std::array<long double, 4> expected,
                        long double tolerance = Tolerance)
    {
        return Near(static_cast<long double>(value.x), expected[0], tolerance) &&
               Near(static_cast<long double>(value.y), expected[1], tolerance) &&
               Near(static_cast<long double>(value.z), expected[2], tolerance) &&
               Near(static_cast<long double>(value.w), expected[3], tolerance);
    }

    template <class V, class = void>
    struct HasBitwiseAnd : std::false_type
    {
    };

    template <class V>
    struct HasBitwiseAnd<V, std::void_t<decltype(std::declval<V>() & std::declval<V>())>>
        : std::true_type
    {
    };

    template <class V, class = void>
    struct HasShiftLeft : std::false_type
    {
    };

    template <class V>
    struct HasShiftLeft<V, std::void_t<decltype(std::declval<V>() << 1)>> : std::true_type
    {
    };

    template <class V, class = void>
    struct HasModulo : std::false_type
    {
    };

    template <class V>
    struct HasModulo<V, std::void_t<decltype(std::declval<V>() % std::declval<V>())>>
        : std::true_type
    {
    };

    template <class V, class U, class = void>
    struct HasScalarAddition : std::false_type
    {
    };

    template <class V, class U>
    struct HasScalarAddition<V, U, std::void_t<decltype(std::declval<V>() + std::declval<U>())>>
        : std::true_type
    {
    };

    struct NotArithmetic
    {
    };

    struct EqProbe
    {
        constexpr EqProbe(int input = 0) noexcept : value(input) {}

        friend bool operator==(const EqProbe& left, const EqProbe& right) noexcept
        {
            ++calls;
            return left.value == right.value;
        }

        int value;
        static int calls;
    };

    int EqProbe::calls = 0;

    static_assert(std::is_standard_layout_v<Vector2<int>>);
    static_assert(std::is_standard_layout_v<Vector3<double>>);
    static_assert(std::is_standard_layout_v<Vector4<float>>);
    static_assert(std::is_trivially_copyable_v<Vector2<int>>);
    static_assert(std::is_trivially_copyable_v<Vector3<int>>);
    static_assert(std::is_trivially_copyable_v<Vector4<int>>);
    static_assert(std::is_constructible_v<Vector2<double>, const Vector2<int>&>);
    static_assert(std::is_constructible_v<Vector3<double>, Vector3<int>&&>);
    static_assert(std::is_constructible_v<Vector4<int>, const Vector4<double>&>);
    static_assert(!std::is_constructible_v<Vector2<int>, NotArithmetic>);
    static_assert(!std::is_constructible_v<Vector3<int>, int, int>);
    static_assert(!std::is_constructible_v<Vector4<int>, int, int, int>);
    static_assert(std::is_same_v<decltype(Vector2<int>() + Vector2<double>()), Vector2<double>>);
    static_assert(std::is_same_v<decltype(Vector3<float>() * 2.0), Vector3<double>>);
    static_assert(std::is_same_v<decltype(2.0 + Vector4<float>()), Vector4<double>>);
    static_assert(std::is_same_v<decltype(Vector2<int>().Length()), double>);
    static_assert(std::is_same_v<decltype(Vector3<float>().LengthSqr()), float>);
    static_assert(std::is_same_v<decltype(Vector4<long double>().Normalize()), Vector4<long double>>);
    static_assert(HasBitwiseAnd<Vector2<int>>::value);
    static_assert(HasBitwiseAnd<Vector3<unsigned>>::value);
    static_assert(HasBitwiseAnd<Vector4<long>>::value);
    static_assert(!HasBitwiseAnd<Vector2<float>>::value);
    static_assert(!HasBitwiseAnd<Vector3<double>>::value);
    static_assert(!HasBitwiseAnd<Vector4<long double>>::value);
    static_assert(HasShiftLeft<Vector2<int>>::value);
    static_assert(!HasShiftLeft<Vector2<float>>::value);
    static_assert(HasModulo<Vector2<float>>::value);
    static_assert(HasModulo<Vector3<double>>::value);
    static_assert(HasModulo<Vector4<int>>::value);
    static_assert(HasScalarAddition<Vector2<int>, short>::value);
    static_assert(!HasScalarAddition<Vector2<int>, NotArithmetic>::value);
    static_assert(noexcept(Vector2<int>(1, 2) / 0));
    static_assert(noexcept(Vector3<int>(1, 2, 3) << -1));
    static_assert(noexcept(Vector4<double>().Clamp(Vector4<double>(), Vector4<double>())));

    static_assert(std::is_same_v<Vector2i8, Vector2<std::int8_t>>);
    static_assert(std::is_same_v<Vector2i16, Vector2<std::int16_t>>);
    static_assert(std::is_same_v<Vector2i32, Vector2<std::int32_t>>);
    static_assert(std::is_same_v<Vector2i64, Vector2<std::int64_t>>);
    static_assert(std::is_same_v<Vector2u8, Vector2<std::uint8_t>>);
    static_assert(std::is_same_v<Vector2u16, Vector2<std::uint16_t>>);
    static_assert(std::is_same_v<Vector2u32, Vector2<std::uint32_t>>);
    static_assert(std::is_same_v<Vector2u64, Vector2<std::uint64_t>>);
    static_assert(std::is_same_v<Vector2f, Vector2<float>>);
    static_assert(std::is_same_v<Vector2d, Vector2<double>>);
    static_assert(std::is_same_v<Vector2ld, Vector2<long double>>);
    static_assert(std::is_same_v<Vector3i8, Vector3<std::int8_t>>);
    static_assert(std::is_same_v<Vector3i16, Vector3<std::int16_t>>);
    static_assert(std::is_same_v<Vector3i32, Vector3<std::int32_t>>);
    static_assert(std::is_same_v<Vector3i64, Vector3<std::int64_t>>);
    static_assert(std::is_same_v<Vector3u8, Vector3<std::uint8_t>>);
    static_assert(std::is_same_v<Vector3u16, Vector3<std::uint16_t>>);
    static_assert(std::is_same_v<Vector3u32, Vector3<std::uint32_t>>);
    static_assert(std::is_same_v<Vector3u64, Vector3<std::uint64_t>>);
    static_assert(std::is_same_v<Vector3f, Vector3<float>>);
    static_assert(std::is_same_v<Vector3d, Vector3<double>>);
    static_assert(std::is_same_v<Vector3ld, Vector3<long double>>);
    static_assert(std::is_same_v<Vector4i8, Vector4<std::int8_t>>);
    static_assert(std::is_same_v<Vector4i16, Vector4<std::int16_t>>);
    static_assert(std::is_same_v<Vector4i32, Vector4<std::int32_t>>);
    static_assert(std::is_same_v<Vector4i64, Vector4<std::int64_t>>);
    static_assert(std::is_same_v<Vector4u8, Vector4<std::uint8_t>>);
    static_assert(std::is_same_v<Vector4u16, Vector4<std::uint16_t>>);
    static_assert(std::is_same_v<Vector4u32, Vector4<std::uint32_t>>);
    static_assert(std::is_same_v<Vector4u64, Vector4<std::uint64_t>>);
    static_assert(std::is_same_v<Vector4f, Vector4<float>>);
    static_assert(std::is_same_v<Vector4d, Vector4<double>>);
    static_assert(std::is_same_v<Vector4ld, Vector4<long double>>);

    TEST(constructors_copy_move_conversion_assignment_and_convert)
    {
        Vector2<int> v2_default;
        Vector2<int> v2_splat(3);
        Vector2<int> v2(1, 2);
        CHECK((ComponentsNear(v2_default, {0, 0})));
        CHECK((ComponentsNear(v2_splat, {3, 3})));
        CHECK((ComponentsNear(v2, {1, 2})));
        Vector2<int> v2_copy(v2);
        Vector2<int> v2_move(std::move(v2_copy));
        CHECK((ComponentsNear(v2_move, {1, 2})));
        Vector2<double> v2_converted(v2);
        Vector2<double> v2_rvalue(Vector2<int>(3, 4));
        CHECK((ComponentsNear(v2_converted, {1, 2})));
        CHECK((ComponentsNear(v2_rvalue, {3, 4})));
        v2_converted = Vector2<short>(5, 6);
        const Vector2<short> v2_source(7, 8);
        v2_converted = v2_source;
        CHECK((ComponentsNear(v2_converted, {7, 8})));
        CHECK((ComponentsNear(v2.Convert<float>(), {1, 2})));

        Vector3<int> v3_default;
        Vector3<int> v3_splat(3);
        Vector3<int> v3(1, 2, 3);
        CHECK((ComponentsNear(v3_default, {0, 0, 0})));
        CHECK((ComponentsNear(v3_splat, {3, 3, 3})));
        Vector3<int> v3_copy(v3);
        Vector3<int> v3_move(std::move(v3_copy));
        CHECK((ComponentsNear(v3_move, {1, 2, 3})));
        Vector3<double> v3_converted(v3);
        Vector3<double> v3_rvalue(Vector3<int>(4, 5, 6));
        v3_converted = Vector3<short>(7, 8, 9);
        const Vector3<short> v3_source(10, 11, 12);
        v3_converted = v3_source;
        CHECK((ComponentsNear(v3_rvalue, {4, 5, 6})));
        CHECK((ComponentsNear(v3_converted, {10, 11, 12})));
        CHECK((ComponentsNear(v3.Convert<float>(), {1, 2, 3})));

        Vector4<int> v4_default;
        Vector4<int> v4_splat(3);
        Vector4<int> v4(1, 2, 3, 4);
        CHECK((ComponentsNear(v4_default, {0, 0, 0, 0})));
        CHECK((ComponentsNear(v4_splat, {3, 3, 3, 3})));
        Vector4<int> v4_copy(v4);
        Vector4<int> v4_move(std::move(v4_copy));
        CHECK((ComponentsNear(v4_move, {1, 2, 3, 4})));
        Vector4<double> v4_converted(v4);
        Vector4<double> v4_rvalue(Vector4<int>(5, 6, 7, 8));
        v4_converted = Vector4<short>(9, 10, 11, 12);
        const Vector4<short> v4_source(13, 14, 15, 16);
        v4_converted = v4_source;
        CHECK((ComponentsNear(v4_rvalue, {5, 6, 7, 8})));
        CHECK((ComponentsNear(v4_converted, {13, 14, 15, 16})));
        CHECK((ComponentsNear(v4.Convert<float>(), {1, 2, 3, 4})));
    }

    TEST(vector2_arithmetic_scalar_and_compound_operators)
    {
        const Vector2<int> a(8, 12);
        const Vector2<short> b(2, 3);
        CHECK((ComponentsNear(a + b, {10, 15})));
        CHECK((ComponentsNear(a - b, {6, 9})));
        CHECK((ComponentsNear(a * b, {16, 36})));
        CHECK((ComponentsNear(a / b, {4, 4})));
        CHECK((ComponentsNear(a % b, {0, 0})));
        CHECK((ComponentsNear(a + 2.5, {10.5, 14.5})));
        CHECK((ComponentsNear(a - 2, {6, 10})));
        CHECK((ComponentsNear(a * 2, {16, 24})));
        CHECK((ComponentsNear(a / 2, {4, 6})));
        CHECK((ComponentsNear(a % 5, {3, 2})));
        CHECK((ComponentsNear(2 + a, {10, 14})));
        CHECK((ComponentsNear(20 - a, {12, 8})));
        CHECK((ComponentsNear(2 * a, {16, 24})));
        CHECK((ComponentsNear(24 / a, {3, 2})));
        CHECK((ComponentsNear(25 % a, {1, 1})));

        Vector2<int> value(8, 12);
        CHECK(&(value += b) == &value);
        CHECK((ComponentsNear(value, {10, 15})));
        value -= b;
        value *= b;
        value /= b;
        value %= Vector2<int>(5, 5);
        CHECK((ComponentsNear(value, {3, 2})));
        value += 2;
        value -= 1;
        value *= 2;
        value /= 2;
        value %= 3;
        CHECK((ComponentsNear(value, {1, 0})));
        CHECK((ComponentsNear(-a, {-8, -12})));
        CHECK((ComponentsNear(+a, {8, 12})));
    }

    TEST(vector3_arithmetic_scalar_and_compound_operators)
    {
        const Vector3<int> a(8, 12, 18);
        const Vector3<short> b(2, 3, 6);
        CHECK((ComponentsNear(a + b, {10, 15, 24})));
        CHECK((ComponentsNear(a - b, {6, 9, 12})));
        CHECK((ComponentsNear(a * b, {16, 36, 108})));
        CHECK((ComponentsNear(a / b, {4, 4, 3})));
        CHECK((ComponentsNear(a % b, {0, 0, 0})));
        CHECK((ComponentsNear(a + 2, {10, 14, 20})));
        CHECK((ComponentsNear(a - 2, {6, 10, 16})));
        CHECK((ComponentsNear(a * 2, {16, 24, 36})));
        CHECK((ComponentsNear(a / 2, {4, 6, 9})));
        CHECK((ComponentsNear(a % 5, {3, 2, 3})));
        CHECK((ComponentsNear(2 + a, {10, 14, 20})));
        CHECK((ComponentsNear(20 - a, {12, 8, 2})));
        CHECK((ComponentsNear(2 * a, {16, 24, 36})));
        CHECK((ComponentsNear(72 / a, {9, 6, 4})));
        CHECK((ComponentsNear(73 % a, {1, 1, 1})));

        Vector3<int> value(8, 12, 18);
        CHECK(&(value += b) == &value);
        value -= b;
        value *= b;
        value /= b;
        value %= Vector3<int>(5, 5, 5);
        CHECK((ComponentsNear(value, {3, 2, 3})));
        value += 2;
        value -= 1;
        value *= 2;
        value /= 2;
        value %= 3;
        CHECK((ComponentsNear(value, {1, 0, 1})));
        CHECK((ComponentsNear(-a, {-8, -12, -18})));
        CHECK((ComponentsNear(+a, {8, 12, 18})));
    }

    TEST(vector4_arithmetic_scalar_and_compound_operators)
    {
        const Vector4<int> a(8, 12, 18, 24);
        const Vector4<short> b(2, 3, 6, 8);
        CHECK((ComponentsNear(a + b, {10, 15, 24, 32})));
        CHECK((ComponentsNear(a - b, {6, 9, 12, 16})));
        CHECK((ComponentsNear(a * b, {16, 36, 108, 192})));
        CHECK((ComponentsNear(a / b, {4, 4, 3, 3})));
        CHECK((ComponentsNear(a % b, {0, 0, 0, 0})));
        CHECK((ComponentsNear(a + 2, {10, 14, 20, 26})));
        CHECK((ComponentsNear(a - 2, {6, 10, 16, 22})));
        CHECK((ComponentsNear(a * 2, {16, 24, 36, 48})));
        CHECK((ComponentsNear(a / 2, {4, 6, 9, 12})));
        CHECK((ComponentsNear(a % 5, {3, 2, 3, 4})));
        CHECK((ComponentsNear(2 + a, {10, 14, 20, 26})));
        CHECK((ComponentsNear(30 - a, {22, 18, 12, 6})));
        CHECK((ComponentsNear(2 * a, {16, 24, 36, 48})));
        CHECK((ComponentsNear(144 / a, {18, 12, 8, 6})));
        CHECK((ComponentsNear(145 % a, {1, 1, 1, 1})));

        Vector4<int> value(8, 12, 18, 24);
        CHECK(&(value += b) == &value);
        value -= b;
        value *= b;
        value /= b;
        value %= Vector4<int>(5, 5, 5, 5);
        CHECK((ComponentsNear(value, {3, 2, 3, 4})));
        value += 2;
        value -= 1;
        value *= 2;
        value /= 2;
        value %= 3;
        CHECK((ComponentsNear(value, {1, 0, 1, 2})));
        CHECK((ComponentsNear(-a, {-8, -12, -18, -24})));
        CHECK((ComponentsNear(+a, {8, 12, 18, 24})));
    }

    TEST(safe_division_modulo_zero_extrema_and_floating_point)
    {
        const int lowest = std::numeric_limits<int>::lowest();
        CHECK((ComponentsNear(Vector2<int>(lowest, 7) / Vector2<int>(-1, 0),
                              {std::numeric_limits<int>::max(), 0})));
        CHECK((ComponentsNear(Vector2<int>(lowest, 7) / Vector2<int>(2, 1),
                              {lowest / 2, 7})));
        CHECK((ComponentsNear(Vector3<int>(lowest, 7, 9) % Vector3<int>(-1, 0, 4), {0, 0, 1})));
        CHECK((ComponentsNear(Vector3<int>(lowest, 7, 9) % Vector3<int>(2, 1, 4), {0, 0, 1})));
        CHECK((ComponentsNear(Vector4<int>(1, 2, 3, 4) / 0, {0, 0, 0, 0})));
        CHECK((ComponentsNear(10 / Vector4<int>(0, 2, 5, 10), {0, 5, 2, 1})));
        CHECK((ComponentsNear(10 % Vector4<int>(0, 3, 4, 6), {0, 1, 2, 4})));

        Vector2<int> compound(lowest, 9);
        compound /= Vector2<int>(-1, 0);
        CHECK((ComponentsNear(compound, {std::numeric_limits<int>::max(), 0})));
        compound = Vector2<int>(lowest, 9);
        compound %= Vector2<int>(-1, 0);
        CHECK((ComponentsNear(compound, {0, 0})));
        compound = Vector2<int>(5, 6);
        compound /= 0;
        compound %= 0;
        CHECK((ComponentsNear(compound, {0, 0})));

        CHECK((ComponentsNear(Vector2<double>(5.5, -5.5) % 2.0, {1.5, -1.5})));
        CHECK((ComponentsNear(5.5 % Vector3<double>(2.0, 0.0, -2.0), {1.5, 0.0, 1.5})));
        Vector4<double> floating(5.5, 6.5, -5.5, -6.5);
        floating %= Vector4<double>(2.0, 4.0, 2.0, 4.0);
        CHECK((ComponentsNear(floating, {1.5, 2.5, -1.5, -2.5})));
        floating %= 1.0;
        CHECK((ComponentsNear(floating, {0.5, 0.5, -0.5, -0.5})));
    }

    TEST(bitwise_and_shift_operators_for_every_dimension)
    {
        Vector2<int> v2(0x0c, 0x0a);
        const Vector2<int> m2(0x05, 0x03);
        CHECK((ComponentsNear(v2 & m2, {4, 2})));
        CHECK((ComponentsNear(v2 & 0x06, {4, 2})));
        CHECK((ComponentsNear(0x06 & v2, {4, 2})));
        CHECK((ComponentsNear(v2 | m2, {13, 11})));
        CHECK((ComponentsNear(v2 | 1, {13, 11})));
        CHECK((ComponentsNear(1 | v2, {13, 11})));
        CHECK((ComponentsNear(v2 ^ m2, {9, 9})));
        CHECK((ComponentsNear(v2 ^ 3, {15, 9})));
        CHECK((ComponentsNear(3 ^ v2, {15, 9})));
        v2 &= m2;
        v2 |= Vector2<int>(1, 4);
        v2 ^= Vector2<int>(2, 1);
        v2 &= 7;
        v2 |= 8;
        v2 ^= 1;
        CHECK((ComponentsNear(v2, {14, 14})));
        CHECK((ComponentsNear(~Vector2<int>(0, 1), {~0, ~1})));

        Vector3<int> v3(1, 2, 4);
        CHECK((ComponentsNear(v3 & Vector3<int>(3, 3, 3), {1, 2, 0})));
        CHECK((ComponentsNear(v3 & 3, {1, 2, 0})));
        CHECK((ComponentsNear(3 & v3, {1, 2, 0})));
        CHECK((ComponentsNear(v3 | Vector3<int>(2, 4, 1), {3, 6, 5})));
        CHECK((ComponentsNear(v3 | 8, {9, 10, 12})));
        CHECK((ComponentsNear(8 | v3, {9, 10, 12})));
        CHECK((ComponentsNear(v3 ^ Vector3<int>(3, 3, 3), {2, 1, 7})));
        CHECK((ComponentsNear(v3 ^ 1, {0, 3, 5})));
        CHECK((ComponentsNear(1 ^ v3, {0, 3, 5})));
        v3 &= Vector3<int>(7, 7, 7);
        v3 |= Vector3<int>(8, 8, 8);
        v3 ^= Vector3<int>(1, 1, 1);
        v3 &= 15;
        v3 |= 16;
        v3 ^= 2;
        CHECK((ComponentsNear(v3, {26, 25, 31})));
        CHECK((ComponentsNear(~Vector3<int>(0, 1, 2), {~0, ~1, ~2})));

        Vector4<int> v4(1, 2, 4, 8);
        CHECK((ComponentsNear(v4 & Vector4<int>(3, 3, 7, 15), {1, 2, 4, 8})));
        CHECK((ComponentsNear(v4 & 3, {1, 2, 0, 0})));
        CHECK((ComponentsNear(3 & v4, {1, 2, 0, 0})));
        CHECK((ComponentsNear(v4 | Vector4<int>(2, 4, 8, 1), {3, 6, 12, 9})));
        CHECK((ComponentsNear(v4 | 16, {17, 18, 20, 24})));
        CHECK((ComponentsNear(16 | v4, {17, 18, 20, 24})));
        CHECK((ComponentsNear(v4 ^ Vector4<int>(3, 3, 3, 3), {2, 1, 7, 11})));
        CHECK((ComponentsNear(v4 ^ 1, {0, 3, 5, 9})));
        CHECK((ComponentsNear(1 ^ v4, {0, 3, 5, 9})));
        v4 &= Vector4<int>(15);
        v4 |= Vector4<int>(16);
        v4 ^= Vector4<int>(1);
        v4 &= 31;
        v4 |= 32;
        v4 ^= 2;
        CHECK((ComponentsNear(v4, {50, 49, 55, 59})));
        CHECK((ComponentsNear(~Vector4<int>(0, 1, 2, 3), {~0, ~1, ~2, ~3})));

        const int digits = std::numeric_limits<unsigned int>::digits;
        CHECK((ComponentsNear(Vector2<int>(1, 2) << 2, {4, 8})));
        CHECK((ComponentsNear(Vector2<int>(8, 16) >> 2, {2, 4})));
        CHECK((ComponentsNear(Vector3<int>(1, 2, 3) << -1, {0, 0, 0})));
        CHECK((ComponentsNear(Vector3<int>(1, 2, 3) >> digits, {0, 0, 0})));
        CHECK((ComponentsNear(Vector4<int>(1, 2, 3, 4) << digits, {0, 0, 0, 0})));
        CHECK((ComponentsNear(Vector4<int>(1, 2, 3, 4) >> -1, {0, 0, 0, 0})));
        Vector2<int> shift2(1, 2);
        shift2 <<= 3;
        shift2 >>= 2;
        CHECK((ComponentsNear(shift2, {2, 4})));
        Vector3<int> shift3(1, 2, 3);
        shift3 <<= -1;
        shift3 >>= digits;
        CHECK((ComponentsNear(shift3, {0, 0, 0})));
        Vector4<int> shift4(16, 32, 48, 64);
        shift4 >>= 4;
        shift4 <<= 1;
        CHECK((ComponentsNear(shift4, {2, 4, 6, 8})));
        CHECK((ComponentsNear(Vector2<int>(-1, -2) >> 1,
                              {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()})));
    }

    TEST(boolean_and_comparisons_cover_integral_float_and_short_circuit_paths)
    {
        CHECK(!static_cast<bool>(Vector2<int>()));
        CHECK(static_cast<bool>(Vector2<int>(1, 0)));
        CHECK(static_cast<bool>(Vector2<int>(0, 1)));
        CHECK(!static_cast<bool>(Vector3<int>()));
        CHECK(static_cast<bool>(Vector3<int>(1, 0, 0)));
        CHECK(static_cast<bool>(Vector3<int>(0, 1, 0)));
        CHECK(static_cast<bool>(Vector3<int>(0, 0, 1)));
        CHECK(!static_cast<bool>(Vector4<int>()));
        CHECK(static_cast<bool>(Vector4<int>(1, 0, 0, 0)));
        CHECK(static_cast<bool>(Vector4<int>(0, 1, 0, 0)));
        CHECK(static_cast<bool>(Vector4<int>(0, 0, 1, 0)));
        CHECK(static_cast<bool>(Vector4<int>(0, 0, 0, 1)));

        const Vector2<int> v2a(3, 4);
        const Vector2<int> v2b(0, 5);
        const Vector2<int> v2c(6, 8);
        CHECK(v2a != v2b);
        CHECK(v2a < v2c);
        CHECK(v2a <= v2b);
        CHECK(v2c > v2a);
        CHECK(v2b >= v2a);
        CHECK(v2a == Vector2<short>(3, 4));
        CHECK(v2a == Vector2<int>(3, 4));
        CHECK(v2a != Vector2<int>(9, 4));
        CHECK(v2a != Vector2<int>(3, 9));

        const Vector3<int> v3a(1, 2, 2);
        const Vector3<int> v3b(0, 0, 3);
        const Vector3<int> v3c(0, 0, 4);
        CHECK(v3a != Vector3<int>(2, 1, 2));
        CHECK(v3a < v3c);
        CHECK(v3a <= v3b);
        CHECK(v3c > v3a);
        CHECK(v3b >= v3a);
        CHECK(v3a == Vector3<int>(1, 2, 2));
        CHECK(v3a != Vector3<int>(9, 2, 2));
        CHECK(v3a != Vector3<int>(1, 9, 2));
        CHECK(v3a != Vector3<int>(1, 2, 9));

        const Vector4<int> v4a(1, 1, 1, 1);
        const Vector4<int> v4b(2, 0, 0, 0);
        const Vector4<int> v4c(3, 0, 0, 0);
        CHECK(v4a != v4b);
        CHECK(v4a < v4c);
        CHECK(v4a <= v4b);
        CHECK(v4c > v4a);
        CHECK(v4b >= v4a);
        CHECK(v4a == Vector4<int>(1, 1, 1, 1));
        CHECK(v4a != Vector4<int>(9, 1, 1, 1));
        CHECK(v4a != Vector4<int>(1, 9, 1, 1));
        CHECK(v4a != Vector4<int>(1, 1, 9, 1));
        CHECK(v4a != Vector4<int>(1, 1, 1, 9));

        CHECK(Vector2<double>(1.0, 2.0) == Vector2<double>(1.0 + 5.0e-7, 2.0 - 5.0e-7));
        CHECK(Vector2<double>(1.0, 2.0) != Vector2<double>(1.0 + 2.0e-6, 2.0));
        CHECK(Vector2<double>(1.0, 2.0) != Vector2<double>(1.0, 2.0 + 3.0e-6));
        CHECK(Vector3<double>(1.0, 2.0, 3.0) == Vector3<double>(1.0, 2.0, 3.0));
        CHECK(Vector3<double>(1.0, 2.0, 3.0) != Vector3<double>(1.1, 2.0, 3.0));
        CHECK(Vector3<double>(1.0, 2.0, 3.0) != Vector3<double>(1.0, 2.0, 3.1));
        CHECK(Vector3<double>(1.0, 2.0, 3.0) != Vector3<double>(1.0, 2.1, 3.0));
        CHECK(Vector4<double>(1.0, 2.0, 3.0, 4.0) == Vector4<double>(1.0, 2.0, 3.0, 4.0));
        CHECK(Vector4<double>(1.0, 2.0, 3.0, 4.0) != Vector4<double>(1.1, 2.0, 3.0, 4.0));
        CHECK(Vector4<double>(1.0, 2.0, 3.0, 4.0) != Vector4<double>(1.0, 2.1, 3.0, 4.0));
        CHECK(Vector4<double>(1.0, 2.0, 3.0, 4.0) != Vector4<double>(1.0, 2.0, 3.1, 4.0));
        CHECK(Vector4<double>(1.0, 2.0, 3.0, 4.0) != Vector4<double>(1.0, 2.0, 3.0, 4.1));

        EqProbe::calls = 0;
        CHECK(Vector2<EqProbe>(1, 2) != Vector2<EqProbe>(9, 2));
        CHECK_EQUAL(EqProbe::calls, 1);
        CHECK(Vector2<EqProbe>(1, 2) != Vector2<EqProbe>(1, 9));
        CHECK_EQUAL(EqProbe::calls, 3);
        CHECK(Vector2<EqProbe>(1, 2) == Vector2<EqProbe>(1, 2));
        CHECK_EQUAL(EqProbe::calls, 5);
        EqProbe::calls = 0;
        CHECK(Vector3<EqProbe>(1, 2, 3) != Vector3<EqProbe>(9, 2, 3));
        CHECK_EQUAL(EqProbe::calls, 1);
        CHECK(Vector3<EqProbe>(1, 2, 3) != Vector3<EqProbe>(1, 9, 3));
        CHECK_EQUAL(EqProbe::calls, 3);
        CHECK(Vector3<EqProbe>(1, 2, 3) != Vector3<EqProbe>(1, 2, 9));
        CHECK_EQUAL(EqProbe::calls, 6);
        CHECK(Vector3<EqProbe>(1, 2, 3) == Vector3<EqProbe>(1, 2, 3));
        CHECK_EQUAL(EqProbe::calls, 9);
        EqProbe::calls = 0;
        CHECK(Vector4<EqProbe>(1, 2, 3, 4) != Vector4<EqProbe>(9, 2, 3, 4));
        CHECK_EQUAL(EqProbe::calls, 1);
        CHECK(Vector4<EqProbe>(1, 2, 3, 4) != Vector4<EqProbe>(1, 9, 3, 4));
        CHECK_EQUAL(EqProbe::calls, 3);
        CHECK(Vector4<EqProbe>(1, 2, 3, 4) != Vector4<EqProbe>(1, 2, 9, 4));
        CHECK_EQUAL(EqProbe::calls, 6);
        CHECK(Vector4<EqProbe>(1, 2, 3, 4) != Vector4<EqProbe>(1, 2, 3, 9));
        CHECK_EQUAL(EqProbe::calls, 10);
        CHECK(Vector4<EqProbe>(1, 2, 3, 4) == Vector4<EqProbe>(1, 2, 3, 4));
        CHECK_EQUAL(EqProbe::calls, 14);
    }

    TEST(vector2_geometry_rotation_interpolation_and_movement)
    {
        const Vector2<double> value(3.0, 4.0);
        CHECK(Near(value.Length(), 5.0));
        CHECK(Near(value.LengthSqr(), 25.0));
        CHECK(Near(value.DotProduct(Vector2<double>(-4.0, 3.0)), 0.0));
        CHECK(Near(value.Distance(Vector2<double>()), 5.0));
        CHECK(Near(value.DistanceSqr(Vector2<double>()), 25.0));
        CHECK(Near(Vector2<double>(1, 0).Angle(Vector2<double>(0, 1)), std::acos(-1.0L) / 2));
        CHECK((ComponentsNear(value.Normalize(), {0.6, 0.8})));
        CHECK((ComponentsNear(Vector2<double>().Normalize(), {0, 0})));
        Vector2<double> in_place(value);
        CHECK(&in_place.NormalizeInPlace() == &in_place);
        CHECK((ComponentsNear(in_place, {0.6, 0.8})));
        CHECK((ComponentsNear(Vector2<double>(0, 10).Lerp(Vector2<double>(10, 20), 0.25), {2.5, 12.5})));
        CHECK((ComponentsNear(Vector2<double>(1, -1).Reflect(Vector2<double>(0, 1)), {1, 1})));
        in_place = Vector2<double>(1, -1);
        CHECK(&in_place.ReflectInPlace(Vector2<double>(0, 1)) == &in_place);
        CHECK((ComponentsNear(in_place, {1, 1})));
        CHECK((ComponentsNear(Vector2<double>(1, 0).Rotate(std::acos(-1.0L) / 2), {0, 1})));
        in_place = Vector2<double>(1, 0);
        CHECK(&in_place.RotateInPlace(std::acos(-1.0L) / 2) == &in_place);
        CHECK((ComponentsNear(in_place, {0, 1})));

        const Vector2<double> origin;
        const Vector2<double> target(3, 4);
        CHECK((ComponentsNear(origin.MoveTowards(target, 2), {1.2, 1.6})));
        CHECK((ComponentsNear(origin.MoveTowards(target, 5), {3, 4})));
        CHECK((ComponentsNear(target.MoveTowards(target, 1), {3, 4})));
        CHECK((ComponentsNear(origin.MoveTowards(target, -1), {-0.6, -0.8})));
        in_place = origin;
        CHECK(&in_place.MoveTowardsInPlace(target, 2) == &in_place);
        CHECK((ComponentsNear(in_place, {1.2, 1.6})));
        CHECK((ComponentsNear(Vector2<double>(2, 0).Invert(), {0.5, 0})));
        in_place = Vector2<double>(4, 0);
        CHECK(&in_place.InvertInPlace() == &in_place);
        CHECK((ComponentsNear(in_place, {0.25, 0})));
    }

    TEST(vector3_geometry_projection_orthogonalization_and_degenerate_cases)
    {
        const Vector3<double> x_axis(1, 0, 0);
        const Vector3<double> y_axis(0, 1, 0);
        CHECK((ComponentsNear(x_axis.CrossProduct(y_axis), {0, 0, 1})));
        Vector3<double> in_place(x_axis);
        CHECK(&in_place.CrossProductInPlace(y_axis) == &in_place);
        CHECK((ComponentsNear(in_place, {0, 0, 1})));
        CHECK((ComponentsNear(Vector3<double>(1, 2, 3).Perpendicular(), {0, 3, -2})));
        CHECK((ComponentsNear(Vector3<double>(3, 1, 2).Perpendicular(), {-2, 0, 3})));
        CHECK((ComponentsNear(Vector3<double>(3, 2, 1).Perpendicular(), {2, -3, 0})));
        in_place = Vector3<double>(3, 2, 1);
        CHECK(&in_place.PerpendicularInPlace() == &in_place);
        CHECK(Near(in_place.DotProduct(Vector3<double>(3, 2, 1)), 0));

        const Vector3<double> value(1, 2, 2);
        CHECK(Near(value.Length(), 3));
        CHECK(Near(value.LengthSqr(), 9));
        CHECK(Near(value.DotProduct(Vector3<double>(2, 1, 0)), 4));
        CHECK(Near(value.Distance(Vector3<double>(1, 2, 5)), 3));
        CHECK(Near(value.DistanceSqr(Vector3<double>(1, 2, 5)), 9));
        CHECK(Near(x_axis.Angle(y_axis), std::acos(-1.0L) / 2));
        CHECK((ComponentsNear(value.Normalize(), {1.0 / 3, 2.0 / 3, 2.0 / 3})));
        CHECK((ComponentsNear(Vector3<double>().Normalize(), {0, 0, 0})));
        in_place = value;
        CHECK(&in_place.NormalizeInPlace() == &in_place);

        const Vector3<double> projected = Vector3<double>(2, 3, 0).Project(x_axis);
        CHECK_MSG((ComponentsNear(projected, {2, 0, 0})),
                  "Project must return the component parallel to the target vector");
        CHECK((ComponentsNear(value.Project(Vector3<double>()), {0, 0, 0})));
        in_place = Vector3<double>(2, 3, 0);
        CHECK(&in_place.ProjectInPlace(x_axis) == &in_place);
        CHECK_MSG((ComponentsNear(in_place, {2, 0, 0})),
                  "ProjectInPlace must store the parallel projection");

        const auto orthogonal = x_axis.OrthoNormalize(y_axis);
        CHECK((ComponentsNear(orthogonal.first, {1, 0, 0})));
        CHECK((ComponentsNear(orthogonal.second, {0, 1, 0})));
        in_place = Vector3<double>(2, 0, 0);
        CHECK(&in_place.OrthoNormalizeInPlace(nullptr) == &in_place);
        CHECK((ComponentsNear(in_place, {2, 0, 0})));
        Vector3<double> other(0, 3, 0);
        in_place.OrthoNormalizeInPlace(&other);
        CHECK(Near(in_place.Length(), 1));
        CHECK(Near(other.Length(), 1));
        CHECK(Near(in_place.DotProduct(other), 0));

        CHECK((ComponentsNear(x_axis.RotateByAxisAngle(Vector3<double>(0, 0, 1),
                                                       std::acos(-1.0L) / 2), {0, 1, 0})));
        in_place = x_axis;
        CHECK(&in_place.RotateByAxisAngleInPlace(Vector3<double>(0, 0, 1),
                                                 std::acos(-1.0L) / 2) == &in_place);
        CHECK((ComponentsNear(in_place, {0, 1, 0})));

        CHECK((ComponentsNear(Vector3<double>().MoveTowards(Vector3<double>(0, 0, 4), 2), {0, 0, 2})));
        CHECK((ComponentsNear(Vector3<double>().MoveTowards(Vector3<double>(0, 0, 4), 4), {0, 0, 4})));
        CHECK((ComponentsNear(Vector3<double>().MoveTowards(Vector3<double>(0, 0, 4), -1), {0, 0, -1})));
        CHECK((ComponentsNear(x_axis.MoveTowards(x_axis, 1), {1, 0, 0})));
        in_place = Vector3<double>();
        CHECK(&in_place.MoveTowardsInPlace(Vector3<double>(0, 0, 4), 2) == &in_place);
        CHECK((ComponentsNear(in_place, {0, 0, 2})));
        CHECK((ComponentsNear(Vector3<double>(0, 0, 0).Lerp(Vector3<double>(2, 4, 6), 0.5), {1, 2, 3})));
        in_place = Vector3<double>();
        CHECK(&in_place.LerpInPlace(Vector3<double>(2, 4, 6), 0.5) == &in_place);
        CHECK((ComponentsNear(in_place, {1, 2, 3})));

        const Vector3<double> zero_tangent;
        CHECK((ComponentsNear(Vector3<double>().CubicHermite(zero_tangent, Vector3<double>(2, 4, 6),
                                                            zero_tangent, 0.5), {1, 2, 3})));
        in_place = Vector3<double>();
        CHECK(&in_place.CubicHermiteInPlace(zero_tangent, Vector3<double>(2, 4, 6),
                                            zero_tangent, 0.5) == &in_place);
        CHECK((ComponentsNear(in_place, {1, 2, 3})));
        CHECK((ComponentsNear(Vector3<double>(1, -1, 0).Reflect(y_axis), {1, 1, 0})));
        in_place = Vector3<double>(1, -1, 0);
        CHECK(&in_place.ReflectInPlace(y_axis) == &in_place);
        CHECK((ComponentsNear(in_place, {1, 1, 0})));

        const Vector3<double> a(0, 0, 0);
        const Vector3<double> b(1, 0, 0);
        const Vector3<double> c(0, 1, 0);
        CHECK((ComponentsNear(Vector3<double>(0.25, 0.25, 0).Barycenter(a, b, c), {0.5, 0.25, 0.25})));
        CHECK((ComponentsNear(Vector3<double>(1, 0, 0).Barycenter(a, a, a), {0, 0, 0})));
        in_place = Vector3<double>(0.25, 0.25, 0);
        CHECK(&in_place.BarycenterInPlace(a, b, c) == &in_place);
        CHECK((ComponentsNear(in_place, {0.5, 0.25, 0.25})));
        CHECK((ComponentsNear(Vector3<double>(2, 0, -4).Invert(), {0.5, 0, -0.25})));
        in_place = Vector3<double>(2, 0, -4);
        CHECK(&in_place.InvertInPlace() == &in_place);
        CHECK((ComponentsNear(in_place, {0.5, 0, -0.25})));
    }

    TEST(vector4_geometry_normalization_interpolation_and_movement)
    {
        const Vector4<double> value(1, 2, 2, 4);
        CHECK(Near(value.Length(), 5));
        CHECK(Near(value.LengthSqr(), 25));
        CHECK(Near(value.DotProduct(Vector4<double>(1, 1, 1, 1)), 9));
        CHECK(Near(value.Distance(Vector4<double>(1, 2, 2, 9)), 5));
        CHECK(Near(value.DistanceSqr(Vector4<double>(1, 2, 2, 9)), 25));
        CHECK((ComponentsNear(value.Normalize(), {0.2, 0.4, 0.4, 0.8})));
        CHECK((ComponentsNear(Vector4<double>().Normalize(), {0, 0, 0, 0})));
        Vector4<double> in_place(value);
        CHECK(&in_place.NormalizeInPlace() == &in_place);
        CHECK((ComponentsNear(in_place, {0.2, 0.4, 0.4, 0.8})));

        const Vector4<double> origin;
        const Vector4<double> target(0, 0, 0, 4);
        CHECK((ComponentsNear(origin.MoveTowards(target, 2), {0, 0, 0, 2})));
        CHECK((ComponentsNear(origin.MoveTowards(target, 4), {0, 0, 0, 4})));
        CHECK((ComponentsNear(target.MoveTowards(target, 1), {0, 0, 0, 4})));
        CHECK((ComponentsNear(origin.MoveTowards(target, -1), {0, 0, 0, -1})));
        in_place = origin;
        CHECK(&in_place.MoveTowardsInPlace(target, 2) == &in_place);
        CHECK((ComponentsNear(in_place, {0, 0, 0, 2})));
        CHECK((ComponentsNear(origin.Lerp(Vector4<double>(2, 4, 6, 8), 0.25), {0.5, 1, 1.5, 2})));
        in_place = origin;
        CHECK(&in_place.LerpInPlace(Vector4<double>(2, 4, 6, 8), 0.25) == &in_place);
        CHECK((ComponentsNear(in_place, {0.5, 1, 1.5, 2})));
        CHECK((ComponentsNear(Vector4<double>(2, 0, -4, 0.5).Invert(), {0.5, 0, -0.25, 2})));
        in_place = Vector4<double>(2, 0, -4, 0.5);
        CHECK(&in_place.InvertInPlace() == &in_place);
        CHECK((ComponentsNear(in_place, {0.5, 0, -0.25, 2})));
    }

    TEST(component_and_magnitude_clamping_minimum_and_maximum)
    {
        Vector2<double> v2(5, -5);
        CHECK((ComponentsNear(v2.Clamp(Vector2<double>(0, 0), Vector2<double>(4, -4)), {4, -4})));
        CHECK(&v2.ClampInPlace(Vector2<double>(4, -4), Vector2<double>(0, 0)) == &v2);
        CHECK((ComponentsNear(v2, {4, -4})));
        CHECK((ComponentsNear(Vector2<double>(3, 4).ClampMagnitude(10, 2), {3, 4})));
        CHECK((ComponentsNear(Vector2<double>(3, 4).ClampMagnitude(10, 12), {6, 8})));
        CHECK((ComponentsNear(Vector2<double>(3, 4).ClampMagnitude(0, 3), {1.8, 2.4})));
        CHECK((ComponentsNear(Vector2<double>(3, 4).ClampMagnitude(4, 6), {3, 4})));
        CHECK((ComponentsNear(Vector2<double>().ClampMagnitude(2, 4), {0, 0})));
        v2 = Vector2<double>(3, 4);
        CHECK(&v2.ClampMagnitudeInPlace(0, 3) == &v2);
        CHECK((ComponentsNear(v2, {1.8, 2.4})));

        Vector3<double> v3(5, -5, 2);
        CHECK((ComponentsNear(v3.Clamp(Vector3<double>(0, 0, 3), Vector3<double>(4, -4, 1)), {4, -4, 2})));
        CHECK(&v3.ClampInPlace(Vector3<double>(0, 0, 3), Vector3<double>(4, -4, 1)) == &v3);
        CHECK((ComponentsNear(Vector3<double>(3, 4, 0).ClampMagnitude(10, 2), {3, 4, 0})));
        CHECK((ComponentsNear(Vector3<double>(3, 4, 0).ClampMagnitude(10, 12), {6, 8, 0})));
        CHECK((ComponentsNear(Vector3<double>(3, 4, 0).ClampMagnitude(-3, -1), {0, 0, 0})));
        CHECK((ComponentsNear(Vector3<double>().ClampMagnitude(2, 4), {0, 0, 0})));
        v3 = Vector3<double>(3, 4, 0);
        CHECK(&v3.ClampMagnitudeInPlace(0, 3) == &v3);
        CHECK((ComponentsNear(v3, {1.8, 2.4, 0})));

        Vector4<double> v4(5, -5, 2, 9);
        CHECK((ComponentsNear(v4.Clamp(Vector4<double>(0, 0, 3, 10),
                                       Vector4<double>(4, -4, 1, 8)), {4, -4, 2, 9})));
        CHECK(&v4.ClampInPlace(Vector4<double>(0, 0, 3, 10), Vector4<double>(4, -4, 1, 8)) == &v4);
        CHECK((ComponentsNear(Vector4<double>(3, 4, 0, 0).ClampMagnitude(10, 2), {3, 4, 0, 0})));
        CHECK((ComponentsNear(Vector4<double>(3, 4, 0, 0).ClampMagnitude(10, 12), {6, 8, 0, 0})));
        CHECK((ComponentsNear(Vector4<double>(3, 4, 0, 0).ClampMagnitude(0, 3), {1.8, 2.4, 0, 0})));
        CHECK((ComponentsNear(Vector4<double>().ClampMagnitude(2, 4), {0, 0, 0, 0})));
        v4 = Vector4<double>(3, 4, 0, 0);
        CHECK(&v4.ClampMagnitudeInPlace(4, 6) == &v4);
        CHECK((ComponentsNear(v4, {3, 4, 0, 0})));

        CHECK((ComponentsNear(Min(Vector2<int>(1, 5), Vector2<double>(2, 4)), {1, 4})));
        CHECK((ComponentsNear(Max(Vector2<int>(1, 5), Vector2<double>(2, 4)), {2, 5})));
        CHECK((ComponentsNear(Min(Vector3<int>(1, 5, 3), Vector3<double>(2, 4, 6)), {1, 4, 3})));
        CHECK((ComponentsNear(Max(Vector3<int>(1, 5, 3), Vector3<double>(2, 4, 6)), {2, 5, 6})));
        CHECK((ComponentsNear(Min(Vector4<int>(1, 5, 3, 9), Vector4<double>(2, 4, 6, 8)), {1, 4, 3, 8})));
        CHECK((ComponentsNear(Max(Vector4<int>(1, 5, 3, 9), Vector4<double>(2, 4, 6, 8)), {2, 5, 6, 9})));
    }

    TEST(streaming_hashes_and_hash_equality_contract)
    {
        std::ostringstream stream;
        stream << Vector2<int>(1, 2) << ' ' << Vector3<int>(3, 4, 5) << ' '
               << Vector4<int>(6, 7, 8, 9);
        CHECK_EQUAL(stream.str(), "(1, 2) (3, 4, 5) (6, 7, 8, 9)");

        const Vector2<int> v2(1, 2);
        const Vector3<int> v3(1, 2, 3);
        const Vector4<int> v4(1, 2, 3, 4);
        CHECK_EQUAL(std::hash<Vector2<int>>{}(v2), std::hash<Vector2<int>>{}(Vector2<int>(1, 2)));
        CHECK_EQUAL(std::hash<Vector3<int>>{}(v3), std::hash<Vector3<int>>{}(Vector3<int>(1, 2, 3)));
        CHECK_EQUAL(std::hash<Vector4<int>>{}(v4), std::hash<Vector4<int>>{}(Vector4<int>(1, 2, 3, 4)));
        CHECK_NOT_EQUAL(std::hash<Vector2<int>>{}(v2), std::hash<Vector2<int>>{}(Vector2<int>(2, 1)));
        CHECK_NOT_EQUAL(std::hash<Vector3<int>>{}(v3), std::hash<Vector3<int>>{}(Vector3<int>(3, 2, 1)));
        CHECK_NOT_EQUAL(std::hash<Vector4<int>>{}(v4), std::hash<Vector4<int>>{}(Vector4<int>(4, 3, 2, 1)));

        std::unordered_set<Vector3<int>> values;
        values.insert(v3);
        CHECK_EQUAL(values.count(Vector3<int>(1, 2, 3)), 1u);

        const Vector2<double> close_left(1.0, 2.0);
        const Vector2<double> close_right(1.0 + 5.0e-7, 2.0);
        REQUIRE(close_left == close_right);
        CHECK_EQUAL_MSG(std::hash<Vector2<double>>{}(close_left),
                        std::hash<Vector2<double>>{}(close_right),
                        "equal vectors must have equal hashes");
    }

    TEST(nan_infinity_zero_and_extreme_values)
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double infinity = std::numeric_limits<double>::infinity();
        CHECK(Vector2<double>(nan, 0) != Vector2<double>(nan, 0));
        CHECK(Vector3<double>(0, nan, 0) != Vector3<double>(0, nan, 0));
        CHECK(Vector4<double>(0, 0, nan, 0) != Vector4<double>(0, 0, nan, 0));
        CHECK(std::isinf(Vector2<double>(infinity, 0).Length()));
        CHECK(std::isinf(Vector3<double>(0, infinity, 0).LengthSqr()));
        CHECK(std::isinf(Vector4<double>(0, 0, 0, infinity).Distance(Vector4<double>())));

        const Vector2<double> infinite(infinity, infinity);
        CHECK_MSG(infinite == infinite, "vector equality must remain reflexive for infinities");

        CHECK((ComponentsNear(Vector2<int>(std::numeric_limits<int>::max(), 0) /
                              Vector2<int>(1, 0), {std::numeric_limits<int>::max(), 0})));
        CHECK((ComponentsNear(Vector3<unsigned>(std::numeric_limits<unsigned>::max(), 0, 1) /
                              Vector3<unsigned>(1, 0, 1), {std::numeric_limits<unsigned>::max(), 0, 1})));
        const auto divided = Vector4<double>(infinity, 1, -infinity, 0) /
                             Vector4<double>(1, 0, -1, 0);
        CHECK(std::isinf(divided.x));
        CHECK_EQUAL(divided.y, 0);
        CHECK(std::isinf(divided.z));
        CHECK_EQUAL(divided.w, 0);
        CHECK(std::isnan((Vector2<double>(infinity, 1) % Vector2<double>(2, 1)).x));
        CHECK((ComponentsNear(Vector3<double>().Barycenter(Vector3<double>(), Vector3<double>(),
                                                          Vector3<double>()), {0, 0, 0})));
        CHECK((ComponentsNear(Vector3<double>().Perpendicular(), {0, 0, 0})));
    }
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
