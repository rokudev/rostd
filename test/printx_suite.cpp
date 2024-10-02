/*
 * Copyright (c) 2021-2022 Roku, Inc. All rights reserved.
 * This software and any compilation or derivative thereof is, and shall
 * remain, the proprietary information of Roku, Inc. and is highly confidential
 * in nature.
 */
#include "test.hpp"
#include <rostd/printx.hpp>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

enum EnumTest1 : int {};
enum class EnumTest2 : unsigned long {};
enum class EnumTest3 : short {};
enum class EnumTest4 {};

namespace rostd::printx::detail {
template <> struct traits<EnumTest4> {
    static constexpr auto spec = "s";
};
} // namespace rostd::printx::detail

namespace printx_suite {
namespace { // anonymous
namespace compile_time_unit_tests {

using namespace rostd::printx;

consteval bool fmteq(std::string_view x, std::string_view y) {
    return x == y;
}

struct CustomCStr {
    char const* c_str() const { return {}; }
};

#define ASSERT(From, Type, To) \
    static_assert(fmteq(build_fmt<From, Type>().data, To))

ASSERT("%?",       bool,                   "%d");
ASSERT("%?",       char,                   "%c");
ASSERT("%?",       signed char,            "%hhd");
ASSERT("%?",       unsigned char,          "%hhu");
ASSERT("%?",       short,                  "%hd");
ASSERT("%?",       unsigned short,         "%hu");
ASSERT("%?",       int,                    "%d");
ASSERT("%?",       unsigned,               "%u");
ASSERT("%?",       long,                   "%ld");
ASSERT("%?",       unsigned long,          "%lu");
ASSERT("%?",       long long,              "%lld");
ASSERT("%?",       unsigned long long,     "%llu");
ASSERT("%?",       float,                  "%g");
ASSERT("%?",       double,                 "%g");
ASSERT("%?",       long double,            "%Lg");
ASSERT("%?",       void*,                  "%p");
ASSERT("%?",       void const*,            "%p");
ASSERT("%?",       int*,                   "%p");
ASSERT("%?",       std::nullptr_t,         "%p");
ASSERT("%?",       char*,                  "%s");
ASSERT("%?",       char const*,            "%s");
ASSERT("%?",       char[6],                "%s");
ASSERT("%?",       char const[6],          "%s");
ASSERT("%?",       char const (&)[6],      "%s");
ASSERT("%?",       std::string,            "%s");
ASSERT("%?",       std::filesystem::path,  "%s");
ASSERT("%?",       CustomCStr,             "%s");
ASSERT("%?",       std::string_view,       "%.*s");
ASSERT("%?",       std::span<char>,        "%.*s");
ASSERT("%?",       std::vector<char>,      "%.*s");

ASSERT("%d",       bool,                   "%d");
ASSERT("%c",       char,                   "%c");
ASSERT("%hhd",     signed char,            "%hhd");
ASSERT("%hhu",     unsigned char,          "%hhu");
ASSERT("%hd",      short,                  "%hd");
ASSERT("%hu",      unsigned short,         "%hu");
ASSERT("%d",       int,                    "%d");
ASSERT("%u",       unsigned,               "%u");
ASSERT("%ld",      long,                   "%ld");
ASSERT("%lu",      unsigned long,          "%lu");
ASSERT("%lld",     long long,              "%lld");
ASSERT("%llu",     unsigned long long,     "%llu");
ASSERT("%e",       float,                  "%e");
ASSERT("%a",       double,                 "%a");
ASSERT("%Lg",      long double,            "%Lg");
ASSERT("%p",       void*,                  "%p");
ASSERT("%p",       void const*,            "%p");
ASSERT("%p",       int*,                   "%p");
ASSERT("%p",       std::nullptr_t,         "%p");
ASSERT("%s",       char*,                  "%s");
ASSERT("%s",       char const*,            "%s");
ASSERT("%s",       char[6],                "%s");
ASSERT("%s",       char const[6],          "%s");
ASSERT("%s",       char const (&)[6],      "%s");
ASSERT("%s",       std::string,            "%s");
ASSERT("%s",       std::filesystem::path,  "%s");
ASSERT("%s",       CustomCStr,             "%s");
ASSERT("%s",       std::string_view,       "%.*s");
ASSERT("%s",       std::span<char>,        "%.*s");
ASSERT("%s",       std::vector<char>,      "%.*s");
ASSERT("%n",       int*,                   "%n");

ASSERT("%c",       signed char,            "%c");
ASSERT("%c",       signed short,           "%c");
ASSERT("%c",       signed,                 "%c");
//ASSERT("%c",       signed long,            "%c"); // should error
//ASSERT("%c",       signed long long,       "%c"); // should error
ASSERT("%d",       signed char,            "%hhd");
ASSERT("%d",       signed short,           "%hd");
ASSERT("%d",       signed,                 "%d");
ASSERT("%d",       signed long,            "%ld");
ASSERT("%d",       signed long long,       "%lld");
ASSERT("%o",       signed char,            "%hho");
ASSERT("%o",       signed short,           "%ho");
ASSERT("%o",       signed,                 "%o");
ASSERT("%o",       signed long,            "%lo");
ASSERT("%o",       signed long long,       "%llo");
ASSERT("%u",       signed char,            "%hhu");
ASSERT("%u",       signed short,           "%hu");
ASSERT("%u",       signed,                 "%u");
ASSERT("%u",       signed long,            "%lu");
ASSERT("%u",       signed long long,       "%llu");
ASSERT("%x",       signed char,            "%hhx");
ASSERT("%x",       signed short,           "%hx");
ASSERT("%x",       signed,                 "%x");
ASSERT("%x",       signed long,            "%lx");
ASSERT("%x",       signed long long,       "%llx");
ASSERT("%X",       signed char,            "%hhX");
ASSERT("%X",       signed short,           "%hX");
ASSERT("%X",       signed,                 "%X");
ASSERT("%X",       signed long,            "%lX");
ASSERT("%X",       signed long long,       "%llX");

ASSERT("%c",       unsigned char,          "%c");
ASSERT("%c",       unsigned short,         "%c");
ASSERT("%c",       unsigned,               "%c");
//ASSERT("%c",       unsigned long,          "%c"); // should error
//ASSERT("%c",       unsigned long long,     "%c"); // should error
ASSERT("%d",       unsigned char,          "%hhd");
ASSERT("%d",       unsigned short,         "%hd");
ASSERT("%d",       unsigned,               "%d");
ASSERT("%d",       unsigned long,          "%ld");
ASSERT("%d",       unsigned long long,     "%lld");
ASSERT("%o",       unsigned char,          "%hho");
ASSERT("%o",       unsigned short,         "%ho");
ASSERT("%o",       unsigned,               "%o");
ASSERT("%o",       unsigned long,          "%lo");
ASSERT("%o",       unsigned long long,     "%llo");
ASSERT("%u",       unsigned char,          "%hhu");
ASSERT("%u",       unsigned short,         "%hu");
ASSERT("%u",       unsigned,               "%u");
ASSERT("%u",       unsigned long,          "%lu");
ASSERT("%u",       unsigned long long,     "%llu");
ASSERT("%x",       unsigned char,          "%hhx");
ASSERT("%x",       unsigned short,         "%hx");
ASSERT("%x",       unsigned,               "%x");
ASSERT("%x",       unsigned long,          "%lx");
ASSERT("%x",       unsigned long long,     "%llx");
ASSERT("%X",       unsigned char,          "%hhX");
ASSERT("%X",       unsigned short,         "%hX");
ASSERT("%X",       unsigned,               "%X");
ASSERT("%X",       unsigned long,          "%lX");
ASSERT("%X",       unsigned long long,     "%llX");

ASSERT("%f",       float,                  "%f");
ASSERT("%f",       double,                 "%f");
ASSERT("%f",       long double,            "%Lf");
ASSERT("%e",       float,                  "%e");
ASSERT("%e",       double,                 "%e");
ASSERT("%e",       long double,            "%Le");
ASSERT("%g",       float,                  "%g");
ASSERT("%g",       double,                 "%g");
ASSERT("%g",       long double,            "%Lg");
ASSERT("%a",       float,                  "%a");
ASSERT("%a",       double,                 "%a");
ASSERT("%a",       long double,            "%La");
ASSERT("%F",       float,                  "%F");
ASSERT("%F",       double,                 "%F");
ASSERT("%F",       long double,            "%LF");
ASSERT("%E",       float,                  "%E");
ASSERT("%E",       double,                 "%E");
ASSERT("%E",       long double,            "%LE");
ASSERT("%G",       float,                  "%G");
ASSERT("%G",       double,                 "%G");
ASSERT("%G",       long double,            "%LG");
ASSERT("%A",       float,                  "%A");
ASSERT("%A",       double,                 "%A");
ASSERT("%A",       long double,            "%LA");

// Pointer-to-char can be printed via %p as well:
ASSERT("%p",       char*,                  "%p");
ASSERT("%p",       char const*,            "%p");
ASSERT("%p",       char[6],                "%p");
ASSERT("%p",       char const[6],          "%p");
ASSERT("%p",       char const (&)[6],      "%p");

// Arrays (except char[]) are deduced to %p:
ASSERT("%?",       long[6],                "%p");
ASSERT("%?",       float const[6],         "%p");
ASSERT("%?",       short const (&)[6],     "%p");

ASSERT("%03?",     int,                    "%03d");
ASSERT("%.4?",     int,                    "%.4d");
ASSERT("%-20?",    unsigned long,          "%-20lu");
ASSERT("%-20.4?",  unsigned long long,     "%-20.4llu");

ASSERT("%?",       EnumTest1,              "%d");
ASSERT("%?",       EnumTest2,              "%lu");
ASSERT("%?",       EnumTest3,              "%hd");
ASSERT("%?",       EnumTest4,              "%s");

#undef ASSERT

static_assert(fmteq(build_fmt<"no args">().data, "no args"));
static_assert(fmteq(build_fmt<"%% %%">().data, "%% %%"));

// Guarantee width and precision specifiers work with `int`
static_assert(fmteq(build_fmt<"%*?", int, char*>().data, "%*s"));
static_assert(fmteq(build_fmt<"%*?", int, char*>().data, "%*s"));
static_assert(fmteq(build_fmt<"%*?", int, char*>().data, "%*s"));
static_assert(fmteq(build_fmt<"%.*?", int, char*>().data, "%.*s"));
static_assert(fmteq(build_fmt<"%.*?", int, char*>().data, "%.*s"));
static_assert(fmteq(build_fmt<"%*.*?", int, int, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", int, int, char*>().data, "%*.*s"));

// Guarantee width and precision specifiers work with anything that promotes to
// an `int`.
static_assert(fmteq(build_fmt<"%*.*?", bool, bool, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", char, char, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", signed char, signed char, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", unsigned char, unsigned char, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", short, short, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", unsigned short, unsigned short, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", int, int, char*>().data, "%*.*s"));
static_assert(fmteq(build_fmt<"%*.*?", unsigned int, unsigned int, char*>().data, "%*.*s"));

static_assert(fmteq(build_fmt<"a %? b %x c %% d %? e\n", double, unsigned long,
                            char const*>()
                            .data,
        "a %g b %lx c %% d %s e\n"));

// Ensure the width specifier can still be used with `std::string_view`:
static_assert(fmteq(build_fmt<"%*?", int, std::string_view>().data, "%*.*s"));

} // namespace compile_time_unit_tests
} // anonymous namespace
} // namespace printx_suite

int main() {
    using namespace std::literals;
    static constexpr auto buffer_size = std::size_t{1024};

    { // Check %n functionality.
        auto buf = std::vector<char>(buffer_size, '\0');
        auto pos = int{};
        auto const size = rostd::sprintf<"Size and pos should be equal.%n">
                                        (buf, &pos);
        assert(size == pos);
        assert(buf[size] == '\0');
    }

    { // Guarantee that packed fields can bind to snprintf function parameters:
        auto buf = std::array<char, buffer_size>{};
        struct Packed {
            unsigned a:2;
            short    b:12;
            int      c:3;
            bool     d:1;
        };
        auto p = Packed{3, -2000, 3, 1};
        rostd::sprintf<"%d %? %d %?">(buf, p.a, p.b, p.c, p.d);
        assert(buf.data() == std::string_view{"3 -2000 3 1"});
    }

    char buf[buffer_size] = {};

#define CHECK_CMP(Val, Fmt, Output) \
    { \
        rostd::snprintf<Fmt>(buf, sizeof buf, Val); \
        assert(std::string_view{buf} == Output); \
    }
#define CHECK_NEQ(Val, Fmt, Output) \
    { \
        rostd::snprintf<Fmt>(buf, sizeof buf, Val); \
        assert(std::string_view{buf} != Output); \
    }

    CHECK_CMP("Just a string"  , "%?", "Just a string");
    CHECK_CMP("Just a string"s , "%?", "Just a string");
    CHECK_CMP("Just a string"sv, "%?", "Just a string");
    CHECK_CMP("Just a string"  , "%s", "Just a string");
    CHECK_CMP("Just a string"s , "%s", "Just a string");
    CHECK_CMP("Just a string"sv, "%s", "Just a string");
    CHECK_CMP(1234567,           "%?", "1234567");
    CHECK_CMP(1234567,           "%d", "1234567");
    CHECK_CMP(1234567,           "%i", "1234567");
    CHECK_CMP(1234567,           "%u", "1234567");
    CHECK_CMP(-1234567,          "%?", "-1234567");
    CHECK_CMP(-1234567,          "%d", "-1234567");
    CHECK_CMP(-1234567,          "%i", "-1234567");
    CHECK_NEQ(-1234567,          "%u", "-1234567");
    CHECK_CMP(INT64_MAX,         "%?", "9223372036854775807");
    CHECK_CMP(INT64_MAX,         "%d", "9223372036854775807");
    CHECK_CMP(INT64_MAX,         "%i", "9223372036854775807");
    CHECK_CMP(INT64_MAX,         "%u", "9223372036854775807");
    CHECK_CMP(INT64_MAX,         "%o", "777777777777777777777");
    CHECK_CMP(INT64_MAX,         "%x", "7fffffffffffffff");
    CHECK_CMP(INT64_MAX,         "%X", "7FFFFFFFFFFFFFFF");
    CHECK_CMP(INT64_MIN,         "%?", "-9223372036854775808");
    CHECK_CMP(INT64_MIN,         "%d", "-9223372036854775808");
    CHECK_CMP(INT64_MIN,         "%i", "-9223372036854775808");
    CHECK_NEQ(INT64_MIN,         "%u", "-9223372036854775808");
    CHECK_CMP(UINT64_MAX,        "%?", "18446744073709551615");
    CHECK_NEQ(UINT64_MAX,        "%d", "18446744073709551615");
    CHECK_NEQ(UINT64_MAX,        "%i", "18446744073709551615");
    CHECK_CMP(UINT64_MAX,        "%u", "18446744073709551615");
    CHECK_CMP(UINT64_MAX,        "%o", "1777777777777777777777");
    CHECK_CMP(UINT64_MAX,        "%x", "ffffffffffffffff");
    CHECK_CMP(UINT64_MAX,        "%X", "FFFFFFFFFFFFFFFF");
    CHECK_CMP('a',               "%c", "a");

    CHECK_CMP("right",           "%10?",  "     right");
    CHECK_CMP("left",            "%-10?", "left      ");
    CHECK_CMP("right",           "%10.2?",  "        ri");
    CHECK_CMP("left",            "%-10.2?", "le        ");
    CHECK_CMP("right",           "%10s",  "     right");
    CHECK_CMP("left",            "%-10s", "left      ");
    CHECK_CMP("right",           "%10.2s",  "        ri");
    CHECK_CMP("left",            "%-10.2s", "le        ");
}
