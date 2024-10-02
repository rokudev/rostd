// Copyright 2021-2024 Roku, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// clang-format off

#ifndef ROSTD_PRINTX_HPP
#define ROSTD_PRINTX_HPP

#include <concepts>
#include <cstdio>
#include <iterator>
#include <tuple>
#include <type_traits>

namespace rostd {

/**
 * The `printx` namespace provides utilities that facilitate true type-safe
 * `printf` functionality at zero marginal cost (in code size and runtime
 * performance) relative to using correct, non-type-safe `printf`. Uses of
 * `printx` will compile away, leaving no residue beyond the underlying
 * `printf`-family function call and its associated format string.
 *
 * The `printx` format string template parameter is transformed into a correct
 * standard `printf` format string at compile time. Standard format strings are
 * fully supported, though length sub-specifiers are unnecessary and inferred
 * automatically, and the additional `%?` specifier is also provided, which
 * will substitute correctly for any supported type.
 */
namespace printx {
namespace detail {

enum : unsigned {
    promotes_to_int   = 0b0001, // varargs promotes to `int`
    prints_as_pointer = 0b0010, // can be printed as pointer via `%p`
    forbid_precision  = 0b0100, // precision specifier not allowed
    record_position   = 0b1000, // can be used with `%n`
};

template <typename> struct traits;

#define PRINTX_FMT_TRAITS \
    XM( bool               , d   , promotes_to_int ) \
    XM( char               , c   , promotes_to_int ) \
    XM( signed char        , hhd , promotes_to_int ) \
    XM( unsigned char      , hhu , promotes_to_int ) \
    XM( short              , hd  , promotes_to_int ) \
    XM( unsigned short     , hu  , promotes_to_int ) \
    XM( int                , d   , promotes_to_int ) \
    XM( unsigned int       , u   , promotes_to_int ) \
    XM( long               , ld  , 0u ) \
    XM( unsigned long      , lu  , 0u ) \
    XM( long long          , lld , 0u ) \
    XM( unsigned long long , llu , 0u ) \
    XM( float              , g   , 0u ) \
    XM( double             , g   , 0u ) \
    XM( long double        , Lg  , 0u ) \
    XM( std::nullptr_t     , p   , prints_as_pointer ) \
    XM( int*               , p   , prints_as_pointer | record_position ) \
    XM( char*              , s   , prints_as_pointer ) \
    XM( char const*        , s   , prints_as_pointer ) \

#define XM(Type, Spec, Flg) \
    template <> struct traits<Type> { \
        static constexpr auto spec = #Spec; \
        static constexpr auto flags = Flg; \
    };
PRINTX_FMT_TRAITS
#undef XM

// String literals and character arrays (must be null-terminated)
template <std::size_t Size> struct traits<char[Size]> : traits<char*> {};

// Array and pointer types, printed as pointers
template <typename Type>
    requires (std::is_array_v<Type> || std::is_pointer_v<Type>)
struct traits<Type> : traits<std::nullptr_t> {};

// Enum types, printed as their underlying integer type
template <typename Type> requires std::is_enum_v<Type>
struct traits<Type> : traits<std::underlying_type_t<Type>> {
    static auto fwd_args(Type const& e) {
        return std::tuple{static_cast<std::underlying_type_t<Type>>(e)};
    }
};

// Intrinsic support for types that have a `.c_str()` method such as
// `std::string` and `std::filesystem::path`.
template <typename Str>
    requires requires(Str s) { s.c_str(); }
struct traits<Str> {
    static auto fwd_args(Str const& arg) {
        return std::tuple{arg.c_str()};
    }
    static constexpr auto spec = "s";
};

namespace concepts {

template <typename Container>
concept container_of_char = // container types with a value_type of char
        std::same_as<char, std::remove_cv_t<typename Container::value_type>>;

} // namespace concepts

// Structured to match types like `std::string_view` and `std::vector<char>`
template <concepts::container_of_char Str>
    requires (!requires(Str s) { s.c_str(); } // these are handled separately
            && requires(Str s) { std::data(s); std::size(s); })
struct traits<Str> {
    static auto fwd_args(Str const& arg) {
        return std::tuple{static_cast<int>(std::size(arg)), std::data(arg)};
    }
    static constexpr auto spec = ".*s";
    // These types need to hijack the field precision specifier, so this will
    // cause an error to be generated if it's used explicitly.
    static constexpr auto flags = forbid_precision;
};

// Detect the existence and value of the `flags` trait in a `traits`.
template <typename Arg>
constexpr auto flags() {
    if constexpr (requires { traits<Arg>::flags; }) {
        return traits<Arg>::flags;
    } else {
        return 0u;
    }
}

enum class status {
    correct,
    conversion_lacks_type,
    field_precision_needs_int,
    field_precision_not_allowed,
    field_width_needs_int,
    format_expects_char,
    format_expects_int_ptr,
    format_expects_ptr,
    format_invalid_type,
    format_not_enough_args,
    format_spurious_percent,
    format_too_many_args
};

// This is how error messages are communicated. This code must perform error
// handling and reporting at compile time. Because C++ does not provide a way
// to truly customize error messages, this code needs to force the compile
// error on the line of text that it wants to show.
constexpr char const* check_error(status const st) {
    #define PRINTX_ERROR(msg) { \
            if (std::is_constant_evaluated()) throw; \
            return msg; \
        }
    switch (st) {
    case status::correct: break;
    case status::conversion_lacks_type:
        PRINTX_ERROR("conversion lacks type at end of format");
    case status::field_precision_needs_int:
        PRINTX_ERROR("field precision specifier '.*' expects int");
    case status::field_precision_not_allowed:
        PRINTX_ERROR("field precision specifier not allowed for type");
    case status::field_width_needs_int:
        PRINTX_ERROR("field width specifier '*' expects int");
    case status::format_expects_char:
        PRINTX_ERROR("format %c expects argument of type char");
    case status::format_expects_int_ptr:
        PRINTX_ERROR("format %n expects argument of type int*");
    case status::format_expects_ptr:
        PRINTX_ERROR("format %p expects argument of pointer type");
    case status::format_invalid_type:
        PRINTX_ERROR("format expects argument of different type");
    case status::format_not_enough_args:
        PRINTX_ERROR("not enough arguments for format");
    case status::format_spurious_percent:
        PRINTX_ERROR("spurious trailing '%' in format");
    case status::format_too_many_args:
        PRINTX_ERROR("too many arguments for format");
    }
    return nullptr;
    #undef PRINTX_ERROR
}

class transformer {
public:
    constexpr virtual ~transformer() = default;

    // On success, `status::correct` is returned and `src` points to the end of
    // the input string. On failure, an error status is returned, and `src`
    // points to the part of the string where the problem was detected.
    template <typename... Args>
    constexpr status transform(char const*& src) noexcept {
        return transform_priv<std::remove_cvref_t<Args>...>(src);
    }

private:
    template <typename... Args>
    constexpr status transform_priv(char const*& src) noexcept {
        constexpr specifier specifiers[] = {
            specifier{traits<Args>::spec, flags<Args>()}...,
            specifier{}
        };
        return find_specifier(src, specifiers);
    }

    // This appends a character to the output:
    constexpr virtual void append(char) = 0;

    // This class is used to determine "specifier class" and whether it is
    // compatible with another specifier; used for error checking.
    class specifier_class {
    public:
        constexpr specifier_class(char ch) : v{init(ch)} {}
        constexpr explicit operator bool() const { return v != invalid; }
        constexpr bool operator==(specifier_class const&) const = default;
    private:
        enum category { invalid, string, integer, floating_point, pointer } v;
        static constexpr category init(char const ch) {
            switch (ch) {
            case 's': return string;
            case 'c': case 'd': case 'i':
            case 'u': case 'o': case 'x': case 'X': return integer;
            case 'f': case 'F': case 'e': case 'E':
            case 'g': case 'G': case 'a': case 'A': return floating_point;
            case 'p': case 'n': return pointer;
            }
            return invalid;
        }
    };

    struct specifier {
        char const* spec = nullptr;
        unsigned flags = 0u;
        constexpr explicit operator bool() const { return spec != nullptr; }
    };

    static constexpr bool at_end(char const* const ptr) noexcept
            { return *ptr == '\0'; }

    // Printf format string: %[flags][width][.precision][length]specifier
    constexpr status find_specifier(char const*& src,
                                    specifier const*) noexcept;
    constexpr status transform_specifier(char const*& src,
                                         specifier const*) noexcept;
};

// The job of this function is to copy text verbatim until it finds a format
// specifier.
constexpr status transformer::find_specifier(char const*& src,
        specifier const* spec_array) noexcept {
    while (!at_end(src)) {
        if (*src == '%') {
            append('%');
            if (at_end(++src))
                return status::format_spurious_percent;
            if (*src == '%') { // handle escaped '%', just go around again
                append(*src++);
                continue;
            }
            return transform_specifier(src, spec_array);
        } else {
            append(*src++);
        }
    }
    return *spec_array ? status::format_too_many_args : status::correct;
}

// There are potentially 3 arguments that match to a single format specifier:
//   1) flags and the field width specifier ('*' consumes an argument)
//   2) dot and the field precision specifier ('*' consumes an argument)
//   3) length sub-specifier and type specifier (consumes an argument)
constexpr status transformer::transform_specifier(char const*& src,
        specifier const* spec_array) noexcept {
    if (!*spec_array) return status::format_not_enough_args;

    while (!at_end(src)) { // copy any flags directly
        switch (*src) {
        case '-': case '+': case ' ': case '#': case '0':
            append(*src++);
            continue;
        }
        break;
    }

    for (auto&& field : { status::field_width_needs_int,
                          status::field_precision_needs_int }) {
        if (at_end(src)) return status::conversion_lacks_type;

        if (field == status::field_precision_needs_int) { // require dot first
            if (*src == '.') {
                if (spec_array->flags & forbid_precision)
                    return status::field_precision_not_allowed;
                append('.');
                if (at_end(++src)) return status::conversion_lacks_type;
            } else {
                break; // no field precision specifier
            }
        }

        if (*src == '*') {
            append('*');
            if (at_end(++src)) return status::conversion_lacks_type;
            if (!(spec_array->flags & promotes_to_int)) return field;
            ++spec_array; // move to the next type
            if (!*spec_array) return status::format_not_enough_args;
            if (field == status::field_precision_needs_int) {
                if (spec_array->flags & forbid_precision)
                    return status::field_precision_not_allowed;
            }
        } else {
            while (*src >= '0' && *src <= '9') {
                append(*src);
                if (at_end(++src)) return status::conversion_lacks_type;
            }
        }
    }

    // Length sub-specifier and type specifier are parsed together. Length
    // sub-specifiers are ignored (and replaced as necessary), and this is
    // equipped to ignore non-standard sub-specifiers as well, such as I32/I64.
    // This code is only looking for the type specifier.
    for (int i = 1; i <= 4; ++i) { // this could not be more than 4 chars
        if (at_end(src)) return status::conversion_lacks_type;
        auto const ch = *src++;
        if (ch == '?') {
            // This is the special character that indicates that the format
            // specifier should be deduced.
            for (auto p = spec_array->spec; *p; append(*p++)) {}
        } else if (auto const cl = specifier_class{ch}) {
            if (ch == 'c') { // %c takes no sub-specifiers
                if (!(spec_array->flags & promotes_to_int))
                    return status::format_expects_char;
            } else if (ch == 'n') { // %n takes no sub-specifiers
                if (!(spec_array->flags & record_position))
                    return status::format_expects_int_ptr;
            } else if (ch == 'p') { // %p takes no sub-specifiers
                if (!(spec_array->flags & prints_as_pointer))
                    return status::format_expects_ptr;
            } else {
                // Sub-specifier is all but the last char of the specifier.
                auto p = spec_array->spec;
                for (auto next = p + 1; *next; ++next) append(*p++);
                if (cl != *p) return status::format_invalid_type;
            }
            append(ch);
        } else {
            continue;
        }
        ++spec_array; // move to the next type
        return find_specifier(src, spec_array);
    }

    return status::conversion_lacks_type;
}

// This counts the exact number of bytes that the transformed string will use.
// (Does NOT include any null-terminator that may be needed.)
struct counting_transformer : transformer {
    std::size_t count = 0;
    constexpr ~counting_transformer() override = default;
    constexpr void append(char) override { ++count; }
};

class appending_transformer : public transformer {
public:
    constexpr appending_transformer(char* out) : out{out} {}
    constexpr ~appending_transformer() override = default;
private:
    constexpr void append(char c) override { *out++ = c; }
    char* out;
};

template <typename... Args>
constexpr std::size_t count_size(char const* str) {
    auto cx = counting_transformer{};
    cx.transform<Args...>(str);
    return cx.count;
}

// A traits<> that has its own fwd_args() function is allowed to override
// default behavior (which is just to pass the value through to the argument
// list).
template <typename Arg>
    requires requires(Arg arg) { traits<Arg>::fwd_args(arg); }
auto fwd_args(Arg const& arg) {
    return traits<Arg>::fwd_args(arg);
}

template <typename Arg>
auto fwd_args(Arg const& arg) {
    return std::tuple{arg};
}

} // namespace detail

namespace { // anonymous, internal linkage always
template <std::size_t Size>
struct literal {
    consteval literal() = default;
    consteval literal(char const (&str)[Size]) {
        for (std::size_t i = 0; i < Size; ++i) data[i] = str[i];
    }
    char data[Size] = {};
};
} // anonymous namespace

template <literal Fmt, typename... Args>
consteval auto build_fmt() noexcept {
    using namespace printx::detail;
    auto buffer = literal<count_size<Args...>(Fmt.data) + 1>{};
    auto src = Fmt.data;
    auto const st = appending_transformer{buffer.data}.transform<Args...>(src);
    check_error(st);
    return buffer;
}

template <typename Function, typename... Args>
decltype(auto) invoke(Function const& call, Args const&... args) {
    if constexpr (sizeof...(args) == 0) return call();
    else return std::apply(call, std::tuple_cat(detail::fwd_args(args)...));
}

} // namespace printx

#if defined(__GNUC__) || defined(__clang__)
    // These functions send what appear to the compiler to be non-literals to
    // `printf`-family calls. Disable these warnings in order to compile
    // cleanly. Because the format strings are validated by `printx`, these
    // checks are redundant.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif

template <printx::literal Fmt, typename... Args>
[[gnu::always_inline, gnu::flatten]] inline
int printf(Args const&... args) noexcept {
    return printx::invoke([](auto const&... args) {
            static constexpr auto fmt = printx::build_fmt<Fmt, Args...>();
            return std::printf(fmt.data, args...);
        }, args...);
}

template <printx::literal Fmt, typename Stream, typename... Args>
[[gnu::always_inline, gnu::flatten]] inline
int fprintf(Stream const& stream, Args const&... args) noexcept {
    return printx::invoke([&](auto const&... args) {
            static constexpr auto fmt = printx::build_fmt<Fmt, Args...>();
            return std::fprintf(stream, fmt.data, args...);
        }, args...);
}

template <printx::literal Fmt, typename... Args>
[[gnu::always_inline, gnu::flatten]] inline
int snprintf(char* s, std::size_t n, Args const&... args) noexcept {
    return printx::invoke([&](auto const&... args) {
            static constexpr auto fmt = printx::build_fmt<Fmt, Args...>();
            return std::snprintf(s, n, fmt.data, args...);
        }, args...);
}

template <printx::literal Fmt, typename Buffer, typename... Args>
    requires requires(Buffer b) { std::data(b); std::size(b); }
[[gnu::always_inline, gnu::flatten]] inline
int sprintf(Buffer&& buffer, Args const&... args) noexcept {
    return printx::invoke([&](auto const&... args) {
            static constexpr auto fmt = printx::build_fmt<Fmt, Args...>();
            return std::snprintf(std::data(buffer), std::size(buffer),
                    fmt.data, args...);
        }, args...);
}

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

} // namespace rostd

#endif // ROSTD_PRINTX_HPP
