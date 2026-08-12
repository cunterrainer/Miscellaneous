#include "Core.h"
#include "Test.h"

#include <cstdint>
#include <string>

namespace
{
    constexpr int Version(int major, int minor, int patch)
    {
        return CORE_VERSION_NUMBER(major, minor, patch);
    }

    TEST(version_number_encoding_and_decoding)
    {
        CHECK_EQUAL(CORE_VERSION_NUMBER_ZERO, 0);
        CHECK_EQUAL(CORE_VERSION_NUMBER_MIN, 1);
        CHECK_EQUAL(CORE_VERSION_NUMBER_AVAILABLE, 1);
        CHECK_EQUAL(CORE_VERSION_NUMBER_NOT_AVAILABLE, 0);
        CHECK_EQUAL(CORE_VERSION_NUMBER_MAX, 999999999);
        CHECK_EQUAL(Version(12, 34, 56789), 123456789);
        CHECK_EQUAL(Version(112, 134, 156789), 123456789);
        CHECK_EQUAL(CORE_VERSION_NUMBER_MAJOR(123456789), 12);
        CHECK_EQUAL(CORE_VERSION_NUMBER_MINOR(123456789), 34);
        CHECK_EQUAL(CORE_VERSION_NUMBER_PATCH(123456789), 56789);
    }

    TEST(hexadecimal_version_conversions)
    {
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VRP(0xABC), Version(10, 11, 12));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VVRP(0x12A3), Version(18, 10, 3));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VRPP(0xA3BC), Version(10, 3, 188));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VVRR(0x1234), Version(18, 52, 0));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VRRPPPP(0xA123456), Version(10, 18, 0x3456));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VVRRP(0x12345), Version(18, 52, 5));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VRRPP000(0xA1234000U),
                    static_cast<unsigned int>(Version(10, 18, 52)));
        CHECK_EQUAL(CORE_PREDEF_MAKE_0X_VVRRPP(0x123456), Version(18, 52, 86));
    }

    TEST(decimal_version_conversions)
    {
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VPPP(7123), Version(7, 0, 123));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVPPP(42123), Version(42, 0, 123));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VR0(470), Version(4, 7, 0));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRP(478), Version(4, 7, 8));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRP000(478000), Version(4, 7, 8));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRPPPP(471234), Version(4, 7, 1234));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRPP(4712), Version(4, 7, 12));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRR(434), Version(4, 34, 0));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRRPP(43456), Version(4, 34, 56));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VRR000(434000), Version(4, 34, 0));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VV00(4200), Version(42, 0, 0));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVR_0PPPPP(427, 12345), Version(42, 7, 12345));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRR(4234), Version(42, 34, 0));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRRP(42345), Version(42, 34, 5));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRRPP(423456), Version(42, 34, 56));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRRPPP(4234123), Version(42, 34, 123));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRR0PP00(423407800), Version(42, 34, 78));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRR0PPPP(423407890), Version(42, 34, 7890));
        CHECK_EQUAL(CORE_PREDEF_MAKE_10_VVRR00PP00(4234007800), Version(42, 34, 78));
    }

    TEST(date_version_conversions)
    {
        CHECK_EQUAL(CORE_PREDEF_MAKE_DATE(2024, 7, 9), Version(54, 7, 9));
        CHECK_EQUAL(CORE_PREDEF_MAKE_YYYYMMDD(20240709), Version(54, 7, 9));
        CHECK_EQUAL(CORE_PREDEF_MAKE_YYYY(2024), Version(54, 1, 1));
        CHECK_EQUAL(CORE_PREDEF_MAKE_YYYYMM(202407), Version(54, 7, 1));
    }

    TEST(native_detection_values_and_names_agree)
    {
        CHECK(std::string{CORE_ARCH_NAME} != "Unknown");
        CHECK_EQUAL(CORE_ARCH_WORD_BITS, static_cast<int>(sizeof(void*) * 8));
        CHECK(std::string{CORE_ARCH_WORD_BITS_NAME} != "Unknown");
        CHECK(std::string{CORE_COMP_NAME} != "Unknown");
        CHECK_EQUAL(std::string{CORE_LANG_NAME}, "C++");
        CHECK_EQUAL(std::string{CORE_LANG_STDCPP_NAME}, "Standard C++17");
        CHECK_EQUAL(CORE_LANG_STDCPP, CORE_PREDEF_MAKE_YYYYMM(201703L));
        CHECK(CORE_LANG_STDCPP_CPP17 == CORE_VERSION_NUMBER_AVAILABLE);
        CHECK(std::string{CORE_LIB_STD_NAME} != "Unknown");
        CHECK(std::string{CORE_LIB_C_NAME} != "Unknown");
        CHECK_EQUAL(std::string{CORE_OS_NAME}, "Linux");
        CHECK(CORE_OS_LINUX == CORE_VERSION_NUMBER_AVAILABLE);
        CHECK_EQUAL(std::string{CORE_ENDIAN_NAME}, "Byte-Swapped Little-Endian");
        CHECK(CORE_ENDIAN_LITTLE_BYTE == CORE_VERSION_NUMBER_AVAILABLE);
        CHECK(std::string{CORE_HW_SIMD_NAME} != "Unknown");

#if defined(__clang__)
        CHECK_EQUAL(std::string{CORE_COMP_NAME}, "Clang");
        CHECK(CORE_COMP_CLANG > CORE_VERSION_NUMBER_ZERO);
        CHECK(CORE_COMP_GCC_EMULATED > CORE_VERSION_NUMBER_ZERO);
#elif defined(__GNUC__)
        CHECK_EQUAL(std::string{CORE_COMP_NAME}, "Gnu GCC C/C++");
        CHECK(CORE_COMP_GCC > CORE_VERSION_NUMBER_ZERO);
#endif
    }

    TEST(debug_or_release_mode_is_exclusive)
    {
#ifdef NDEBUG
        CHECK(CORE_MODE_RELEASE == CORE_VERSION_NUMBER_AVAILABLE);
        CHECK_EQUAL(std::string{CORE_MODE_NAME}, "Release");
#ifdef CORE_MODE_DEBUG
        FAIL_CHECK("CORE_MODE_DEBUG must be absent in a release build");
#endif
#else
        CHECK(CORE_MODE_DEBUG == CORE_VERSION_NUMBER_AVAILABLE);
        CHECK_EQUAL(std::string{CORE_MODE_NAME}, "Debug");
#ifdef CORE_MODE_RELEASE
        FAIL_CHECK("CORE_MODE_RELEASE must be absent in a debug build");
#endif
#endif
    }
}

int main(int argc, char** argv)
{
    return Test::RunAllTests(argc, argv);
}
