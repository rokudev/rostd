// Copyright 2021-2022 Roku, Inc.
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

#ifndef ROSTD_PRINTX_HPP
#define ROSTD_PRINTX_HPP

#include <concepts>
#include <cstdio>
#include <iterator>
#include <tuple>
#include <type_traits>

namespace rostd {

/**
 * The `printx` namespace implements a type-safe front-end for the `printf`
 * family of functions. The goal is to provide type-safe `printf` at zero
 * marginal cost (in code size and runtime performance) relative to using
 * correct, non-type-safe `printf`. Uses of `printx` functionality should
 * compile away, leaving no residue beyond the underlying `printf`-family
 * function call and its associated format string.
 *
 * The `printx` format string template parameter is transformed into a correct
 * `printf` format string at compile time, deducing format specifiers from the
 * provided types. Available format specifiers are those normally available to
 * `printf`, in which case each specifier is validated against its associated
 * argument's type, or the `%?` specifier, which will substitute correctly for
 * any supported type. Length modifiers are ignored and always deduced in the
 * resulting format string.
 *
 * Any `printf`-like function can be adapted to use these `printx` features.
 * The "formula" used to adapt `printf` can be copied (and perhaps modified
 * only slightly) for other similar functions.
 */
namespace printx {

// This is the special character that indicates to the printx format string
// that it should print whatever type is there.
constexpr inline auto deduce_specifier = '?';

enum Flags : unsigned {
    PromotesToInt   = 0b0001, // varargs promotes to `int`
    PrintsAsPointer = 0b0010, // can be printed as pointer via `%p`
    ForbidPrecision = 0b0100, // precision specifier not allowed
    RecordPosition  = 0b1000, // can be used with `%n`
};

template <typename> struct FmtTraits;

#define PRINTX_FMT_TRAITS \
    XM( bool               , d   , PromotesToInt ) \
    XM( char               , c   , PromotesToInt ) \
    XM( signed char        , hhd , PromotesToInt ) \
    XM( unsigned char      , hhu , PromotesToInt ) \
    XM( short              , hd  , PromotesToInt ) \
    XM( unsigned short     , hu  , PromotesToInt ) \
    XM( int                , d   , PromotesToInt ) \
    XM( unsigned int       , u   , PromotesToInt ) \
    XM( long               , ld  , 0u ) \
    XM( unsigned long      , lu  , 0u ) \
    XM( long long          , lld , 0u ) \
    XM( unsigned long long , llu , 0u ) \
    XM( float              , g   , 0u ) \
    XM( double             , g   , 0u ) \
    XM( long double        , Lg  , 0u ) \
    XM( std::nullptr_t     , p   , PrintsAsPointer ) \
    XM( int*               , p   , PrintsAsPointer | RecordPosition ) \
    XM( char*              , s   , PrintsAsPointer ) \
    XM( char const*        , s   , PrintsAsPointer ) \

#define XM(Type, Spec, Flg) \
    template <> struct FmtTraits<Type> { \
        static constexpr auto spec = #Spec; \
        static constexpr auto flags = Flg; \
    };
PRINTX_FMT_TRAITS
#undef XM

// String literals and character arrays (must be null-terminated)
template <std::size_t Size> struct FmtTraits<char[Size]> : FmtTraits<char*> {};

// Array and pointer types, printed as pointers
template <typename Type>
    requires (std::is_array_v<Type> || std::is_pointer_v<Type>)
struct FmtTraits<Type> : FmtTraits<std::nullptr_t> {};

// Enum types, printed as their underlying integer type
template <typename Type> requires std::is_enum_v<Type>
struct FmtTraits<Type> : FmtTraits<std::underlying_type_t<Type>> {
    static auto fwd_args(Type const& e) {
        return std::tuple{static_cast<std::underlying_type_t<Type>>(e)};
    }
};

// Intrinsic support for types that have a `.c_str()` method such as
// `std::string` and `std::filesystem::path`.
template <typename Str>
    requires requires(Str s) { s.c_str(); }
struct FmtTraits<Str> {
    static auto fwd_args(Str const& arg) {
        return std::tuple{arg.c_str()};
    }
    static constexpr auto spec = "s";
};

template <typename Container>
concept IsContainerOfChar = // container types with a value_type of char
        std::same_as<char, std::remove_cv_t<typename Container::value_type>>;

// Structured to match types like `std::string_view` and `std::vector<char>`
template <IsContainerOfChar Str>
    requires (!requires(Str s) { s.c_str(); } // these are handled separately
            && requires(Str s) { std::data(s); std::size(s); })
struct FmtTraits<Str> {
    static auto fwd_args(Str const& arg) {
        return std::tuple{static_cast<int>(std::size(arg)), std::data(arg)};
    }
    static constexpr auto spec = ".*s";
    // These types need to hijack the field precision specifier, so this will
    // cause an error to be generated if it's used explicitly.
    static constexpr auto flags = ForbidPrecision;
};

// Detect the existence and value of the `flags` trait in a `FmtTraits`.
template <typename Arg>
constexpr auto flags() {
    if constexpr (requires { FmtTraits<Arg>::flags; }) {
        return FmtTraits<Arg>::flags;
    } else {
        return 0u;
    }
}

class Transformer {
public:
    constexpr virtual ~Transformer() = default;

    // If this succeeds, nullptr is returned and `src` points to the end of the
    // input string.
    // If this fails, an error message is returned, and `src` points to the
    // part of the string where the problem was detected.
    template <typename... Args>
    constexpr char const* transform(char const*& src) noexcept {
        return transform_priv<std::remove_cvref_t<Args>...>(src);
    }

private:
    template <typename... Args>
    constexpr char const* transform_priv(char const*& src) noexcept {
        constexpr Specifier specifiers[] = {
            Specifier{FmtTraits<Args>::spec, flags<Args>()}...,
            Specifier{}
        };
        return xform0(src, specifiers);
    }

    // This appends a character to the output:
    constexpr virtual void append(char) noexcept = 0;

    enum Status {
        ConversionLacksType,
        FieldPrecisionNeedsInt,
        FieldPrecisionNotAllowed,
        FieldWidthNeedsInt,
        FormatExpectsChar,
        FormatExpectsIntPtr,
        FormatExpectsPtr,
        FormatInvalidType,
        FormatNotEnoughArgs,
        FormatSpuriousPercent,
        FormatTooManyArgs
    };

    // This is how error messages are communicated. This code must perform
    // error handling and reporting at compile time. Because C++ does not
    // provide a way to truly customize error messages, this code needs to
    // force the compile error on the line of text that it wants to show.
    constexpr char const* error(Status const status) {
        #define PRINTX_ERROR(msg) { \
                if (std::is_constant_evaluated()) throw; \
                return msg; \
            }
        switch (status) {
        case ConversionLacksType:
            PRINTX_ERROR("conversion lacks type at end of format");
        case FieldPrecisionNeedsInt:
            PRINTX_ERROR("field precision specifier '.*' expects int");
        case FieldPrecisionNotAllowed:
            PRINTX_ERROR("field precision specifier not allowed for type");
        case FieldWidthNeedsInt:
            PRINTX_ERROR("field width specifier '*' expects int");
        case FormatExpectsChar:
            PRINTX_ERROR("format %c expects argument of type char");
        case FormatExpectsIntPtr:
            PRINTX_ERROR("format %n expects argument of type int*");
        case FormatExpectsPtr:
            PRINTX_ERROR("format %p expects argument of pointer type");
        case FormatInvalidType:
            PRINTX_ERROR("format expects argument of different type");
        case FormatNotEnoughArgs:
            PRINTX_ERROR("not enough arguments for format");
        case FormatSpuriousPercent:
            PRINTX_ERROR("spurious trailing '%' in format");
        case FormatTooManyArgs:
            PRINTX_ERROR("too many arguments for format");
        }
        return nullptr;
        #undef PRINTX_ERROR
    }

    // This class is used to determine "specifier class" and whether it is
    // compatible with another specifier; used for error checking.
    class SpecClass {
    public:
        constexpr SpecClass(char ch) : v{init(ch)} {}
        constexpr explicit operator bool() const { return v != Invalid; }
        constexpr bool operator==(SpecClass const&) const = default;
    private:
        enum Class { Invalid, String, Integer, Float, Pointer } v;
        static constexpr Class init(char const ch) {
            switch (ch) {
            case 's': return String;
            case 'c': case 'd': case 'i':
            case 'u': case 'o': case 'x': case 'X': return Integer;
            case 'f': case 'F': case 'e': case 'E':
            case 'g': case 'G': case 'a': case 'A': return Float;
            case 'p': case 'n': return Pointer;
            }
            return Invalid;
        }
    };

    struct Specifier {
        char const* spec = nullptr;
        unsigned flags = 0u;
        constexpr explicit operator bool() const { return spec != nullptr; }
    };

    static constexpr bool at_end(char const* const ptr) noexcept
            { return *ptr == '\0'; }

    // Printf format string: %[flags][width][.precision][modifier]specifier
    constexpr char const* xform0(char const*& src, Specifier const*) noexcept;
    constexpr char const* xform1(char const*& src, Specifier const*) noexcept;
};

// The job of this function is to copy text verbatim until it finds a format
// specifier.
constexpr char const* Transformer::xform0(char const*& src,
        Specifier const* spec_array) noexcept {
    while (!at_end(src)) {
        if (*src == '%') {
            append('%');
            if (at_end(++src))
                return error(FormatSpuriousPercent);
            if (*src == '%') { // handle escaped '%', just go around again
                append('%');
                ++src;
                continue;
            }
            return xform1(src, spec_array);
        } else {
            append(*src++);
        }
    }
    return *spec_array ? error(FormatTooManyArgs) : nullptr;
}

// There are potentially 3 arguments that match to a single format specifier:
//   1) flags and the field width specifier ('*' consumes an argument)
//   2) dot and the field precision specifier ('*' consumes an argument)
//   3) length modifiers and type specifier (consumes an argument)
constexpr char const* Transformer::xform1(char const*& src,
        Specifier const* spec_array) noexcept {
    if (!*spec_array) return error(FormatNotEnoughArgs);

    while (true) { // copy any flags directly
        if (at_end(src)) break;
        switch (*src) {
        case '-': case '+': case ' ': case '#': case '0':
            append(*src++);
            continue;
        }
        break;
    }

    for (auto&& field : { FieldWidthNeedsInt, FieldPrecisionNeedsInt }) {
        if (at_end(src)) return error(ConversionLacksType);

        if (field == FieldPrecisionNeedsInt) { // detect/require dot first
            if (*src == '.') {
                if (spec_array->flags & ForbidPrecision)
                    return error(FieldPrecisionNotAllowed);
                append('.');
                if (at_end(++src)) return error(ConversionLacksType);
            } else {
                break; // no field precision specifier
            }
        }

        if (*src == '*') {
            append('*');
            if (at_end(++src)) return error(ConversionLacksType);
            if (!(spec_array->flags & PromotesToInt)) return error(field);
            ++spec_array; // move to the next type
            if (!*spec_array) return error(FormatNotEnoughArgs);
        } else {
            while (true) {
                if (auto const fc = *src; fc >= '0' && fc <= '9') {
                    append(fc);
                    ++src;
                    if (at_end(src)) return error(ConversionLacksType);
                } else {
                    break;
                }
            }
        }
    }

    // Length modifier and specifier are parsed together. Length modifiers are
    // ignored (and replaced, if necessary). This code is only looking for the
    // type specifier.
    for (int i = 1; i <= 3; ++i) { // this could not be more than 3 chars
        if (at_end(src)) return error(ConversionLacksType);
        auto const ch = *src++;
        if (ch == deduce_specifier) {
            for (auto p = spec_array->spec; *p; append(*p++));
        } else if (auto const cl = SpecClass{ch}) {
            if (ch == 'c') { // %c takes no modifiers
                if (!(spec_array->flags & PromotesToInt))
                    return error(FormatExpectsChar);
            } else if (ch == 'n') { // %n takes no modifiers
                if (!(spec_array->flags & RecordPosition))
                    return error(FormatExpectsIntPtr);
            } else if (ch == 'p') { // %p takes no modifiers
                if (!(spec_array->flags & PrintsAsPointer))
                    return error(FormatExpectsPtr);
            } else {
                // Modifier is all but the last char of the format specifier.
                auto p = spec_array->spec;
                for (auto next = p + 1; *next; ++next) append(*p++);
                if (cl != *p) return error(FormatInvalidType);
            }
            append(ch);
        } else {
            continue;
        }
        ++spec_array; // move to the next type
        return xform0(src, spec_array);
    }

    return error(ConversionLacksType);
}

// This counts the exact number of bytes that the transformed string will use.
// (Does NOT include any null-terminator that may be needed.)
struct CountingTransformer : Transformer {
    std::size_t count = 0;
    constexpr ~CountingTransformer() override {}
    constexpr void append(char) noexcept override { ++count; }
};

class AppendingTransformer : public Transformer {
public:
    constexpr AppendingTransformer(char* out) : out{out} {}
    constexpr ~AppendingTransformer() override {}
private:
    constexpr void append(char c) noexcept override { *out++ = c; }
    char* out;
};

template <typename... Args>
constexpr std::size_t count_size(char const* str) {
    auto cx = CountingTransformer{};
    cx.transform<Args...>(str);
    return cx.count;
}

namespace { // anonymous namespace inhibits symbol table entries

template <std::size_t Size>
struct FmtLiteral {
    consteval FmtLiteral() = default;
    consteval FmtLiteral(char const (&str)[Size]) {
        for (std::size_t i = 0; i < Size; ++i) data[i] = str[i];
    }
    char data[Size] = {};
};

} // anonymous namespace

// A FmtTraits<> that has its own fwd_args() function is allowed to override
// default behavior (which is just to pass the value through to the argument
// list).
template <typename Arg>
    requires requires(Arg arg) { FmtTraits<Arg>::fwd_args(arg); }
auto fwd_args(Arg const& arg) {
    return FmtTraits<Arg>::fwd_args(arg);
}

template <typename Arg>
auto fwd_args(Arg const& arg) {
    return std::tuple{arg};
}

template <FmtLiteral Fmt, typename... Args>
consteval auto build_fmt() noexcept {
    auto buffer = FmtLiteral<count_size<Args...>(Fmt.data) + 1>{};
    auto src = Fmt.data;
    AppendingTransformer{buffer.data}.transform<Args...>(src);
    return buffer;
}

template <typename Function, typename... Args>
decltype(auto) call_func(Function const& call, Args const&... args) {
    if constexpr (sizeof...(args) == 0) return call();
    else return std::apply(call, std::tuple_cat(fwd_args(args)...));
}

} // namespace printx

#if defined(__GNUC__) || defined(__clang__)
    // These functions send what appear to the compiler to be non-literals to
    // `printf`-family calls. Disable these warnings in order to compile
    // cleanly. Because the format strings are validated by `printx`, these
    // checks are redundant.
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif

// Note the formula used here. Adapting existing `printf`-family functionality
// to use this feature should always remain as similar to this as possible.

template <printx::FmtLiteral Fmt, typename... Args>
int printf(Args const&... args) noexcept {
    auto call = [&](auto const&... args) {
        static constexpr auto f = printx::build_fmt<Fmt, Args...>();
        return std::printf(f.data, args...);
    };
    return printx::call_func(call, args...);
}

template <printx::FmtLiteral Fmt, typename... Args>
int fprintf(FILE* stream, Args const&... args) noexcept {
    auto call = [&](auto const&... args) {
        static constexpr auto f = printx::build_fmt<Fmt, Args...>();
        return std::fprintf(stream, f.data, args...);
    };
    return printx::call_func(call, args...);
}

template <printx::FmtLiteral Fmt, typename... Args>
int snprintf(char* s, std::size_t n, Args const&... args) noexcept {
    auto call = [&](auto const&... args) {
        static constexpr auto f = printx::build_fmt<Fmt, Args...>();
        return std::snprintf(s, n, f.data, args...);
    };
    return printx::call_func(call, args...);
}

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

} // namespace rostd

#endif // ROSTD_PRINTX_HPP
