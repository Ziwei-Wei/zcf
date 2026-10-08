// SPDX-License-Identifier: MIT
// Copyright (c) zcf formatter coverage authors.
// ============================================================================
// Format style: file-level section comments and include ordering.
// Language feature: C++23 standard-library surface plus local project includes.
// ============================================================================
//
// Composite input for the C++ style behavior supported by zcf:
// clang-format configuration, uncrustify C++ spacing/newline rules, the
// tree-sitter/integer-literal normalizer passes, C++23 syntax/library surfaces,
// and compiler-gated C++26 language surfaces listed by cppreference.
#include "zeta.h"
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <compare>
#include <complex>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <expected>
#include <filesystem>
#include <flat_map>
#include <flat_set>
#include <format>
#include <fstream>
#include <functional>
#include <generator>
#include <ios>
#include <iostream>
#include <mdspan>
#include <memory>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <print>
#include <queue>
#include <ranges>
#include <regex>
#include <source_location>
#include <span>
#include <spanstream>
#include <stack>
#include <stacktrace>
#include <stdfloat>
#include <stdatomic.h>
#include <string>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <typeinfo>
#include <type_traits>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include "alpha.h"

// ============================================================================
// Format style: preprocessor directives and directive comments.
// Language feature: modules, C++23 #warning, every standard conditional
// directive form, includes, pragmas, #undef, #line, null directives, and nested
// conditional compilation blocks.
// ============================================================================
//
// The disabled module block is intentional: it lets the formatter see import
// declarations and #warning without requiring a compiler/toolchain module setup.
#if 0 // trailing comment after a disabled conditional directive
module;
import std;
import std.compat;
export module zcf.coverage;
export import std;
module: private;
#warning "C++23 formatting coverage includes the new #warning directive" // trailing comment after #warning
#endif // trailing comment after the disabled module block

#if 0                                                  // trailing comment after a second disabled conditional directive
#pragma once                                           // trailing comment after #pragma once
#include <vector>                                      // trailing comment after an angle-header #include
#include "alpha.h"                                     // trailing comment after a quoted-header #include
#// trailing comment after a null preprocessing directive
#ifndef ZCF_DISABLED_HEADER_GUARD_EXAMPLE_H        // trailing comment after #ifndef
#define ZCF_DISABLED_HEADER_GUARD_EXAMPLE_H        // trailing comment after an empty #define
#define ZCF_HEADER(name)                    <name> // trailing comment after a function-like #define
#if __has_include(ZCF_HEADER(vector))              // trailing comment after a macro-dependent #if
#include ZCF_HEADER(vector)                        // trailing comment after a macro-expanded #include
#else // trailing comment after a preprocessor #else
#error "formatter coverage for disabled preprocessor #error branches" // trailing comment after #error
#endif // trailing comment after an inner conditional directive
_Pragma("message(\"zcf _Pragma operator coverage\")") // trailing comment after the _Pragma operator
#endif // ZCF_DISABLED_HEADER_GUARD_EXAMPLE_H covers include guard comments
#endif // disabled header guard and macro include coverage

#if 0
int digraph_array<: 2 :> = (< % 1), (2 % >);
void
DynamicThrowSpecExample()
throw(int);
#endif // disabled digraph token coverage without relying on trigraph support

#if defined(ZCF_CPP23_PLATFORM_A) // trailing comment after #if
#define ZCF_PLATFORM_VALUE 1      // trailing comment after the first conditional definition
#elifdef ZCF_CPP23_PLATFORM_B     // trailing comment after #elifdef
#define ZCF_PLATFORM_VALUE 2      // trailing comment after the second conditional definition
#elifndef ZCF_CPP23_PLATFORM_C    // trailing comment after #elifndef
#define ZCF_PLATFORM_VALUE 3      // trailing comment after the third conditional definition
#else // trailing comment after the final conditional branch
#define ZCF_PLATFORM_VALUE 4      // trailing comment after the fallback definition
#endif // trailing comment after a complete conditional definition group

#ifdef ZCF_DIRECTIVE_COMMENT_IFDEF        // trailing comment after #ifdef
#define ZCF_DIRECTIVE_COMMENT_VALUE 5     // trailing comment in the #ifdef branch
#elif defined(ZCF_DIRECTIVE_COMMENT_ELIF) // trailing comment after plain #elif
#define ZCF_DIRECTIVE_COMMENT_VALUE 6     // trailing comment in the #elif branch
#else // trailing comment after the direct conditional fallback
#define ZCF_DIRECTIVE_COMMENT_VALUE 7     // trailing comment in the direct conditional fallback
#endif // trailing comment after the #ifdef/#elif group

#if defined(ZCF_CONTINUED_CONDITION_A) || \
    defined(ZCF_CONTINUED_CONDITION_B)  // trailing comment after a continued #if's final physical line
#define ZCF_CONTINUED_CONDITION_VALUE 1 // trailing comment in a continued conditional branch
#else // trailing comment after a continued conditional fallback
#define ZCF_CONTINUED_CONDITION_VALUE 0 // trailing comment in the continued conditional fallback
#endif // trailing comment after a continued conditional group

#if defined(__cpp_lib_stdatomic_h)
#define ZCF_STDATOMIC_FEATURE __cpp_lib_stdatomic_h
#else
#define ZCF_STDATOMIC_FEATURE 0
#endif

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(deprecated)
#define ZCF_HAS_DEPRECATED_ATTRIBUTE 1
#endif
#endif

#if defined(__has_builtin)
#if __has_builtin(__builtin_assume)
#define ZCF_HAS_BUILTIN_ASSUME 1
#endif
#endif

#pragma message("zcf macro pragma coverage") // trailing comment after #pragma message
#define ZCF_TEMP_MACRO 1                     // trailing comment before a pushed macro is redefined
#pragma push_macro("ZCF_TEMP_MACRO")         // trailing comment after #pragma push_macro
#undef ZCF_TEMP_MACRO                        // trailing comment after #undef
#define ZCF_TEMP_MACRO 2                     // trailing comment after a replacement definition
#pragma pop_macro("ZCF_TEMP_MACRO")          // trailing comment after #pragma pop_macro
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic pop
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(pop)
#endif
#line 100 "zcf_macro_coverage.cpp" // trailing comment after #line

#if defined(ZCF_ENABLE_NESTED_MACRO_LAYER) // trailing comment after an outer nested #if
#if defined(ZCF_ENABLE_INNER_MACRO_LAYER)  // trailing comment after an inner nested #if
#define ZCF_NESTED_MACRO_VALUE 10          // trailing comment in the innermost true branch
#else // trailing comment in the innermost false branch
#define ZCF_NESTED_MACRO_VALUE 11          // trailing comment in the innermost fallback definition
#endif // defined(ZCF_ENABLE_INNER_MACRO_LAYER) controls nested macro coverage branch selection
#else // trailing comment in the outer false branch
#define ZCF_NESTED_MACRO_VALUE 12          // trailing comment in the outer fallback definition
#endif // defined(ZCF_ENABLE_NESTED_MACRO_LAYER) controls outer macro coverage branch selection

// ============================================================================
// Format style: macro alignment, continuation indentation, statement macros,
// macro block boundaries, and whitespace-sensitive macro arguments.
// Language feature: object-like, function-like, token-paste, stringification,
// variadic, __VA_OPT__, declaration, namespace/body, and multiline macros.
//
// A // comment cannot safely precede a continuation backslash: backslash-newline
// splicing happens before comment removal and would extend the line comment onto
// the next physical line. Intermediate continuation lines therefore use block
// comments, while final replacement lines below exercise trailing // comments.
// ============================================================================
#define ZCF_SHORT_MACRO                   1                              // macro alignment: short name
#define ZCF_LONGER_MACRO_NAME             2                              // macro alignment: longer name
#define ZCF_EVEN_LONGER_MACRO_NAME(value) ((value) + 3)                  // macro alignment: function-like
#define ZCF_COMMENTED_MACRO(value)        ((value) * ZCF_TEMP_MACRO) // macro alignment across comments
#define ZCF_EMPTY_MARKER                                                 // trailing comment after an empty object-like macro
#define ZCF_EMPTY_FUNCTION()              TakeOne(0)                     // trailing comment after a zero-argument function-like macro
#define ZCF_ALIAS_VALUE                   ZCF_LONGER_MACRO_NAME      // trailing comment after a macro alias
#define ZCF_JOIN(a, b)                    a##b                           // trailing comment after token pasting
#define ZCF_STRINGIFY(x)                  #x                             // trailing comment after stringification
#define ZCF_MULTI_LINE_SUM(name, a, b) \
    int name()                             \
    {                                      \
        auto value = (a) + (b);            \
        return value;                      \
    } // trailing comment after a declaration-producing macro's final replacement line
#define ZCF_MULTI_LINE_EXPR(result, left, right)            \
    do {                                                        \
        auto result = (left) + (right);                         \
        ::zcf::coverage::TakeOne(static_cast<int>(result)); \
    }                                                           \
    while (false) // trailing comment after a statement macro's final replacement line
#define ZCF_COMMENTED_CONTINUATION(result, left, right)                    \
    do {                                                                       \
    /* macro continuation comment before the local variable */                 \
        auto result = (left) /* macro continuation comment between operands */ \
                      + (right);                                               \
        ::zcf::coverage::TakeOne(static_cast<int>(result));                \
    }                                                                          \
    while (false) // trailing comment after a block-commented macro's final replacement line
#define ZCF_MULTI_LINE_DECLARATION(name) \
    struct name                              \
    {                                        \
        int left;                            \
        int right;                           \
        int                                  \
        total() const                        \
        {                                    \
            return left + right;             \
        }                                    \
    }; // trailing comment after a type-producing macro's final replacement line
#define ZCF_MULTI_LINE_NAMESPACE_BODY(name, type_name) \
    namespace name                                         \
    {                                                      \
    struct type_name                                       \
    {                                                      \
        int value;                                         \
        int                                                \
        get() const                                        \
        {                                                  \
            return value;                                  \
        }                                                  \
    };                                                     \
    }                                                           // trailing comment after a namespace-producing macro's final replacement line
#define ZCF_API                               [[nodiscard]] // trailing comment after an attribute macro
#if defined(_MSC_VER)
#define ZCF_NOINLINE                          __declspec(noinline)
#else
#define ZCF_NOINLINE                          __attribute__((noinline))
#endif
#define ZCF_ATTRIBUTE_DECL(type, name)        ZCF_NOINLINE type name(int value) // trailing comment after a declaration macro
#define ZCF_RAW(...)                          __VA_ARGS__                           // trailing comment after a variadic passthrough macro
#define ZCF_LOG_IMPL(file, line, format, ...) ::zcf::coverage::LogImpl(file, line, format __VA_OPT__(, ) __VA_ARGS__)
#define ZCF_TRACE(format, ...)                ZCF_LOG_IMPL(__FILE__, __LINE__, format __VA_OPT__(, ) __VA_ARGS__) // trailing comment after a variadic forwarding macro
#define ZCF_VA_OPT_SENTINEL(format, ...)      ZCF_RAW(format __VA_OPT__(, ) __VA_ARGS__)                          // trailing comment after an __VA_OPT__ macro
#define ZCF_REQUIRE(expr)                                    \
    do {                                                         \
        if (!(expr)) { ::zcf::coverage::TakeOne(__LINE__); } \
    }                                                            \
    while (false)
#define ZCF_STATEMENT(expr)                               \
    do {                                                      \
        ::zcf::coverage::TakeOne(static_cast<int>(expr)); \
    }                                                         \
    while (false)
#define ZCF_IF_PRESENT(optional, name) if (auto name = (optional); name.has_value())
#define ZCF_SWITCH(value)              switch (value)
#define ZCF_CASE(value)                case value
#define foreach(item, range)               for (item : range)
#define Q_FOREACH(item, range)             for (item : range)
#define BOOST_FOREACH(item, range)         for (item : range)
#define KJ_IF_MAYBE(name, expr)            if (auto name = (expr); name.has_value())
#define ZCF_DECLARE_COUNTER(name)      int name(); // trailing comment after a declaration-generating macro
#define ZCF_DEFINE_COUNTER(name, value) \
    int name() { return value; } // trailing comment after a definition-generating macro's final line
#define ZCF_BEGIN_GENERATED_NAMESPACE(name) \
    namespace name                              \
    {
#define ZCF_END_GENERATED_NAMESPACE(name) } // trailing comment after a namespace-closing macro
#define ZCF_MULTI_LINE_VARIADIC_CALL(format, ...) \
    ZCF_TRACE(                                    \
        format                                        \
            __VA_OPT__(, )                            \
                __VA_ARGS__                           \
    ) // trailing comment after a multiline variadic macro's final replacement line
#if defined(_MSC_VER)
#define ZCF_STDCALL                   __stdcall
#define ZCF_PACKED_BEGIN              __pragma(pack(push, 1))
#define ZCF_PACKED_END                __pragma(pack(pop))
#define ZCF_WARNING_SUPPRESS(code)    __pragma(warning(suppress : code))
#define ZCF_DECLSPEC_EXPORT           __declspec(dllexport)
#define ZCF_DECLSPEC_NOVTABLE         __declspec(novtable)
#define ZCF_DECLSPEC_SELECTANY        __declspec(selectany)
#define ZCF_DECLSPEC_ALIGN(alignment) __declspec(align(alignment))
#else
#define ZCF_STDCALL
#define ZCF_PACKED_BEGIN
#define ZCF_PACKED_END
#define ZCF_WARNING_SUPPRESS(code)
#define ZCF_DECLSPEC_EXPORT
#define ZCF_DECLSPEC_NOVTABLE
#define ZCF_DECLSPEC_SELECTANY        inline
#define ZCF_DECLSPEC_ALIGN(alignment) alignas(alignment)
#endif
#ifndef SEC_ENTRY
#define SEC_ENTRY ZCF_STDCALL
#endif
#ifndef NTAPI
#define NTAPI ZCF_STDCALL
#endif
#ifndef _In_reads_
#define _In_reads_(size)
#endif
#ifndef _In_reads_bytes_
#define _In_reads_bytes_(size)
#endif
#ifndef _Out_
#define _Out_
#endif
#ifndef _Out_writes_bytes_to_
#define _Out_writes_bytes_to_(size, count)
#endif
#ifndef _Inout_
#define _Inout_
#endif
#ifndef _Deref_out_opt_
#define _Deref_out_opt_
#endif
#ifndef _COM_Outptr_
#define _COM_Outptr_
#endif
#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(identifier) struct
#endif
#ifndef WINAPI_PARTITION_DESKTOP
#define WINAPI_PARTITION_DESKTOP 0x00000001
#endif
#ifndef WINAPI_PARTITION_APP
#define WINAPI_PARTITION_APP 0x00000002
#endif
#ifndef WINAPI_PARTITION_SYSTEM
#define WINAPI_PARTITION_SYSTEM 0x00000004
#endif
#ifndef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(partitions) 0
#endif
#define ZCF_HANDLE_MESSAGE(window, message, handler) \
    case message:                                        \
        return handler(window, message)
#define ZCF_RETURN_IF_FAILED(expression)   \
    do {                                       \
        auto result = (expression);            \
        if (FAILED(result)) { return result; } \
    }                                          \
    while (false)
#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(expression) ZCF_RETURN_IF_FAILED(expression)
#endif
#define ZCF_LINE_SPLICING_TEXT   "line splicing whitespace coverage" \
                                   "without committing trailing whitespace" // trailing comment after a spliced string macro's final line
#define ZCF_UNDEF_ME             1                                      // trailing comment before undefining an object-like macro
#undef ZCF_UNDEF_ME                                                     // trailing comment after undefining an object-like macro

#if 0
using ZCFRawWindowCallback = long(CALLBACK*)(
    void* window,
    unsigned message,
    std::uintptr_t word_parameter,
    std::intptr_t long_parameter
    );
long WINAPI
ZCFRawWindowProcedure(
    void* window,
    unsigned message,
    std::uintptr_t word_parameter,
    std::intptr_t long_parameter
    );
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) // trailing comment after a function-like macro in #if
#define ZCF_WINDOWS_FAMILY_AVAILABLE 1                                                             // trailing comment after an enabled feature macro
#else // trailing comment after the feature macro's alternate branch
#define ZCF_WINDOWS_FAMILY_AVAILABLE 0                                                             // trailing comment after a disabled feature macro
#endif // trailing comment after a macro-dependent conditional group

// ============================================================================
// Format style: declarations ordered most-common-first; tiers are ranked by
// how frequently each pattern appears in real C++ codebases.
// Language feature: complete C++ surface through C++23, from everyday patterns
// (aliases, enums, value types, class hierarchies) to specialized features
// (virtual inheritance, coroutines, C++23 deducing-this, concepts).
// ============================================================================
namespace zcf
{

namespace coverage
{

// ============================================================================
// TIER 1 — ALIASES, TYPEDEFS, NAMESPACE BOILERPLATE           [most common]
// Format style: using declarations, typedef, namespace alias, inline/anonymous
// namespace definitions, using-namespace directives, global variable attributes.
// Language feature: type aliasing (using/typedef), namespace alias, inline
// namespace, anonymous namespace, constinit, thread_local, deprecated,
// maybe_unused.
// ============================================================================
using Index    = std::vector<int>::size_type;
using Callback = int (*)(
    int,
    const char*
    );
typedef int LegacyCount;
namespace ranges_alias = std::ranges;

inline namespace abi_v1
{

struct InlineNamespaceTag
{
    int value;
};

}

namespace
{

struct AnonymousNamespaceTag
{
    int value;
};

}

using namespace std::literals::string_view_literals;
using namespace std::chrono_literals;
thread_local constinit int g_thread_local_counter                                                           = 0;
constinit int g_constinit_counter                                                                           = 1;
[[maybe_unused]] [[deprecated("exercise deprecated attribute formatting")]] static int g_deprecated_counter = 0;

// ============================================================================
// TIER 2 — ENUMERATIONS                                        [very common]
// Format style: scoped/unscoped enum declarations, enumerator spacing,
// using-enum-name, non-member constexpr operator for scoped enum, vendor-
// specific function attributes via preprocessor.
// Language feature: enum class with underlying type, legacy enum, using enum,
// carries_dependency attribute, conditional vendor attributes.
// ============================================================================
enum class Mode : unsigned
{
    Alpha = 1,
    Beta  = 2,
    Gamma = 3
};

enum LegacyMode
{
    LegacyAlpha = 0,
    LegacyBeta  = 1,
    LegacyGamma = 2
};

enum class Permission : unsigned
{
    Read    = 1,
    Write   = 2,
    Execute = 4
};

using enum Mode;
static_assert(std::is_scoped_enum_v<Mode>);

constexpr Permission
operator|(
    Permission left,
    Permission right
    )
{
    return static_cast<Permission>(std::to_underlying(left) | std::to_underlying(right));
}

[[carries_dependency]] int
CarriesDependency(
    int value
    )
{
    return value;
}

#if defined(__GNUC__) || defined(__clang__)
[[gnu::always_inline]] inline int
VendorAttributedInline(
    int value
    )
{
    return value + 1;
}
#else
[[msvc::forceinline]] inline int
VendorAttributedInline(
    int value
    )
{
    return value + 1;
}
#endif

// ============================================================================
// TIER 3 — SIMPLE AGGREGATES: BIT-FIELDS, UNIONS, ALIGNED STORAGE  [common]
// Format style: bit-field member declarations, union member layout, alignas
// specifier on struct, static_assert with type-trait predicate.
// Language feature: named bit-fields, union type-punning, alignas, alignof,
// is_implicit_lifetime, reference_constructs_from_temporary.
// ============================================================================
struct Flags
{
    unsigned enabled  : 1;
    unsigned category : 3;
    unsigned reserved : 12;
};

union Payload
{
    std::uint32_t bits;
    float number;
};

struct alignas(64) AlignedRecord
{
    std::array<std::byte, 64> bytes;
};

static_assert(std::is_implicit_lifetime_v<Flags>);
using TemporaryReferenceCheck = std::bool_constant<std::reference_constructs_from_temporary_v<const int&, int>>;

// ============================================================================
// TIER 4 — CONSTEXPR VALUE TYPES: FULL OPERATOR SET, COPY/MOVE  [common]
// Format style: constexpr class body, delegating constructor init-list,
// operator declarations (prefix/postfix, compound, binary, comparison,
// conversion), friend non-member operator, friend stream operator declaration.
// Language feature: delegating constructor, explicit =default on all six
// special members, all arithmetic/compound-assign/comparison operators,
// prefix and postfix ++/--, operator%, friend scalar-multiply, friend stream
// operator<<, [[nodiscard]] on a struct type.
// Note: arithmetic operators on floating-point types -> Vec2. Integer step
// counter -> Counter. Both are needed to show different formatting contexts.
// ============================================================================

// --- Integer step counter: prefix/postfix and compound operators ---
struct Counter
{
    int value;

    Counter&
    operator++()
    {
        ++value;

        return *this;
    }

    Counter
    operator++(
        int
        )
    {
        Counter copy = *this;
        ++value;

        return copy;
    }

    Counter&
    operator--()
    {
        --value;

        return *this;
    }

    Counter
    operator--(
        int
        )
    {
        Counter copy = *this;
        --value;

        return copy;
    }

    Counter
    operator%(
        int mod
        ) const
    {
        return {value % mod};
    }

    Counter&
    operator+=(
        int delta
        )
    {
        value += delta;

        return *this;
    }

    Counter&
    operator-=(
        int delta
        )
    {
        value -= delta;

        return *this;
    }
};

// --- Floating-point value type: special members and full arithmetic ---
struct [[nodiscard]] Vec2
{
    double x, y;

    constexpr Vec2():

        Vec2(
            0.0,
            0.0
            )
    {
    }

    constexpr Vec2(
        double x,
        double y
        ):
        x(x),
        y(y)
    {
    }

    constexpr
    Vec2(
        const Vec2&
        ) = default;

    constexpr Vec2&
    operator=(
        const Vec2&
        ) = default;

    constexpr
    Vec2(
        Vec2&&
        ) = default;

    constexpr Vec2&
    operator=(
        Vec2&&
        ) = default;

    constexpr Vec2
    operator+(
        const Vec2& o
        ) const
    {
        return {x + o.x, y + o.y};
    }

    constexpr Vec2
    operator-(
        const Vec2& o
        ) const
    {
        return {x - o.x, y - o.y};
    }

    constexpr Vec2
    operator*(
        double s
        ) const
    {
        return {x* s, y* s};
    }

    constexpr Vec2
    operator/(
        double s
        ) const
    {
        return {x / s, y / s};
    }

    constexpr Vec2&
    operator+=(
        const Vec2& o
        )
    {
        x += o.x;
        y += o.y;

        return *this;
    }

    constexpr Vec2&
    operator-=(
        const Vec2& o
        )
    {
        x -= o.x;
        y -= o.y;

        return *this;
    }

    constexpr Vec2&
    operator*=(
        double s
        )
    {
        x *= s;
        y *= s;

        return *this;
    }

    constexpr Vec2&
    operator/=(
        double s
        )
    {
        x /= s;
        y /= s;

        return *this;
    }

    constexpr bool
    operator==(
        const Vec2& o
        ) const
    {
        return (x == o.x) && (y == o.y);
    }

    constexpr bool
    operator!=(
        const Vec2& o
        ) const
    {
        return !(*this == o);
    }

    friend Vec2
    operator*(
        double s,
        const Vec2& v
        )
    {
        return v * s;
    }

    friend std::ostream&
    operator<<(
        std::ostream& os,
        const Vec2& v
        );
};

std::ostream&
operator<<(
    std::ostream& os,
    const Vec2& v
    )
{
    return os << "(" << v.x << "," << v.y << ")";
}

// ============================================================================
// TIER 5 — CLASS HIERARCHIES: ABSTRACT INTERFACES AND CONCRETE TYPES  [common]
// Format style: pure-virtual declarations, multiple-inheritance base-specifier
// list, comma-first member-initializer list (7 members), out-of-line method
// definitions for a concrete class, override, final class, noexcept ctor.
// Language feature: abstract class (pure virtual), multiple public inheritance,
// complex member-initializer list, out-of-line method bodies, final class.
// ============================================================================

// --- Abstract shape and labeling interfaces ---
class IShape
{
public:

    virtual
    ~IShape() = default;

    [[nodiscard]] virtual double
    area() const = 0;

    [[nodiscard]] virtual double
    perimeter() const = 0;

    virtual std::string
    describe() const = 0;

    virtual void
    scale(
        double factor
        ) = 0;
};

class ILabeled
{
public:

    virtual
    ~ILabeled() = default;

    [[nodiscard]] virtual std::string_view
    label() const = 0;

    virtual void
    relabel(
        std::string new_label
        ) = 0;
};

// --- Concrete multiple-inheritance implementation ---
class Rectangle:
    public IShape,
    public ILabeled
{
public:

    Rectangle(
        double width,
        double height,
        std::string label,
        int id,
        std::vector<double> extra_data
        ):
        width_(width),
        height_(height),
        label_(std::move(label)),
        id_(id),
        extra_(std::move(extra_data)),
        area_cache_(width_ * height_),
        perimeter_cache_(2.0 * (width_ + height_))
    {
    }

    [[nodiscard]] double
    area() const override
    {
        return area_cache_;
    }

    [[nodiscard]] double
    perimeter() const override
    {
        return perimeter_cache_;
    }

    std::string
    describe() const override;

    void
    scale(
        double factor
        ) override;

    [[nodiscard]] std::string_view
    label() const override
    {
        return label_;
    }

    void
    relabel(
        std::string new_label
        ) override
    {
        label_ = std::move(new_label);
    }

    [[nodiscard]] double
    width() const noexcept
    {
        return width_;
    }

    [[nodiscard]] double
    height() const noexcept
    {
        return height_;
    }

private:

    double width_;
    double height_;
    std::string label_;
    int id_;
    std::vector<double> extra_;
    double area_cache_;
    double perimeter_cache_;
};

// --- Out-of-line concrete method definitions ---
std::string
Rectangle::describe() const
{
    return std::to_string(width_) + "x" + std::to_string(height_) + " id=" + std::to_string(id_) + " label=" + label_;
}

void
Rectangle::scale(
    double factor
    )
{
    width_          *= factor;
    height_         *= factor;
    area_cache_      = width_ * height_;
    perimeter_cache_ = 2.0 * (width_ + height_);
}

// --- Secondary base/derived hierarchy and pointer declarators ---
class Base
{
public:

    virtual
    ~Base() = default;

    virtual int
    id() const
    {
        return 0;
    }
};

/**
 * Format style: Doxygen block comments before a class declaration.
 * Language feature: final class, noexcept constructor, override, forwarding
 * references, fold expressions, and pointer/reference declarators.
 */
class Derived final:
    public Base
{
public:

    Derived(
        int seed,
        std::unique_ptr<int> owned,
        std::shared_ptr<const int> shared
        ) noexcept:
        Base(),
        seed_(seed),
        owned_(std::move(owned)),
        shared_(std::move(shared))
    {
    }

    [[nodiscard]] int
    id() const override
    {
        return seed_;
    }

    template <typename ... Args>
    auto
    call(
        Args&&... args
        ) const -> decltype((... + args))
    {
        return (... + args);
    }

    void
    pointers(
        const volatile int* const* input,
        int& output
        ) const;

private:

    int seed_;
    std::unique_ptr<int> owned_;
    std::shared_ptr<const int> shared_;
};

void
Derived::pointers(
    const volatile int* const* input,
    int& output
    ) const
{
    output = **input + seed_;
}

// ============================================================================
// TIER 6 — ACCESS CONTROL: MUTABLE, FRIEND FUNCTION, [[no_unique_address]]
// Format style: access modifier placement, mutable member declaration,
// friend function declaration inside class, [[no_unique_address]] attribute,
// [[nodiscard(reason)]], protected virtual method, using-base-constructor.
// Language feature: friend injection, mutable specifier, protected access,
// explicit(false), [[no_unique_address]], [[nodiscard("reason")]],
// constructor inheritance via using.
// ============================================================================

class AccessAndFriend
{
    friend int
    InspectFriend(
        const AccessAndFriend& value
        );

public:

    explicit (false) AccessAndFriend(
        int seed
        ):
        seed_(seed),
        cached_(seed),
        tag_{seed}
    {
    }

    [[nodiscard("exercise nodiscard reason formatting")]] int
    seed() const
    {
        return seed_;
    }

    int
    cache() const
    {
        cached_ += 1;

        return cached_;
    }

protected:

    virtual int
    protected_value() const
    {
        return seed_;
    }

private:

    int seed_;
    mutable int cached_;
    [[no_unique_address]] InlineNamespaceTag tag_;
};

struct ProtectedAccessProbe:
    AccessAndFriend
{
    using AccessAndFriend::AccessAndFriend;

    int
    expose() const
    {
        return protected_value();
    }
};

// ============================================================================
// TIER 7 — COMPARISON AND CONVERSION OPERATORS, REF-QUALIFIED METHODS
// Format style: defaulted three-way comparison declaration, explicit conversion
// operator declarations, lvalue/rvalue ref-qualifier specifiers, volatile
// method qualifier.
// Language feature: defaulted operator<=> (spaceship), explicit operator bool,
// implicit conversion operator int, lvalue-ref-qualified method, rvalue-ref-
// qualified method, const volatile method qualifier.
// Note: binary arithmetic operators +/-/*/% and compound += -= *= /= are
// covered by Vec2 (floating-point) and Counter (integer) in Tier 4.
// ============================================================================

struct Comparable
{
    int value;
    auto
    operator<=>(
        const Comparable&
        ) const = default;
};

// Explicit conversion operators, ref-qualified methods, volatile-qualified
// method. Binary arithmetic (+, -, *, /) is already covered by Vec2/Counter.
struct OperatorShowcase
{
    int value;

    explicit
    operator bool() const
    {
        return value != 0;
    }

    operator int() const
    {
        return value;
    }

    int
    lvalue_only() &
    {
        return value;
    }

    int
    rvalue_only() &&
    {
        return value;
    }

    int
    observe() const volatile
    {
        return value;
    }
};

// ============================================================================
// TIER 8 — RESOURCE MANAGEMENT: RAII, CUSTOM SMART POINTER, ALLOCATOR  [common]
// Format style: [[nodiscard]]/[[deprecated]] on type declarations, anonymous
// union inside struct, inline static member variable, friend class declaration,
// operator->/operator* pair, operator new/delete/new[]/delete[] overloads.
// Language feature: [[nodiscard]] struct type, [[deprecated]] class type,
// anonymous union inside struct, nested enum class inside struct, inline static
// member variable (C++17), friend class, custom smart pointer with operator->
// and operator*, global operator new/delete overloads.
// ============================================================================

// --- Ordinary RAII resource and manager ---
class ResourceManager;

struct [[nodiscard]] Resource
{
    enum class State
    {
        Idle,
        Active,
        Error
    };

    friend class ResourceManager;
    inline static int instance_count = 0;

    union
    {
        int int_payload;
        float float_payload;
    };

    State state = State::Idle;

    Resource()
    {
        ++instance_count;
    }

    ~Resource()
    {
        --instance_count;
    }

    Resource(
        const Resource& o
        ):
        int_payload(o.int_payload),
        state(o.state)
    {
        ++instance_count;
    }

    Resource&
    operator=(
        const Resource& o
        )
    {
        int_payload = o.int_payload;
        state       = o.state;

        return *this;
    }

    Resource(
        Resource&& o
        ) noexcept:
        int_payload(o.int_payload),
        state(o.state)
    {
        ++instance_count;
    }

    Resource&
    operator=(
        Resource&& o
        ) noexcept
    {
        int_payload = o.int_payload;
        state       = o.state;

        return *this;
    }
};

class [[deprecated("use ResourceManager2")]] ResourceManager
{
    friend struct Resource;

public:

    Resource
    acquire()
    {
        return Resource {};
    }
};

// --- Move-only custom pointer wrapper ---
template <typename T>
struct LegacyHandle
{
    inline static int live_count = 0;

    explicit LegacyHandle(
        T* ptr
        ):
        ptr_(ptr)
    {
        ++live_count;
    }

    ~LegacyHandle()
    {
        delete ptr_;
        --live_count;
    }

    LegacyHandle(
        const LegacyHandle&
        ) = delete;

    LegacyHandle&
    operator=(
        const LegacyHandle&
        ) = delete;

    LegacyHandle(
        LegacyHandle&& o
        ) noexcept:
        ptr_(o.ptr_)
    {
        o.ptr_ = nullptr;
    }

    LegacyHandle&
    operator=(
        LegacyHandle&& o
        ) noexcept
    {
        if (this != &o)
        {
            delete ptr_;
            ptr_   = o.ptr_;
            o.ptr_ = nullptr;
        }

        return *this;
    }

    T*
    operator->() noexcept
    {
        return ptr_;
    }

    const T*
    operator->() const noexcept
    {
        return ptr_;
    }

    T&
    operator*() noexcept
    {
        return *ptr_;
    }

    const T&
    operator*() const noexcept
    {
        return *ptr_;
    }

private:

    T* ptr_;
};

// --- Custom deleter and unique_ptr alias ---
struct HandleDeleter
{
    void
    operator()(
        int* value
        ) const noexcept
    {
        delete value;
    }
};

using OwnedHandle = std::unique_ptr<int, HandleDeleter>;

// --- Class-specific allocation operator overloads ---
struct CustomAllocation
{
    static void*
    operator new(
        std::size_t size
        )
    {
        return ::operator new(size);
    }

    static void
    operator delete(
        void* pointer
        ) noexcept
    {
        ::operator delete(pointer);
    }

    static void*
    operator new[](
        std::size_t size
        )
    {
        return ::operator new[](size);
    }

    static void
    operator delete[](
        void* pointer
        ) noexcept
    {
        ::operator delete[](pointer);
    }

    int value;
};

// ============================================================================
// TIER 9 — SUPPORTING DATA TYPES: NESTED TYPES, MEMBER POINTERS
// Format style: nested struct declaration inside struct, pointer-to-data-member
// and pointer-to-member-function type syntax.
// Language feature: nested type definition, pointer-to-data-member type,
// pointer-to-member-function type (used with .* and ->* operators).
// ============================================================================

struct NestedTypeShowcase
{
    struct Inner
    {
        int value;
    };

    Inner inner;
};

struct MemberPointerRecord
{
    int member;

    int
    method(
        int bias
        ) const
    {
        return member + bias;
    }
};

// ============================================================================
// TIER 10 — TEMPLATES: CLASS TEMPLATES, SPECIALIZATIONS, OUT-OF-LINE DEFS
// Format style: template parameter lists (type, non-type, template-template,
// auto NTTP), partial and full template specializations, extern template
// declaration, explicit template instantiation, user-defined CTAD deduction
// guides, out-of-line template method definitions including nested-template
// member functions, template type alias, inline constexpr variable template.
// Language feature: class templates, SFINAE via enable_if_t partial spec,
// full specialization, deduction guides (CTAD), extern/explicit template,
// out-of-line defs for primary and nested-template member functions, template
// alias, variable template.
// ============================================================================

// --- SFINAE, detection idiom, and full specialization ---
template <class T, class = void>
struct Kind
{
    static constexpr const char*
    name()
    {
        return "other";
    }
};

template <class T>
struct Kind<T, std::enable_if_t<std::is_integral_v<T>>>
{
    static constexpr const char*
    name()
    {
        return "integral";
    }
};

template <typename T, typename = void>
struct HasStaticNow:
    std::false_type
{
};

template <typename T>
struct HasStaticNow<T, std::void_t<decltype(T::now()), typename T::rep, typename T::period>>:
    std::true_type
{
};

template <>
struct Kind<bool, void>
{
    static constexpr const char*
    name()
    {
        return "bool";
    }
};

// --- Class templates, deduction guides, and explicit instantiation ---
template <class T>
struct Box
{
    Box(
        T value
        ):
        value(value)
    {
    }

    T value;
};

Box(const char*)->Box<std::string_view>;

template <class T>
struct InheritedBox:
    Box<T>
{
    using Box<T>::Box;
};
extern template struct Box<int>;
template struct Box<long>;

// --- Repository class template declaration ---
template <typename T>
class Repository
{
public:

    explicit Repository(
        std::string name,
        std::size_t capacity = 16
        ):
        name_(std::move(name)),
        items_(),
        capacity_(capacity)
    {
        items_.reserve(capacity_);
    }

    void
    insert(
        T item
        );

    [[nodiscard]] bool
    contains(
        const T& item
        ) const;

    [[nodiscard]] std::size_t
    size() const noexcept
    {
        return items_.size();
    }

    [[nodiscard]] std::string_view
    name() const noexcept
    {
        return name_;
    }

    template <typename Predicate>
    [[nodiscard]] std::size_t
    count_if(
        Predicate&& pred
        ) const;

    template <typename Mapper, typename R = std::invoke_result_t<Mapper, const T&>>
    [[nodiscard]] std::vector<R>
    transform_all(
        Mapper&& mapper
        ) const;

private:

    std::string name_;
    std::vector<T> items_;
    std::size_t capacity_;
};

// --- Out-of-line and nested template member definitions ---
template <typename T>
void
Repository<T>::insert(
    T item
    )
{
    if (items_.size() < capacity_)
    {
        items_.push_back(std::move(item));
    }
}

template <typename T>
bool
Repository<T>::contains(
    const T& item
    ) const
{
    return std::ranges::contains(
        items_,
        item
        );
}

template <typename T>
template <typename Predicate>
std::size_t
Repository<T>::count_if(
    Predicate&& pred
    ) const
{
    return static_cast<std::size_t>(std::ranges::count_if(
        items_,
        std::forward<Predicate>(pred)
        ));
}

template <typename T>
template <typename Mapper, typename R>
std::vector<R>
Repository<T>::transform_all(
    Mapper&& mapper
    ) const
{
    std::vector<R> result;
    result.reserve(items_.size());
    std::ranges::transform(
        items_,
        std::back_inserter(result),
        std::forward<Mapper>(mapper)
        );

    return result;
}

// --- Deeply qualified and template-heavy function definition ---
// Format style: stacked template heads, nested return and parameter types,
// comment-separated qualified-name components, and closing-angle indentation.
// Language feature: nested class-template member function template, out-of-line
// definition, invoke_result, variant, vector, tuple, optional, and references.
namespace complex_definition
{

template <typename Owner, typename Error>
class TransformationRegistry
{
public:

    template <typename Record>
    class Pipeline
    {
    public:

        template <typename Mapper, typename Predicate>
        [[nodiscard]] static std::variant<std::vector<std::tuple<Owner, Record, std::invoke_result_t<Mapper&, const Record&>>>, Error>
        TransformMatchingRecordsWithFallback(
            const std::vector<std::pair<Owner, Record>>& source_records,
            const std::optional<std::reference_wrapper<const Record>>& fallback_record,
            Mapper&& mapper,
            Predicate&& predicate
            );
    };
};

}

template <typename Owner, typename Error>
template <typename Record>
template <typename Mapper, typename Predicate>
[[nodiscard]] std::variant<
    // Successful results preserve the owner, record, and mapped value.
    std::vector<
        std::tuple<
            Owner,
            Record,
            std::invoke_result_t<
                Mapper&,
                const Record& // Mapper input.
                >             // Mapped value.
            >                 // One transformed record.
        >,                    // All transformed records.
    Error                     // Registry-specific failure.
    >
complex_definition::                   // Helper namespace.
TransformationRegistry<Owner, Error>:: // Outer class specialization.
Pipeline<Record>::                     // Nested class specialization.
TransformMatchingRecordsWithFallback(
    const std::vector<
        std::pair<
            Owner,
            Record // Source record type.
            >      // One source owner-record entry.
        >& source_records,
    const std::optional<
        std::reference_wrapper<
            const Record // Borrowed record type.
            >            // Borrowed fallback record.
        >& fallback_record,
    Mapper&& mapper,
    Predicate&& predicate
    )
{
    (void) source_records;
    (void) fallback_record;
    (void) mapper;
    (void) predicate;

    return std::vector<std::tuple<Owner, Record, std::invoke_result_t<Mapper&, const Record&>>>{};
}

// --- Traits, aliases, variable templates, and non-type parameters ---
template <typename T>
struct TemplateTraits
{
    using value_type = T;
    static constexpr std::string_view category = "value";
};

template <typename T>
struct TemplateTraits<T*>
{
    using value_type = T;
    static constexpr std::string_view category = "pointer";
};

template <typename T>
struct TemplateTraits<T&>
{
    using value_type = T;
    static constexpr std::string_view category = "lvalue";
};

template <>
struct TemplateTraits<void>
{
    using value_type = void;
    static constexpr std::string_view category = "void";
};

template <template <class> class Wrapper, class T>
struct TemplateTemplateUse
{
    Wrapper<T> wrapped;
};

template <auto Value>
struct AutoNonTypeParameter
{
    static constexpr auto value = Value;
};

template <typename T>
using DecayedVector = std::vector<std::remove_cvref_t<T>>;

template <typename T>
inline constexpr bool IsPointerLikeV = std::is_pointer_v<std::remove_reference_t<T>>;
static_assert(IsPointerLikeV<int*>);

// --- Variadic overload sets and constrained static buffers ---
template <typename ... Callables>
struct Overload:
    Callables...
{
    using Callables::operator() ...;
};

template <typename ... Callables>
Overload(Callables...)->Overload<Callables...>;

template <typename T, std::size_t N>
struct StaticBuffer
{
    std::array<T, N> data;

    template <typename ... Args>
    requires(sizeof...(Args) == N)

    constexpr StaticBuffer(
        Args&&... args
        ):
        data{static_cast<T>(std::forward<Args>(args))...}
    {
    }

    template <std::size_t I>
    constexpr decltype(auto)
    get()
    & requires(I < N)
    {
        return (data[I]);
    }

    template <std::size_t I>
    constexpr decltype(auto)
    get() const&
    requires(I < N)
    {
        return (data[I]);
    }
};

// ============================================================================
// TIER 11 — CONCEPTS AND CONSTRAINED TEMPLATES (C++20)
// Format style: concept definitions with simple and compound requires-
// expressions, multi-line requires body with type/expression/nested
// requirements, trailing requires on function template, abbreviated template.
// Language feature: concept, simple requires-expression, compound requires-
// expression with noexcept/return-type constraint, nested requires, variadic
// concept (disjunction fold), consteval NTTP pack fold, abbreviated function
// template with auto parameter, constrained auto.
// ============================================================================

template <typename T>
concept Addable = requires(T value)
{
    value + 1;
};
template <typename T>
concept IntegralLike = std::integral<T>&& requires(T value)
{
    typename std::make_unsigned_t<T>;
    value + 0z;
};
template <typename T>
concept RichRequirement = requires(
    T value,
    const T cvalue
    )
{
    typename T::value_type;
    { value.size() }
    noexcept->std::convertible_to<std::size_t>;
    { cvalue.begin() }->std::input_iterator;
    requires sizeof(T) > 0;
};

template <typename T, typename ... Candidates>
concept OneOf = (std::same_as<std::remove_cvref_t<T>, Candidates>|| ...);

template <auto... Values>
consteval auto
SumNonTypeValues()
{
    return (Values + ... + 0);
}

template <typename T>
requires OneOf<T, int, long, short>
auto
ConstrainedIdentity(
    T&& value
    ) -> std::remove_cvref_t<T>
{
    return static_cast<std::remove_cvref_t<T>>(value);
}

auto
AbbreviatedTemplate(
    std::integral auto value,
    Addable auto delta
    )
{
    return value + delta;
}

// ============================================================================
// TIER 12 — C++23 CLASS FEATURES: DEDUCING THIS, STATIC OPERATORS, CRTP
// Format style: explicit object parameter in member function signatures,
// static operator() and static operator[], multi-dimensional operator[],
// decltype(auto) return from forwarding deducing-this CRTP base.
// Language feature: explicit object parameter (P0847R7), static call operator,
// static subscript operator, multi-index subscript (P2589R3), CRTP via
// deducing this.
// ============================================================================

struct ExplicitObjectParameter
{
    int value;

    auto
    get(
        this const ExplicitObjectParameter& self
        ) -> int
    {
        return self.value;
    }

    auto
    add(
        this ExplicitObjectParameter&& self,
        int delta
        ) -> ExplicitObjectParameter
    {
        self.value += delta;

        return std::move(self);
    }
};

struct StaticCallAndSubscript
{
    static int
    operator()(
        int left,
        int right
        )
    {
        return left + right;
    }

    static int
    operator[](
        std::size_t row,
        std::size_t column
        )
    {
        return static_cast<int>(row + column);
    }
};

struct Tensor3D
{
    int data[2][2][2] {};

    int&
    operator[](
        std::size_t plane,
        std::size_t row,
        std::size_t column
        )
    {
        return data[plane][row][column];
    }
};

template <typename DerivedT>
struct CrtpFormatterBase
{
    decltype(auto)
    as_derived(this auto&& self)
    {
        return static_cast<decltype(self)>(self);
    }
};

// ============================================================================
// TIER 13 — UNCOMMON CLASS FEATURES: explicit(bool), =delete, FUNCTION-TRY
// Format style: explicit(bool) conditional conversion constructor, =delete on
// a specific overload, function-try-block constructor, consteval function,
// static_assert with message string, user-defined literal operator.
// Language feature: conditional explicit conversion, deleted specific overload,
// function-try-block, consteval, static_assert with string, UDL operator "".
// ============================================================================

template <bool Strict>
struct ExplicitBool
{
    explicit (Strict) ExplicitBool(
        int value
        ):
        value(value)
    {
    }

    int value;
};

struct DeletedAndDefaulted
{
    DeletedAndDefaulted() = default;

    DeletedAndDefaulted(
        double
        ) = delete;
};

struct FunctionTryBlock
{
    int value;

    FunctionTryBlock(
        int input
        )
    try:
        value(input)
    {
    }

    catch (...)
    {
        throw;
    }
};

consteval int
CompileTimeOnly(
    int value
    )
{
    return value + 1;
}

static_assert(
    (CompileTimeOnly(1) == 2),
    "consteval function coverage with static_assert message"
    );

constexpr LegacyCount
operator"" _wf(
    unsigned long long value
    )
{
    return static_cast<LegacyCount>(value);
}

// ============================================================================
// TIER 14 — ADVANCED INHERITANCE: VIRTUAL, COVARIANT, PROTECTED/PRIVATE
// Format style: virtual keyword in base-specifier list, final specifier on a
// virtual method (not on the class type), covariant return type in override,
// protected and private base-class specifiers.
// Language feature: virtual (diamond) inheritance, final method specifier,
// covariant return type override, protected base class, private base class
// with selective re-exposure via using.
// ============================================================================

struct DiamondBase
{
    virtual int
    compute() const
    {
        return 0;
    }

    virtual
    ~DiamondBase() = default;
};

struct DiamondLeft:
    virtual public DiamondBase
{
    int
    compute() const override
    {
        return 1;
    }
};

struct DiamondRight:
    virtual public DiamondBase
{
    int
    compute() const override
    {
        return 2;
    }
};

struct DiamondDerived:
    DiamondLeft,
        DiamondRight
{
    int
    compute() const final
    {
        return DiamondLeft::compute() + DiamondRight::compute();
    }
};

struct CovariantBase
{
    virtual CovariantBase*
    clone() const
    {
        return new CovariantBase();
    }

    virtual
    ~CovariantBase() = default;
};

struct CovariantDerived:
    public CovariantBase
{
    CovariantDerived*
    clone() const override
    {
        return new CovariantDerived();
    }
};

struct InheritanceRoot
{
    int value = 0;
};

struct WithProtectedInheritance:
    protected InheritanceRoot
{
    using InheritanceRoot::value;

    int
    get() const
    {
        return value;
    }
};

struct WithPrivateInheritance:
    private InheritanceRoot
{
    int
    get() const
    {
        return value;
    }
};

// ============================================================================
// TIER 15 — COROUTINES (C++20/23)                              [rare]
// Format style: coroutine return type declarations, nested promise_type struct,
// co_yield and co_await keyword placement in function bodies.
// Language feature: coroutine protocol (promise_type, suspend_never,
// coroutine_handle), co_yield for generators, co_await for async tasks,
// std::generator C++23 coroutine return type.
// ============================================================================

struct ImmediateAwaiter
{
    bool
    await_ready() const noexcept
    {
        return true;
    }

    void
    await_suspend(
        std::coroutine_handle<>
        ) const noexcept
    {
    }

    int
    await_resume() const noexcept
    {
        return 7;
    }
};

struct AwaitableTask
{
    struct promise_type
    {
        AwaitableTask
        get_return_object()
        {
            return {};
        }

        std::suspend_never
        initial_suspend() noexcept
        {
            return {};
        }

        std::suspend_never
        final_suspend() noexcept
        {
            return {};
        }

        void
        return_void()
        {
        }

        void
        unhandled_exception()
        {
        }
    };
};

// ============================================================================
// TIER 16 — MACRO-GENERATED TYPES AND RANGE ADAPTOR CLOSURE    [specialized]
// Format style: macro invocations expanding to namespace/type definitions,
// range_adaptor_closure inheritance.
// Language feature: range_adaptor_closure (C++23), macro-generated namespace
// body, macro-generated struct, macro-generated namespace+struct.
// ============================================================================

ZCF_BEGIN_GENERATED_NAMESPACE(macro_generated) // trailing comment after a namespace-opening macro invocation

struct GeneratedByMacro
{
    int field;

    int
    get() const
    {
        return field;
    }
};

ZCF_END_GENERATED_NAMESPACE(macro_generated)     // trailing comment after a namespace-closing macro invocation
ZCF_MULTI_LINE_DECLARATION(MacroGeneratedRecord) // trailing comment after a type-generating macro invocation
ZCF_MULTI_LINE_NAMESPACE_BODY(
    macro_multiline_namespace, // trailing comment after the first namespace macro argument
    MultilineNamespaceRecord   // trailing comment after the final namespace macro argument
    )                          // trailing comment after a multiline namespace macro invocation

struct IdentityClosure:
    std::ranges::range_adaptor_closure<IdentityClosure>
{
    template <std::ranges::viewable_range R>
    auto
    operator()(
        R&& range
        ) const
    {
        return std::views::all(std::forward<R>(range));
    }
};

// ============================================================================
// RDCORE-DERIVED WINDOWS ABI AND SYSTEMS PATTERNS               [specialized]
// Format style: SAL-decorated callback declarations, calling conventions,
// __pragma/__declspec wrappers, callback tables, COM-style methods, cleanup
// labels, message-dispatch macros, GUID/MIDL declarations, family partitions,
// anonymous unions/bit-fields, native handles, and member-function callbacks.
// Language feature: Windows ABI declarations, HRESULT flows, std::function
// callbacks, C ABI tables, advanced SAL, WIL-style returns, OVERLAPPED layouts.
// ============================================================================
// --- HRESULT aliases and GUID/COM interface declarations ---
using ZCFHResult = long;
inline constexpr ZCFHResult ZCFSuccess      = 0;
inline constexpr ZCFHResult ZCFPointerError = -1;

#ifndef IFACEMETHODIMP
#define IFACEMETHODIMP ZCFHResult
#endif
#ifndef FAILED
#define FAILED(result) ((result) < 0)
#endif

struct ZCFGuid
{
    std::uint32_t data1;
    std::uint16_t data2;
    std::uint16_t data3;
    std::uint8_t data4[8];
};

inline constexpr ZCFGuid ZCFClassId =
{
    0x8A28E9D1,
    0x7F44,
    0x4B59,
    {0x90, 0xE1, 0x12, 0x48, 0xA5, 0x7C, 0xD3, 0x61}
};

struct ZCFUnknown
{
    virtual ZCFHResult
    QueryInterface(
        const ZCFGuid& identifier,
        _COM_Outptr_ void** object
        ) = 0;

    virtual
    ~ZCFUnknown() = default;
};

MIDL_INTERFACE("8A28E9D1-7F44-4B59-90E1-1248A57CD361")
ZCFComInterface :
ZCFUnknown
{
    virtual ZCFHResult
        ReadBuffer(
        _In_reads_bytes_(input_size) const void* input,
        std::size_t input_size,
        _Out_writes_bytes_to_(
            capacity,
            *written
            ) std::byte * output,
        std::size_t capacity,
        _Out_ std::size_t* written,
        _Deref_out_opt_ void** state
        ) = 0;
};

#if defined(_MSC_VER)
struct __declspec(uuid("519CFA70-59A7-4B7F-95DB-A3B42A4CC879")) ZCFUuidTagged
{
};

using ZCFUuidType = decltype(__uuidof(ZCFUuidTagged));
#else
struct ZCFUuidTagged
{
};
#endif

// --- SAL-decorated C ABI callback types and calling conventions ---
typedef ZCFHResult (ZCF_STDCALL* ZCFDataCallback)(
    _In_reads_ (size) const std::byte* data,
    std::size_t size,
    _COM_Outptr_ void** context
    );

typedef ZCFHResult (SEC_ENTRY* ZCFBufferCallback)(
    _In_reads_bytes_ (input_size) const void* input,
    std::size_t input_size,
    _Out_writes_bytes_to_ (capacity, *written) std::byte* output,
    std::size_t capacity,
    _Out_ std::size_t* written,
    _Deref_out_opt_ void** state
    );

using ZCFCloseCallback = void (*)(
    _Inout_ void* context
    );

ZCFHResult NTAPI
ZCFNativeStatus(
    _Inout_ void* context
    ) noexcept
{
    return (context != nullptr) ? ZCFSuccess : ZCFPointerError;
}

ZCFHResult
ZCFForwardResult(
    ZCFHResult result
    )
{
    RETURN_IF_FAILED(result);

    return ZCFSuccess;
}

// --- Move-only native handle ownership ---
using ZCFNativeHandle = void*;

void
ZCFCloseNativeHandle(
    ZCFNativeHandle handle
    ) noexcept
{
    (void) handle;
}

class ZCFUniqueNativeHandle
{
public:

    ZCFUniqueNativeHandle() = default;

    explicit ZCFUniqueNativeHandle(
        ZCFNativeHandle handle
        ) noexcept:
        handle_(handle)
    {
    }

    ~ZCFUniqueNativeHandle()
    {
        reset();
    }

    ZCFUniqueNativeHandle(
        const ZCFUniqueNativeHandle&
        ) = delete;

    ZCFUniqueNativeHandle&
    operator=(
        const ZCFUniqueNativeHandle&
        ) = delete;

    ZCFUniqueNativeHandle(
        ZCFUniqueNativeHandle&& other
        ) noexcept:
        handle_(other.release())
    {
    }

    ZCFUniqueNativeHandle&
    operator=(
        ZCFUniqueNativeHandle&& other
        ) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }

        return *this;
    }

    [[nodiscard]] ZCFNativeHandle
    get() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] ZCFNativeHandle
    release() noexcept
    {
        return std::exchange(
            handle_,
            nullptr
            );
    }

    void
    reset(
        ZCFNativeHandle replacement = nullptr
        ) noexcept
    {
        if (handle_ != nullptr)
        {
            ZCFCloseNativeHandle(handle_);
        }

        handle_ = replacement;
    }

private:

    ZCFNativeHandle handle_ = nullptr;
};

// --- Packed records and OVERLAPPED-style layouts ---
ZCF_PACKED_BEGIN                                 // trailing comment after a standalone pragma macro invocation
struct ZCF_DECLSPEC_ALIGN(1) ZCFPackedHeader // trailing comment after an embedded alignment macro
{
    std::uint16_t type;
    std::uint16_t length;
    std::uint32_t sequence;
};

ZCF_PACKED_END // trailing comment after a matching standalone pragma macro invocation

struct ZCFOverlappedLike
{
    union
    {
        struct
        {
            std::uint32_t offset;
            std::uint32_t offset_high;
        };

        void* pointer;
    };

    std::uint32_t pending   : 1;
    std::uint32_t cancelled : 1;
    std::uint32_t reserved  : 30;
};

// --- Callback tables, std::function slots, and COM implementation ---
struct ZCFCallbackTable
{
    std::size_t size;
    ZCFDataCallback on_data;
    ZCFCloseCallback on_close;
};

struct ZCFCallbacks
{
    std::function<void()> on_ready                   = [] {};
    std::function<void(std::string_view)> on_message = nullptr;
    ZCFDataCallback raw_callback                 = nullptr;
};

struct ZCF_DECLSPEC_NOVTABLE ZCFSystemCallback
{
    virtual ZCFHResult
        OnData(
        _In_reads_(size) const std::byte * data,
        std::size_t size
        ) = 0;

    virtual
    ~ZCFSystemCallback() = default;
};

struct ZCFSystemCallbackImpl final:
    ZCFSystemCallback
{
    IFACEMETHODIMP
    OnData(
        _In_reads_(size) const std::byte* data,
        std::size_t size
        ) override
    {
        return ((data != nullptr) && (size > 0)) ? ZCFSuccess : ZCFPointerError;
    }

    void
    OnReady()
    {
        ++ready_count;
    }

    void
    OnMessage(
        std::string_view message
        )
    {
        ready_count += static_cast<int>(message.size());
    }

    int ready_count = 0;
};

// --- Pointer-to-member callback dispatch ---
template <typename Interface>
ZCFHResult
InvokeMemberCallback(
    Interface* target,
    ZCFHResult (Interface::*callback)(
        _In_reads_(size) const std::byte*,
        std::size_t
        ),
    _In_reads_(size) const std::byte* data,
    std::size_t size
    )
{
    return (target->*callback)(
        data,
        size
        );
}

// --- Exported callbacks and message dispatch ---
ZCF_DECLSPEC_SELECTANY const std::uint32_t ZCFAbiVersion = 1; // trailing comment after a declaration-specifier macro

ZCF_DECLSPEC_EXPORT ZCFHResult ZCF_STDCALL
// trailing comment after export and calling-convention macros
ZCFRawDataCallback(
    _In_reads_(size) const std::byte* data,
    std::size_t size,
    _COM_Outptr_ void** context
    )
{
    if ((data == nullptr) || (size == 0) || (context == nullptr))
    {
        return ZCFPointerError;
    }

    *context = const_cast<std::byte*>(data);

    return ZCFSuccess;
}

void
ZCFRawCloseCallback(
    _Inout_ void* context
    )
{
    (void) context;
}

int
ZCFHandleMessage(
    int window,
    int message
    )
{
    return window + message;
}

int
ZCFDispatchMessage(
    int window,
    int message
    )
{
    switch (message)
    {
        ZCF_HANDLE_MESSAGE(
            window,
            1,
            ZCFHandleMessage
            );
        ZCF_HANDLE_MESSAGE(
            window,
            2,
            ZCFHandleMessage
            );
        default:
            return 0;
    }
}

// --- HRESULT propagation and cleanup labels ---
ZCFHResult
ExerciseRdcoreSystemPatterns(
    _In_reads_(size) const std::byte* data,
    std::size_t size,
    _Out_ int* callback_count
    )
{
    ZCFHResult result  = ZCFSuccess;
    void* callback_context = nullptr;
    ZCFSystemCallbackImpl receiver;
    ZCFCallbacks callbacks;
    ZCFCallbackTable callback_table
    {
        sizeof(ZCFCallbackTable),
        ZCFRawDataCallback,
        ZCFRawCloseCallback
    };

    ZCF_WARNING_SUPPRESS(4127) // trailing comment after a diagnostic-control macro invocation

    if ((data == nullptr) || (callback_count == nullptr))
    {
        result = ZCFPointerError;
        goto Cleanup;
    }

    callbacks.on_ready = std::bind(
        &ZCFSystemCallbackImpl::OnReady,
        &receiver
        );
    callbacks.on_message = std::bind(
        &ZCFSystemCallbackImpl::OnMessage,
        &receiver,
        std::placeholders::_1
        );
    callbacks.raw_callback = callback_table.on_data;
    result                 = ZCFForwardResult(
        callbacks.raw_callback(
            data,
            size,
            &callback_context
            )
        );

    if (FAILED(result))
    {
        goto ErrorCleanup;
    }

    callbacks.on_ready();
    callbacks.on_message("connected");
    result = InvokeMemberCallback(
        &receiver,
        &ZCFSystemCallbackImpl::OnData,
        data,
        size
        );

    if (FAILED(result))
    {
        goto ErrorCleanup;
    }

    result = ZCFNativeStatus(callback_context);

    if (FAILED(result))
    {
        goto ErrorCleanup;
    }

    *callback_count = receiver.ready_count + ZCFDispatchMessage(
        1,
        2
        );

ErrorCleanup:

    if (callback_context != nullptr)
    {
        callback_table.on_close(callback_context);
    }

Cleanup:

    return result;
}

// ============================================================================
// Forward declarations and non-exercise utility function definitions
// ============================================================================
// --- Logging, counters, and exercise forward declarations ---
void
TakeOne(
    int value
    );

void
LogImpl(
    const char* file,
    int line,
    const char* format,
    ...
    );

ZCF_DECLARE_COUNTER(MacroDeclaredCounter) // trailing comment after a declaration-generating macro invocation

int
NormalizeSingleArgument(
    int value
    )
{
    return value;
}

std::vector<int>
MakeValues()
{
    return {1, 2, 3};
}

void
LegacyCreateInt(
    int** output
    );

void
LegacyResetInt(
    int** output
    );

void
ExerciseFormatting(
    std::vector<int>& values
    );

void
ExerciseNumericLiterals();

void
ExerciseMathComputations();

// --- Small factories, lambdas, and friend access helpers ---
int
InspectFriend(
    const AccessAndFriend& value
    )
{
    return value.seed_;
}

auto
MakeLambda(
    int bias
    )
{
    return [bias] (int value) { return value + bias; };
}

auto
MakeCxx23Lambda()
{
    return [] [[nodiscard]](int value) static noexcept->int{return value + 1;};
}

// --- Generic description, addition, and comparison helpers ---
template <typename T>
auto
Describe(
    T value
    ) -> std::enable_if_t<std::is_integral_v<T>, std::string>
{
    return std::string(Kind<T>::name()) + ":" + std::to_string((long long) value);
}

template <typename T, typename U>
decltype(std::declval<T>() + std::declval<U>())
Add(
    T left,
    U right
    )
{
    return left + right;
}

[[nodiscard]] static int
CompareInts(
    const void* left,
    const void* right
    ) noexcept
{
    const auto* lhs = static_cast<const int*>(left);
    const auto* rhs = static_cast<const int*>(right);

    return (*lhs > *rhs) - (*lhs < *rhs);
}

// --- Macro-annotated and language-linkage helpers ---
ZCF_API // trailing comment after a standalone attribute macro invocation
int
MacroAnnotatedFunction(
    int value
    )
{
    return ZCF_JOIN(
        val, // trailing comment after the first token-paste macro argument
        ue   // trailing comment after the final token-paste macro argument
        );   // trailing comment after a token-paste macro invocation
}

ZCF_ATTRIBUTE_DECL(
    int,                   // trailing comment after a declaration macro's type argument
    MacroAttributeFunction // trailing comment after a declaration macro's name argument
    )                      // trailing comment after a declaration macro invocation
{
    return value + 1;
}
ZCF_MULTI_LINE_SUM(
    MacroGeneratedFunction, // trailing comment after a function-generating macro name
    1,                      // trailing comment after a function-generating macro operand
    2                       // trailing comment after the final function-generating macro operand
    )                       // trailing comment after a function-generating macro invocation

extern "C" int
ExportedValue()
{
    return 42;
}

extern "C"
{
int
ExportedBlockValue()
{
    return 43;
}
}
extern "C++"
{
int
ExportedCppBlockValue()
{
    return 44;
}
}
ZCF_DEFINE_COUNTER(
    MacroDeclaredCounter,      // trailing comment after a definition macro's name argument
    ZCF_NESTED_MACRO_VALUE // trailing comment after a nested object-like macro argument
    )                          // trailing comment after a definition-generating macro invocation

// --- Coroutine helpers ---
std::generator<int>
GenerateSequence()
{
    co_yield 1;
    co_yield 2;
    co_return;
}

AwaitableTask
AwaitOnce()
{
    auto value = co_await ImmediateAwaiter {};
    TakeOne(value);
    co_return;
}

// ============================================================================
// EXERCISE FUNCTIONS -- ordered most-common pattern first
// ============================================================================

// ============================================================================
// Format style: comments, trailing-comment alignment, and comment reflow.
// Language feature: ordinary line comments, block comments, Doxygen comments,
// trailing comments on declarations/statements, comment pragmas, raw strings,
// and comments embedded around expressions.
// ============================================================================

// ============================================================================
// Format style: integer and float literal assignments — base prefixes, digit
// separators, type suffixes, fixed-width types, C++23 size/bit-int literals,
// extended float types, special float values, character prefixes, string
// literal prefixes, and hex float notation.
// Language feature: decimal/hex/binary/octal integer literals; all integer
// suffix combinations (u, l, ul, ll, ull and uppercase); digit separators
// inside all bases; fixed-width integer types (int8_t through int64_t, uint*,
// intmax_t, ptrdiff_t, size_t, intptr_t, uintptr_t); C++23 z/uz size-type
// literals; C++23 _BitInt(N); double/float/long-double literals; scientific
// notation; hex floating-point literals; C++23 <stdfloat> types; INFINITY/NAN;
// std::numeric_limits; char/wide-char/UTF-8/UTF-16/UTF-32 character literals;
// narrow/wide/UTF-8/UTF-16/UTF-32/raw string literals.
// ============================================================================
void
ExerciseNumericLiterals()
{
    // --- Integer literals: decimal with all suffix combinations ---
    int dec_plain              = 42;
    unsigned int dec_u         = 42U;
    unsigned int dec_U         = 42U;
    long dec_l                 = 42L;
    long dec_L                 = 42L;
    unsigned long dec_ul       = 42UL;
    unsigned long dec_UL       = 42UL;
    unsigned long dec_uL       = 42UL;
    unsigned long dec_Ul       = 42UL;
    long long dec_ll           = 42LL;
    long long dec_LL           = 42LL;
    unsigned long long dec_ull = 42ULL;
    unsigned long long dec_ULL = 42ULL;

    // --- Integer literals: hexadecimal ---
    int hex_plain              = 0xFF;
    int hex_upper              = 0xFF;
    int hex_prefix             = 0xFF;
    unsigned hex_u             = 0xFFU;
    unsigned long long hex_ull = 0xFFFF'FFFFULL;
    int hex_sep                = static_cast<int>(0xDEAD'BEEFU);
    unsigned hex_mask          = 0xFF00'00FFU;

    // --- Integer literals: binary ---
    int bin_plain              = 0b1010;
    int bin_upper              = 0b1010;
    int bin_sep                = 0b1010'1010;
    unsigned bin_u             = 0b0000'1111U;
    unsigned long long bin_all = 0b1111'0000'1111'0000ULL;

    // --- Integer literals: octal ---
    int oct_plain  = 0755;
    int oct_small  = 007;
    unsigned oct_u = 0777U;

    // --- C++23 ssize_t / size_t literals (0z / 0uz) ---
    auto ssize_zero = 0z;
    auto usize_zero = 0uz;
    auto ssize_val  = 42z;
    auto usize_val  = 42uz;

    // --- Fixed-width integer types ---
    std::int8_t i8       = INT8_MIN;
    std::int16_t i16     = INT16_MIN;
    std::int32_t i32     = INT32_MAX;
    std::int64_t i64     = INT64_MAX;
    std::uint8_t u8      = UINT8_MAX;
    std::uint16_t u16    = UINT16_MAX;
    std::uint32_t u32    = UINT32_MAX;
    std::uint64_t u64    = UINT64_MAX;
    std::intmax_t imax   = INTMAX_MIN;
    std::uintmax_t umax  = UINTMAX_MAX;
    std::ptrdiff_t pdiff = 0;
    std::size_t sz       = 0u;
    std::intptr_t iptr   = 0;
    std::uintptr_t uptr  = 0u;

// --- C++23 _BitInt extended integer type ---
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 14)
    _BitInt(7) bitint7 = 42;
    unsigned _BitInt(8) ubitint8 = 0b1111'0000U;
    TakeOne(static_cast<int>(bitint7 + ubitint8));
#endif

    // --- Float literals: double (no suffix) ---
    double dbl_plain    = 1.5;
    double dbl_sci      = 1.5e10;
    double dbl_neg_sci  = 1.5e-3;
    double dbl_zero     = 0.0;
    double dbl_leading  = .5;
    double dbl_trailing = 5.;

    // --- Float literals: float (f/F suffix) ---
    float flt_plain   = 1.5f;
    float flt_F       = 1.5F;
    float flt_sci     = 1.5e3f;
    float flt_neg_sci = 1.5e-3f;
    float flt_zero    = 0.0f;

    // --- Float literals: long double (l/L suffix) ---
    long double ldbl_plain = 1.5L;
    long double ldbl_L     = 1.5L;
    long double ldbl_sci   = 1.5e10l;

    // --- Hex floating-point literals (C++17) ---
    double hex_flt_d      = 0x1.0p0;
    double hex_flt_d2     = 0x1.8p1;
    float hex_flt_f       = 0x1.0p0f;
    float hex_flt_f2      = 0x1.Cp3f;
    long double hex_flt_l = 0x1.0p0l;

    // --- C++23 <stdfloat> extended float types ---
    std::float16_t f16   = 1.0f;
    std::float32_t f32   = 1.0f;
    std::float64_t f64   = 1.0;
    std::float128_t f128 = 1.0l;
    std::bfloat16_t bf16 = 1.0f;

    // --- Special float values ---
    double inf_pos    = INFINITY;
    double inf_neg    = -INFINITY;
    double nan_val    = NAN;
    double nan_quiet  = std::numeric_limits<double>::quiet_NaN();
    double inf_lim    = std::numeric_limits<double>::infinity();
    float flt_max     = std::numeric_limits<float>::max();
    float flt_min     = std::numeric_limits<float>::lowest();
    float flt_eps     = std::numeric_limits<float>::epsilon();
    double dbl_denorm = std::numeric_limits<double>::denorm_min();

    // --- Character literals: all prefix forms ---
    char c_plain   = '\x41';
    char c_oct     = '\101';
    char c_esc     = '\n';
    wchar_t c_wide = L'A';
    char8_t c_u8   = u8'A';
    char16_t c_u16 = u'A';
    char32_t c_u32 = U'A';

    // --- String literals: all prefix forms ---
    const char* s_narrow      = "narrow literal";
    const wchar_t* s_wide     = L"wide literal";
    const char8_t* s_u8       = u8"utf-8 literal";
    const char16_t* s_u16     = u"utf-16 literal";
    const char32_t* s_u32     = U"utf-32 literal";
    const char* s_raw         = R"(raw "no escapes" needed)";
    const wchar_t* s_raw_w    = LR"(wide raw literal)";
    const char8_t* s_raw_u8   = u8R"(utf-8 raw literal)";
    const char16_t* s_raw_u16 = uR"(utf-16 raw literal)";
    const char32_t* s_raw_u32 = UR"(utf-32 raw literal)";
    const char* s_concat      = "first part "
                                "second part "
                                "third part";

    TakeOne(static_cast<int>(dec_plain + dec_u + dec_U + dec_l + dec_L + dec_ul + dec_UL + dec_uL + dec_Ul + dec_ll + dec_LL + dec_ull + dec_ULL + hex_plain + hex_upper + hex_prefix + hex_u + static_cast<int>(hex_ull) + hex_sep + hex_mask + bin_plain + bin_upper + bin_sep + bin_u + static_cast<int>(bin_all) + oct_plain + oct_small + oct_u + ssize_zero + usize_zero + ssize_val + static_cast<int>(usize_val) + i8 + i16 + i32 + static_cast<int>(i64) + u8 + u16 + u32 + static_cast<int>(u64) + static_cast<int>(imax) + static_cast<int>(umax) + pdiff + static_cast<int>(sz) + iptr + static_cast<int>(uptr) + static_cast<int>(dbl_plain) + static_cast<int>(flt_plain) + static_cast<int>(ldbl_plain) + static_cast<int>(hex_flt_d) + static_cast<int>(hex_flt_d2) + static_cast<int>(hex_flt_f) + static_cast<int>(hex_flt_f2) + static_cast<int>(hex_flt_l) + static_cast<int>(f16) + static_cast<int>(f32) + static_cast<int>(f64) + static_cast<int>(f128) + static_cast<int>(bf16) + static_cast<int>(dbl_sci) + static_cast<int>(dbl_neg_sci) + static_cast<int>(dbl_zero) + static_cast<int>(dbl_leading) + static_cast<int>(dbl_trailing) + static_cast<int>(flt_sci) + static_cast<int>(flt_neg_sci) + static_cast<int>(flt_zero) + static_cast<int>(ldbl_sci) + static_cast<int>(inf_pos) + static_cast<int>(std::isnan(nan_val)) + static_cast<int>(std::isnan(nan_quiet)) + static_cast<int>(std::isinf(inf_lim)) + static_cast<int>(flt_max) + static_cast<int>(flt_min) + static_cast<int>(flt_eps) + static_cast<int>(dbl_denorm) + static_cast<int>(inf_neg) + c_plain + c_oct + c_esc + static_cast<int>(c_wide) + static_cast<int>(c_u8) + static_cast<int>(c_u16) + static_cast<int>(c_u32) + static_cast<int>(s_narrow[0]) + static_cast<int>(s_wide[0]) + static_cast<int>(s_u8[0]) + static_cast<int>(s_u16[0]) + static_cast<int>(s_u32[0]) + static_cast<int>(s_raw[0]) + static_cast<int>(s_raw_w[0]) + static_cast<int>(s_raw_u8[0]) + static_cast<int>(s_raw_u16[0]) + static_cast<int>(s_raw_u32[0]) + static_cast<int>(s_concat[0])));
}

// ============================================================================
// Format style: deeply nested arithmetic, mixed precedence, multiline formulas,
// function-heavy expressions, complex numbers, reductions, and nested loops.
// Language feature: <cmath>, <complex>, <numbers>, dot products, reductions,
// matrix multiplication, Horner polynomial evaluation, midpoint/lerp/gcd/lcm.
// ============================================================================
void
ExerciseMathComputations()
{
    // --- Scalar formulas and standard mathematical functions ---
    constexpr double quadratic_a = 1.0;
    constexpr double quadratic_b = -3.0;
    constexpr double quadratic_c = 2.0;
    double discriminant          = std::fma(
        -4.0 * quadratic_a,
        quadratic_c,
        quadratic_b * quadratic_b
        );
    double root_positive = (-quadratic_b + std::sqrt(discriminant)) / (2.0 * quadratic_a);
    double root_negative = (-quadratic_b - std::sqrt(discriminant)) / (2.0 * quadratic_a);

    double angle                  = std::numbers::pi_v<double>/ 6.0;
    double trigonometric_identity = std::pow(
        std::sin(angle),
        2.0
        )
        + std::pow(
            std::cos(angle),
            2.0
            );
    double geometric_measure = std::hypot(
        3.0,
        4.0
        )
        + std::atan2(
            4.0,
            3.0
            );
    double exponential_identity = std::log(std::exp(2.0)) + std::log1p(std::expm1(0.5));
    double rounded_values       = std::floor(2.75) + std::ceil(2.25) + std::round(2.5) + std::trunc(-2.75);
    double remainder_values     = std::fmod(
        17.5,
        3.0
        )
        + std::remainder(
            17.5,
            3.0
            );
    double interpolation = std::lerp(
        root_negative,
        root_positive,
        0.25
        );
    double constants          = std::numbers::e_v<double>+std::numbers::sqrt2_v<double>+std::numbers::phi_v<double>;
    int integer_number_theory = std::gcd(
        84,
        30
        )
        + std::lcm(
            12,
            18
            );
    int midpoint = std::midpoint(
        10,
        24
        );

    // --- Complex-number arithmetic ---
    std::complex<double> complex_value {3.0, 4.0};
    std::complex<double> complex_result =
        std::pow(
            complex_value,
            2.0
            )
        + std::sqrt(complex_value) + std::polar(
            2.0,
            angle
            );
    double complex_measure = std::abs(complex_result) + std::arg(complex_result) + complex_result.real() - complex_result.imag();

    // --- Accumulation, dot products, and transformed reductions ---
    std::array<double, 4> samples = {1.0, 2.0, 3.0, 4.0};
    std::array<double, 4> weights = {0.5, 1.5, -2.0, 3.0};
    double accumulated            = std::accumulate(
        samples.begin(),
        samples.end(),
        0.0
        );
    double dot_product = std::inner_product(
        samples.begin(),
        samples.end(),
        weights.begin(),
        0.0
        );
    double transformed_reduction = std::transform_reduce(
        samples.begin(),
        samples.end(),
        weights.begin(),
        0.0,
        std::plus<>{},
        [] (double sample, double weight) { return std::fma(sample, weight, sample * sample); }
        );

    // --- Matrix multiplication with nested loops ---
    std::array<std::array<double, 2>, 2> left_matrix =
    {
        {
            {
                1.0, 2.0
            }, {
                3.0, 4.0
            }
        }
    };
    std::array<std::array<double, 2>, 2> right_matrix =
    {
        {
            {
                5.0, 6.0
            }, {
                7.0, 8.0
            }
        }
    };
    std::array<std::array<double, 2>, 2> product_matrix {};

    for (std::size_t row = 0; row < product_matrix.size();)
    {
        for (std::size_t column = 0; column < product_matrix[row].size(); ++column)
        {
            for (std::size_t term = 0; term < right_matrix.size(); ++term)
            {
                product_matrix[row][column] += left_matrix[row][term] * right_matrix[term][column];
            }
        }

        ++row;
    }

    // --- Horner polynomial evaluation ---
    std::array<double, 5> coefficients = {2.0, -3.0, 0.5, 4.0, -1.0};
    double polynomial_x                = 1.25;
    double polynomial_value            = 0.0;

    for (double coefficient : coefficients)
    {
        polynomial_value = std::fma(
            polynomial_value,
            polynomial_x,
            coefficient
            );
    }

    // --- Combined multiline result expression ---
    double combined_result =
        ((root_positive + root_negative) * trigonometric_identity) + ((geometric_measure - exponential_identity) / (1.0 + std::abs(remainder_values))) + (interpolation * constants) + complex_measure + accumulated + dot_product + transformed_reduction + product_matrix[0][1] + product_matrix[1][0] + polynomial_value + rounded_values + integer_number_theory + midpoint;
    TakeOne(static_cast<int>(combined_result));
}

// ============================================================================
// TRAILING LINE COMMENT PLACEMENT
// Format style: end-of-line comment spacing and alignment after declarations,
// template parameters, constraints, base classes, access specifiers, enumerators,
// bit-fields, function signatures, expressions, lambda components, statements,
// control-flow labels, and nested braced initializers.
// Language feature: constrained templates, aggregate bases, scoped enums,
// trailing return types, multiline lambdas, range-for, and switch.
// ============================================================================
struct TrailingCommentBase
{
    int base_value; // trailing comment after a base data member
}; // trailing comment after a base type

struct TrailingCommentMixin
{
    int mixin_value; // trailing comment after a mixin data member
}; // trailing comment after a mixin type

template <
    typename Value,    // trailing comment after a type template parameter
    std::size_t Extent // trailing comment after a non-type template parameter
    >
requires(
    Extent > 0 // trailing comment after a constraint operand
    )
struct TrailingCommentRecord:
    TrailingCommentBase,     // trailing comment after the first base
        TrailingCommentMixin // trailing comment after the final base
{
public: // trailing comment after an access specifier

    enum class State
    {
        idle  = 0, // trailing comment after the first enumerator
        ready = 1, // trailing comment after a middle enumerator
        done  = 2  // trailing comment after the final enumerator
    };

    using value_type = Value; // trailing comment after a type alias

    unsigned enabled  : 1;              // trailing comment after a short bit-field
    unsigned category : 3;              // trailing comment after a longer bit-field
    State state;                        // trailing comment after an enum member
    std::array<Value, Extent> elements; // trailing comment after a template member
}; // trailing comment after a constrained class template

template <
    typename Value,    // trailing comment after a function type parameter
    std::size_t Extent // trailing comment after a function non-type parameter
    >
[[nodiscard]] constexpr auto
SelectTrailingCommentValue(
    const TrailingCommentRecord<Value, Extent>& record,                       // trailing comment after the first parameter
    std::size_t index,                                                        // trailing comment after a middle parameter
    Value fallback                                                            // trailing comment after the final parameter
    ) noexcept(noexcept((index < Extent) ? record.elements[index] : fallback) // trailing comment in noexcept
    )
-> Value
// trailing comment after a trailing return type
requires(
    Extent > 0 // trailing comment after a function constraint
    )
{
    if (index < Extent) // trailing comment after an if condition
    {
        return record.elements[index]; // trailing comment after a return statement
    }

    return fallback; // trailing comment after the fallback return
}

/**
 * Formats a deliberately noisy set of comments.
 *
 * This paragraph is intentionally long so ReflowComments has something to wrap
 * and the output shows how block documentation comments are normalized while
 * preserving useful sentence boundaries for future agents reading the fixture.
 *
 * @param values container used only so the comment examples sit next to real
 * code and not isolated text.
 */
/// Format style: triple-slash Doxygen summary before a function.
/// Language feature: ordinary declaration comments adjacent to block Doxygen.
/*!
 * Format style: Qt/Doxygen bang block comments should remain readable.
 * Language feature: documentation comment variant used by many C++ projects.
 */
void
ExerciseComments(
    std::vector<int>& values
    )
{
    // Format style: line comments and comment pragma preservation.
    // Language feature: IWYU-style pragma comments and ordinary explanatory notes.
    // IWYU pragma: keep
    // region formatter comment marker coverage
    // TODO(formatter): keep task-style comments stable during formatting.
    // FIXME(formatter): preserve issue-style comments near code.
    // clang-format off
    int clang_format_off_value = 3;
    // clang-format on
    int short_name       = 1;                             // trailing comment: short declaration
    int much_longer_name = 2;                             // trailing comment: longer declaration
    int expression       = short_name + much_longer_name; // trailing comment: expression spacing
    int nolint_value     = expression;                    // NOLINT(readability-magic-numbers)

    // endregion formatter comment marker coverage

    struct DocumentedMembers
    {
        int value; /**< Format style: trailing Doxygen member comment. */
        int other; ///< Language feature: member declaration with line Doxygen.
    }; // trailing comment after a local type

    using CommentRecord = TrailingCommentRecord<int, 3>; // trailing comment after a local alias
    CommentRecord commented_record                       // trailing comment between a declarator and initializer
    {
        // trailing comment after an opening initializer brace
        {short_name},                // trailing comment after the first base initializer
        {much_longer_name},          // trailing comment after the second base initializer
        1U,                          // trailing comment after a bit-field initializer
        3U,                          // trailing comment after a second bit-field initializer
        CommentRecord::State::ready, // trailing comment after a scoped enumerator
        {
            short_name, // trailing comment at aggregate element depth one
            expression, // aligned comment at aggregate element depth one
            Add(
                short_name,
                much_longer_name
                ) // trailing comment after a call expression
        }  // trailing comment after an inner closing brace
    }; // trailing comment after an aggregate declaration

    DocumentedMembers documented
    {
        .value = clang_format_off_value, // comment inside designated initializer list
        .other = expression              // aligned comment inside initializer list
    }; // trailing comment after a designated initializer
    std::array commented_matrix
    {
        std::array
        {
            short_name, // trailing comment at matrix row zero, column zero
            SelectTrailingCommentValue(
                commented_record,
                1,
                0
                ) // trailing comment after a templated call
        }, // trailing comment after the first matrix row
        std::array
        {
            documented.value, // trailing comment at matrix row one, column zero
            documented.other  // trailing comment at matrix row one, column one
        }  // trailing comment after the final matrix row
    }; // trailing comment after a nested std::array
    values.push_back(
        Add(
            documented.value, // comment inside call argument list
            documented.other  // second argument comment in a multiline call
            )
        );
    auto commented_parameter_lambda = [] (
        int left, /* block comment inside parameter list */
        int right // line comment inside parameter list
        ) { return left + right; };
    values.push_back(
        commented_parameter_lambda(
            nolint_value,
            documented.other
            )
        );

    int continued_expression =
        commented_record.base_value     // trailing comment after a continued binary operator
        + commented_record.mixin_value  // aligned comment after a second continued operator
        + commented_record.elements[2]; // trailing comment after the final expression operand
    int selected_expression =
        (continued_expression > 0) ? // trailing comment after a conditional test
        commented_record.elements[0]
        :                             // trailing comment after the true operand
        commented_record.elements[1]; // trailing comment after the false operand
    std::string chained_text =
        std::string("comments") // trailing comment after a call-chain root
        .append("-across")      // trailing comment after an intermediate member call
        .append("-lines");      // trailing comment after the final member call

    auto multiline_comment_lambda =
        [short_name, // trailing comment after a value capture
            &values  // trailing comment after a reference capture
        ] (
            int left,         // trailing comment after the first lambda parameter
            int right         // trailing comment after the final lambda parameter
            ) noexcept -> int // trailing comment after lambda specifiers
        {
            int local_sum = left + right + short_name + static_cast<int>(values.size()); // trailing comment in a lambda body

            return local_sum; // trailing comment after a lambda return
        }; // trailing comment after a multiline lambda declaration

    values.push_back(
        multiline_comment_lambda(
            selected_expression, // trailing comment after the first lambda argument
            documented.other     // trailing comment after the final lambda argument
            )
        ); // trailing comment after a multiline call statement

    for (const auto& row : commented_matrix) // trailing comment after an outer range-for
    {
        for (int item : row) // trailing comment after an inner range-for
        {
            if ((item % 2) == 0) // trailing comment after a nested condition
            {
                values.push_back(item); // trailing comment in the true branch
            }
            else // trailing comment after an else keyword
            {
                values.push_back(-item); // trailing comment in the false branch
            }
        }
    }

    switch (commented_record.state) // trailing comment after a switch condition
    {
        case CommentRecord::State::idle: // trailing comment after the first case label
        {
            values.push_back(0); // trailing comment inside the first case
            break;               // aligned comment after the first break
        }
        case CommentRecord::State::ready: // trailing comment after a middle case label
        {
            values.push_back(continued_expression); // trailing comment inside the middle case
            break;                                  // aligned comment after the middle break
        }
        case CommentRecord::State::done: // trailing comment after the final case label
        {
            values.push_back(selected_expression); // trailing comment inside the final case
            break;                                 // aligned comment after the final break
        }
    }

    std::ostringstream commented_stream; // trailing comment after a stream declaration
    commented_stream
        << short_name                                                                        // trailing comment after the first insertion
        << ':'                                                                               // aligned comment after a separator insertion
        << much_longer_name;                                                                 // trailing comment after the final insertion
    values.push_back(static_cast<int>(commented_stream.str().size() + chained_text.size())); // trailing comment after a long statement

    /* Format style: block comments next to control-flow braces.
       Language feature: comment before an if-statement and another comment inside
       a compact block. */
    if (expression > 0)
    { /* inline block comment before statement */
        values.push_back(expression);
    }

    //! Format style: Doxygen line comment before a local lambda.
    //! Language feature: lambda with a trailing comment on invocation.
    auto comment_lambda = [] (int value) { return value + 1; }; // trailing comment after lambda declaration
    values.push_back(comment_lambda(expression));               ///< Doxygen trailing comment on a statement.

    /*
    Format style: long block comment reflow.
    Language feature: raw string literal below keeps punctuation, braces, brackets,
    and angle brackets visible to the formatter while the surrounding comment is a
    separate reflow target that should not alter the literal content.
    */
    const char* raw        = R"(// not a real comment inside a raw string
/* not a real block comment either */
template <class T> requires true
)";
    const char* custom_raw = R"wfmt(TODO and FIXME inside a custom delimiter raw string are not comments.)wfmt";
    values.push_back(static_cast<int>(raw[0]));
    values.push_back(static_cast<int>(custom_raw[0]));
}

// ============================================================================
// Format style: control-flow braces, short statements, switch indentation,
// labels, and preprocessor blocks inside a function body.
// Language feature: if/else, classic/range/init-statement loops, switch/case,
// try/catch, Windows SEH, goto labels, C++23 aliases in init-statements, and
// size_t literals.
// ============================================================================
void
ExerciseControl(
    Mode mode,
    std::vector<int>& values
    )
{
    unsigned mask             = 0xFFU + 0b1010U + 077U + 1'000U;
    auto signed_size          = 0z;
    auto unsigned_size        = 0uz;
    const char* escaped_names = "\N{LATIN CAPITAL LETTER A}\x{41}\o{101}\u{0041}";
    [[assume(ZCF_PLATFORM_VALUE > 0)]];
    static_assert(1);

    if constexpr (1)
    {
        values.push_back(static_cast<int>(signed_size + unsigned_size));
    }

    if (using Size = std::size_t; (values.size() > Size {0}))
    {
        values.front() += static_cast<int>(escaped_names[0]);
    }

    for (using Value = int; Value item : MakeValues())
    {
        values.push_back(item);
    }

    for (auto item : MakeValues())
    {
        values.push_back(item);
    }

    bool should_adjust_front =
        !values.empty() &&
        (values.size() <= values.max_size()) &&
        ((mode == Mode::Alpha) || (mode == Mode::Beta) || (mode == Mode::Gamma)) &&
        ((mask & 0x000FU) == 0x000AU) &&
        ((signed_size == 0) || (unsigned_size == 0)) &&
        (escaped_names != nullptr);

    if (should_adjust_front)
    {
        values.front() += 1;
    }
    else if (values.size() > 1)
    {
        values.back() -= 1;
    }
    else
    {
        values.push_back(0);
    }

    for (Index i = 0; i < values.size(); ++i)
    {
        values[i] += static_cast<int>(i);
    }

    for (auto& value : values)
    {
        value = (value < 0) ? -value : value;
    }

    for (
        auto pairs = std::array
        {
            std::pair {1, 2},
            std::pair {3, 4}
        };
        const auto& [left, right] : pairs
        )
    {
        values.push_back(left + right);
    }

    for (Index left = 0, right = values.empty() ? 0 : (values.size() - 1); left < right; ++left, --right)
    {
        values[left] += values[right];
    }

    for (Index remaining = values.size(); remaining > 0;)
    {
        --remaining;
        values[remaining] += static_cast<int>(remaining);
    }

    for (int single_pass = 0;; ++single_pass)
    {
        values.push_back(single_pass);
        break;
    }

    for (auto it = values.begin(); it != values.end(); ++it)
    {
        *it += 1;
    }

    for (std::vector<int>::const_iterator it = values.cbegin(); it != values.cend(); ++it)
    {
        mask += static_cast<unsigned>(*it > 0);
    }

    for (auto rit = values.rbegin(); rit != values.rend(); ++rit)
    {
        values.front() += *rit;
        break;
    }

    while (mask > 0u)
    {
        mask >>= 1U;
    }

    do
    {
        mask++;
    }
    while (mask < 2U);

    switch (mode)
    {
        case Mode::Alpha:
        {
            values.push_back(1);
            break;
        }
        case Mode::Beta:
        {
            values.push_back(2);
            break;
        }
        default:
        {
            std::unreachable();
        }
    }

    try
    {
        if (values.empty())
        {
            throw std::runtime_error("empty");
        }
    }
    catch (const std::exception& ex)
    {
        values.push_back(static_cast<int>(std::string(ex.what()).size()));
    }

    std::exception_ptr captured_exception;
    try
    {
        throw std::runtime_error("captured");
    }
    catch (...)
    {
        captured_exception = std::current_exception();
    }

    try
    {
        if (captured_exception)
        {
            std::rethrow_exception(captured_exception);
        }
    }
    catch (...)
    {
        values.push_back(7);
    }

#ifdef _WIN32
    __try
    {
        values.push_back(4);
    }
    __except (1)
    {
        values.push_back(-4);
    }
#else
    values.push_back(5);
#endif

    if (values.size() > 99)
    {
        goto finished;
    }

    values.push_back(6);
finished:
}

// ============================================================================
// Format style: calls, argument breaking, and comments inside expressions.
// Language feature: nested calls, lambdas, qsort callback, and formatter
// normalizer cases for single-argument calls.
// ============================================================================

void
ExerciseCalls(
    Derived& derived,
    std::vector<int>& values
    )
{
    TakeOne(42);
    TakeOne(
        NormalizeSingleArgument(7)
        );
    TakeOne(
        MakeLambda(3)(4)
        );
    auto total = derived.call(
        1,
        2,
        3
        );
    values.push_back(total);
    std::qsort(
        values.data(),
        values.size(),
        sizeof(int),
        CompareInts
        );
}

// ============================================================================
// Format style: macro invocations and macro-generated language constructs.
// Language feature: statement macros, foreach-style macros, optional-if macros,
// switch/case macros, raw/whitespace-sensitive macros, and multiline macro
// invocations producing declarations and namespaces.
// ============================================================================
void
ExerciseMacros(
    std::vector<int>& values
    )
{
    ZCF_EMPTY_MARKER                                                     // trailing comment after an empty macro invocation
    int macro_object_value   = ZCF_ALIAS_VALUE;                          // trailing comment after an object-like macro expression
    int macro_function_value = ZCF_EVEN_LONGER_MACRO_NAME(1);            // trailing comment after a function-like macro expression
    int macro_nested_value   = ZCF_COMMENTED_MACRO(ZCF_SHORT_MACRO); // trailing comment after nested macro expressions

    ZCF_REQUIRE(!values.empty()); // trailing comment after a statement macro invocation
    ZCF_STATEMENT(values.size()); // aligned comment after another statement macro invocation
    ZCF_EMPTY_FUNCTION();         // trailing comment after a zero-argument function-like macro invocation
    ZCF_TRACE(
        // trailing comment after a macro invocation's opening parenthesis
        "opening-parenthesis-comment"
        );
    ZCF_TRACE(
        "values=%d",                                                                  // trailing comment after a variadic macro's fixed argument
        static_cast<int>(values.size())                                               // trailing comment after its final variadic argument
        );                                                                            // trailing comment after a multiline variadic macro invocation
    ZCF_TRACE("values-without-varargs");                                          // trailing comment after an empty-variadic invocation
    ZCF_TRACE(ZCF_LINE_SPLICING_TEXT);                                        // trailing comment after a nested object-like macro argument
    TakeOne(static_cast<int>(sizeof(ZCF_STRINGIFY(zcf_stringify_argument)))); // trailing comment after a nested stringification macro
    ZCF_MULTI_LINE_VARIADIC_CALL(
        "values=%d",                                              // trailing comment after the fixed argument of a multiline macro
        static_cast<int>(values.size())                           // trailing comment after the final variadic argument
        );                                                        // trailing comment after a multiline forwarding macro invocation
    ZCF_MULTI_LINE_VARIADIC_CALL("values-with-empty-va-opt"); // trailing comment after an empty __VA_OPT__ path
    ZCF_MULTI_LINE_EXPR(
        macro_total,               // trailing comment after a statement macro's declaration argument
        values.size(),             // trailing comment after its first expression argument
        ZCF_NESTED_MACRO_VALUE // trailing comment after its object-like macro argument
        );                         // trailing comment after a multiline statement macro invocation
    ZCF_COMMENTED_CONTINUATION(
        commented_macro_total,                              // trailing comment after a block-commented macro's declaration argument
        values.size(),                                      // trailing comment after its first expression argument
        ZCF_VA_OPT_SENTINEL(ZCF_NESTED_MACRO_VALUE) // trailing comment after a nested variadic macro argument
        );                                                  // trailing comment after a block-commented macro invocation
    foreach(
        int value, // trailing comment after a foreach macro's declaration argument
        values     // trailing comment after a foreach macro's range argument
        )          // trailing comment after a multiline foreach macro header
    {
        TakeOne(value); // trailing comment inside a foreach macro body
    }

    Q_FOREACH (int value, values) // trailing comment after a single-line foreach macro header
    {
        TakeOne(value + ZCF_SHORT_MACRO); // trailing comment after an object-like macro in a body expression
    }

    BOOST_FOREACH(
        int value, // trailing comment after a second foreach macro's declaration argument
        values     // trailing comment after a second foreach macro's range argument
        )          // trailing comment after a second multiline foreach macro header
    {
        TakeOne(ZCF_EVEN_LONGER_MACRO_NAME(value)); // trailing comment after a function-like macro in a body expression
    }
    std::optional<int> maybe = values.empty() ? std::nullopt : std::optional<int>{values.front()};
    KJ_IF_MAYBE(
        found, // trailing comment after an optional-if macro's declaration argument
        maybe  // trailing comment after an optional-if macro's expression argument
        )      // trailing comment after an optional-if macro header
    {
        TakeOne(*found); // trailing comment inside an optional-if macro body
    }
    ZCF_IF_PRESENT(
        maybe,  // trailing comment after an if macro's source argument
        present // trailing comment after an if macro's declaration argument
        )       // trailing comment after a multiline if macro header
    {
        TakeOne(*present + ZCF_LONGER_MACRO_NAME); // trailing comment after an aliased object-like macro expression
    }
    ZCF_SWITCH(values.size()) // trailing comment after a switch macro header
    {
        ZCF_CASE(0):// trailing comment after a case-label macro invocation
        {
            values.push_back(ZCF_COMMENTED_MACRO(1)); // trailing comment after a function-like macro argument
            break;
        }
        ZCF_CASE(1):// trailing comment after a second case-label macro invocation
        {
            values.push_back(MacroDeclaredCounter()); // trailing comment after a macro-generated function call
            break;
        }
        default:
        {
            values.push_back(static_cast<int>(values.size()));
            break;
        }
    }
    auto raw_pair = ZCF_RAW(std::pair<int, int>{1, 2}); // trailing comment after a variadic passthrough macro expression
    macro_generated::GeneratedByMacro generated {raw_pair.first + raw_pair.second};
    MacroGeneratedRecord record {raw_pair.first, raw_pair.second};
    macro_multiline_namespace::MultilineNamespaceRecord namespace_record {record.total()};
    TakeOne(generated.get() + macro_object_value + macro_function_value + macro_nested_value); // trailing comment after values produced through macros
    TakeOne(namespace_record.get());                                                           // trailing comment after a macro-generated type's member call
}

// ============================================================================
// Format style: template declarations, trailing return types, constexpr blocks,
// labels, and coroutine return-type placement.
// Language feature: concepts, if constexpr, if consteval, goto in constexpr
// functions, and C++23 std::generator coroutine syntax.
// ============================================================================
template <Addable T>
auto
ExerciseTemplates(
    T value
    ) -> decltype(value + 1)
{
    if constexpr (std::is_integral_v<T>)
    {
        return value + 1;
    }
    else
    {
        return value;
    }
}

template <RichRequirement T>
decltype(auto)
ExerciseRequiresReturn(T && value) noexcept(noexcept(value.size()))
requires requires {typename std::remove_reference_t<T>::value_type;}
{
    return (value);
}

template <typename ... Args>
constexpr auto
CountPack(
    Args&&... args
    ) noexcept(noexcept(sizeof...(Args)))
{
    return sizeof...(Args) + sizeof...(args);
}

template <typename T, T... Values>
constexpr auto
SumIntegerSequence(
    std::integer_sequence<T, Values...>
    )
{
    return (Values + ... + T {});
}

template <typename Tuple, std::size_t... Indexes>
constexpr auto
TupleIndexSum(
    Tuple&& tuple,
    std::index_sequence<Indexes...>
    )
{
    return (std::get<Indexes>(std::forward<Tuple>(tuple)) + ...);
}

template <typename T, typename ... Rest>
auto
MakeDecayedVector(
    T&& first,
    Rest&&... rest
    ) -> DecayedVector<T>
requires(
    (std::convertible_to<Rest, std::remove_cvref_t<T>>&& ...)
    )
{
    return {std::forward<T>(first), static_cast<std::remove_cvref_t<T>>(std::forward<Rest>(rest))...};
}

constexpr std::size_t
ExerciseGenericLambdas()
{
    // Generic lambda: operator() is a template with two parameters.
    auto glambda = []<class T>(T a, auto&& b) { return a < b; };

    // Generic lambda: operator() is a template with one parameter pack.
    auto f = []<typename ... Ts>(Ts&&... ts)
        {
            return CountPack(std::forward<Ts>(ts)...);
        };

    return static_cast<std::size_t>(glambda(
        1,
        2
        ))
           + f(
               1,
               2
               );
}

template <Addable T>
auto
ExerciseTemplateGaps(
    T value
    )
requires
requires(
    T item
    )
{
    typename TemplateTraits<T>::value_type;
    item + value;
}

{
    StaticBuffer<int, 3> buffer {1, 2, 3};
    auto overload = Overload
    {
        [] (int item) { return item + 1; },
        [] (std::string_view text) { return static_cast<int>(text.size()); }
    };
    auto generic_lambda = [captured = value, &buffer]<typename U, std::size_t N>(std::array<U, N>& array) mutable
    requires(N > 0)
    {
        buffer.template get<0>() += static_cast<int>(array[0]);

        return static_cast<int>(captured) + static_cast<int>(array[0]);
    };
    std::integral auto constrained_auto = ConstrainedIdentity(3);
    auto vector                         = MakeDecayedVector(
        value,
        static_cast<T>(constrained_auto)
        );
    auto sequence_sum = SumIntegerSequence(std::integer_sequence<int, 1, 2, 3>{});
    auto tuple_sum    = TupleIndexSum(
        std::tuple{1, 2, 3},
        std::index_sequence<0, 1, 2>{}
        );
    auto nttp_sum         = SumNonTypeValues<1, 2, 3, 4>();
    auto pointer_category = TemplateTraits<int*>::category;
    auto value_category   = TemplateTraits<T&>::category;
    auto abbreviated      = AbbreviatedTemplate(
        constrained_auto,
        value
        );
    auto generic_lambda_sum = ExerciseGenericLambdas();
    std::array<int, 2> array {4, 5};

    return overload(1) + overload(pointer_category) + overload(value_category) + generic_lambda(array) + buffer.template get<1>() + static_cast<int>(vector.size()) + sequence_sum + tuple_sum + nttp_sum + abbreviated + generic_lambda_sum;
}

template <typename T>
constexpr bool AddExpressionIsNoexcept = noexcept(std::declval<T>() + std::declval<T>());

constexpr int
ExerciseConsteval(
    int value
    )
{
    // *INDENT-OFF*
    if consteval { return 1; }
    else { return value; }
    // *INDENT-ON*
}

constexpr int
ExerciseNotConsteval(
    int value
    )
{
    // *INDENT-OFF*
    if not consteval { return value + 1; }
    else { return value; }
    // *INDENT-ON*
}

constexpr int
ExerciseConstexprControl(
    int value
    )
{
    static constexpr int base = 1;

    if (value < 0)
    {
        goto fallback;
    }

    return value + base;
fallback:

    return base;
}

// ============================================================================
// Format style: uncommon declarations, attributes, casts, pointer-to-member
// operators, allocation operators, asm blocks, and requires expressions.
// Language feature: friend/protected/mutable members, constinit/thread_local,
// using enum, namespace aliases, user-defined literals, explicit(bool),
// defaulted comparison, deleted overloads, dynamic casts, typeid, alignof,
// sizeof..., decltype(auto), function try-blocks, and coroutine co_await.
// ============================================================================
void
ExerciseLanguageGaps(
    Base& base,
    std::vector<int>& values
    )
{
    g_thread_local_counter += 1;
    g_constinit_counter    += g_thread_local_counter;
    AccessAndFriend access {g_constinit_counter};
    ProtectedAccessProbe protected_probe {access.seed()};
    std::atomic<int> cpp_atomic_counter {ZCF_STDATOMIC_FEATURE};
    cpp_atomic_counter.fetch_add(
        1,
        std::memory_order_relaxed
        );

    struct LocalClass
    {
        int value;
    };

    LocalClass local {2};
    Comparable left {1};
    Comparable right {2};
    auto comparison       = left <=> right;
    Permission permission = Permission::Read | Permission::Write;
    OperatorShowcase operator_left {3};
    OperatorShowcase operator_right {1};
    volatile OperatorShowcase volatile_operator {operator_left.value};
    NestedTypeShowcase nested
    {
        {local.value}
    };

    if (operator_left or operator_right)
    {
        values.push_back(static_cast<int>(operator_left));
    }

    ExplicitBool<true> explicit_value {3};
    DeletedAndDefaulted defaulted_value;
    FunctionTryBlock function_try_block {4};
    CustomAllocation* allocated       = new CustomAllocation {5};
    CustomAllocation* allocated_array = new CustomAllocation[2] {
        {6},
        {7}
    };
    MemberPointerRecord record {8};
    int MemberPointerRecord::* data_member = &MemberPointerRecord::member;
    int (MemberPointerRecord::*method_member)(int) const = &MemberPointerRecord::method;
    MemberPointerRecord* record_pointer = &record;
    record.*data_member          += 1;
    record_pointer->*data_member += 1;
    auto member_result     = (record.*method_member)(2);
    auto alias_count       = LegacyCount {42_wf};
    auto literal_text      = "literal view"sv;
    auto auto_pack         = AutoNonTypeParameter<13>{};
    auto template_template = TemplateTemplateUse<Box, int>
    {
        Box<int>
        {
            14
        }
    };
    auto& rich_range          = ExerciseRequiresReturn(values);
    decltype(auto)first_value = (rich_range.front());
    auto count_pack           = CountPack(
        1,
        2,
        3
        );
    constexpr auto compile_time = CompileTimeOnly(5);
    constexpr bool noexcept_add = AddExpressionIsNoexcept<int>;
    int \u03C0_value            = 3;
    auto alignment              = alignof(CustomAllocation);
    auto type_name              = typeid(base).name();

    if (auto* derived = dynamic_cast<Derived*>(&base); (derived != nullptr and not values.empty())) [[likely]]
    {
        TakeOne(derived->id());
    }
    else [[unlikely]]
    {
        TakeOne(static_cast<int>(type_name[0]));
    }

    const int frozen = 9;
    int& unfrozen    = const_cast<int&>(frozen);
    TakeOne(unfrozen);
    std::uintptr_t raw_pointer = reinterpret_cast<std::uintptr_t>(allocated);
#if defined(__GNUC__) || defined(__clang__)
    asm volatile ("" : "+r" (raw_pointer));
#endif

    switch (static_cast<Mode>(Alpha))
    {
        case Alpha:
            values.push_back(static_cast<int>(comparison < 0));
            [[fallthrough]];
        case Beta:
            values.push_back(static_cast<int>(alias_count + literal_text.size() + auto_pack.value + template_template.wrapped.value + std::to_underlying(permission)));
            break;
        case Gamma:
            values.push_back(static_cast<int>(alignment + count_pack + compile_time + noexcept_add + member_result + first_value + \u03C0_value));
            break;
    }

    [[maybe_unused]] language_gap_label:
    Flags designated {.enabled = 1, .category = 2, .reserved = 0};
    (void) defaulted_value;
    TakeOne(static_cast<int>(designated.enabled + InspectFriend(access) + protected_probe.expose() + explicit_value.value + cpp_atomic_counter.load() + operator_left.lvalue_only() + OperatorShowcase{4}.rvalue_only() + volatile_operator.observe() + nested.inner.value));
    TakeOne(CarriesDependency(VendorAttributedInline(static_cast<int>(raw_pointer))));
    TakeOne(function_try_block.value + MacroAttributeFunction(static_cast<int>(raw_pointer)));
    delete allocated;
    delete[] allocated_array;
    AwaitOnce();
}

// ============================================================================
// Format style: long expressions, chained calls, pipeline indentation, template
// argument spacing, braced-init lists, and nested lambdas.
// Language feature: C++23 explicit object parameters, static lambdas, auto
// decay-copy, expected/optional monads, ranges, mdspan, print, spanstream,
// stacktrace, flat containers, out_ptr/inout_ptr, and charconv.
// ============================================================================
void
ExerciseCxx23SyntaxAndLibrary(
    std::vector<int>& values
    )
{
    ExplicitObjectParameter object {3};
    auto explicit_this_result = object.get();
    auto moved_object         = std::move(object).add(4);
    Tensor3D tensor;
    tensor[0, 1, 1] = StaticCallAndSubscript::operator[](
        1,
        2
        );
    auto static_call = StaticCallAndSubscript {}(
        explicit_this_result,
        moved_object.value
        );
    auto lambda_with_attributes = MakeCxx23Lambda();
    auto no_paren_lambda        = [] static { return 5; };
    auto copied                 = auto(static_call);
    auto braced                 = auto{copied + lambda_with_attributes(no_paren_lambda())};
    InheritedBox inherited_box(braced);
    std::expected<int, std::string> expected_value = inherited_box.value;
    auto monadic_expected                          = expected_value.and_then([] (int value) { return std::expected<int, std::string>{value + 1}; }).transform([] (int value) { return value * 2; }).or_else([] (const std::string& error) { return std::expected<int, std::string>{static_cast<int>(error.size())}; });
    std::optional<int> optional_value              = 1;
    auto monadic_optional                          = optional_value.transform([] (int value) { return value + 1; }).and_then([] (int value) { return std::optional<int>{value* 2}; }).or_else([] { return std::optional<int>{0}; });
    std::move_only_function<int(int)> move_only    = [] (int value) { return value + 1; };
    auto bound                                     = std::bind_back(
        Add<int, int>,
        1
        );
    auto swapped   = std::byteswap(0x1234U);
    auto forwarded = std::forward_like<ExplicitObjectParameter &&>(moved_object);
    auto invoked   = std::invoke_r<int>(
        move_only,
        static_cast<int>(swapped)
        );
    auto underlying = std::to_underlying(Mode::Gamma);
    auto tuple_like = std::tuple
    {
        std::pair {invoked, underlying}
    };
    std::flat_map<int, std::string> flat_map
    {
        {1, "one"},
        {2, "two"}
    };
    std::flat_multimap<int, std::string> flat_multimap
    {
        {1, "one"},
        {1, "uno"}
    };
    std::flat_set<int> flat_set {1, 2, 3};
    std::flat_multiset<int> flat_multiset {1, 1, 2};
    std::mdspan<int, std::dextents<std::size_t, 2>> view(values.data(), 1, values.size());
    std::string text = "alpha";
    bool has_alpha   = text.contains("alpha");
    std::string_view text_view {text};
    bool view_has_alpha = text_view.contains("alpha");
    text.resize_and_overwrite(
        8,
        [] (char* data, std::size_t count) { std::fill_n(data, count, 'x'); return count; }
        );
    auto sliced = std::move(text).substr(
        1,
        3
        );
    auto range_pipeline = values | std::views::chunk(2) | std::views::transform([] (auto chunk) { return chunk; }) | std::views::join;
    auto zipped         = std::views::zip(
        values,
        values
        );
    auto adjacent           = values | std::views::adjacent<2>;
    auto adjacent_transform = values | std::views::adjacent_transform<2>([] (int left, int right) { return left + right; });
    auto cartesian          = std::views::cartesian_product(
        values,
        values
        );
    auto chunked    = values | std::views::chunk_by([] (int left, int right) { return left == right; });
    auto enumerated = values | std::views::enumerate;
    auto joined     = std::views::single(values) | std::views::join_with(std::views::single(0));
    auto repeated   = std::views::repeat(
        1,
        3
        );
    auto slided          = values | std::views::slide(2);
    auto strided         = values | std::views::stride(2);
    auto zip_transformed = std::views::zip_transform(
        [] (int left, int right) { return left + right; },
        values,
        values
        );
    auto as_const         = values | std::views::as_const;
    auto as_rvalue        = values | std::views::as_rvalue;
    auto closure_pipeline = values | IdentityClosure {};
    auto as_vector        = std::ranges::to<std::vector<int>>(range_pipeline);
    auto starts           = std::ranges::starts_with(
        values,
        as_vector
        );
    auto ends = std::ranges::ends_with(
        values,
        as_vector
        );
    auto contains = std::ranges::contains(
        values,
        1
        );
    auto contains_subrange = std::ranges::contains_subrange(
        values,
        as_vector
        );
    auto last = std::ranges::find_last(
        values,
        1
        );
    auto folded = std::ranges::fold_left(
        values,
        0,
        std::plus<>{}
        );
    std::ranges::iota(
        values,
        0
        );
    std::ranges::shift_left(
        values,
        1
        );
    std::ranges::shift_right(
        values,
        1
        );
    std::stack<int> stack(values.begin(), values.end());
    std::queue<int> queue(values.begin(), values.end());
    std::unique_ptr<int> out_ptr_value;
    LegacyCreateInt(std::out_ptr(out_ptr_value));
    LegacyResetInt(std::inout_ptr(out_ptr_value));
    std::allocator<int> allocator;
    auto allocation = allocator.allocate_at_least(2);
    auto lifetime   = std::start_lifetime_as<int>(allocation.ptr);
    char number_buffer[32] {};
    auto [number_end, number_error] = std::to_chars(
        number_buffer,
        number_buffer + sizeof(number_buffer),
        folded
        );
    int parsed_number = 0;
    std::from_chars(
        number_buffer,
        number_end,
        parsed_number
        );
    std::print(
        "{} {} {}",
        has_alpha,
        view_has_alpha,
        sliced
        );
    std::println(
        "{}",
        folded
        );
    std::fstream exclusive_file("zcf.tmp", std::ios::out | std::ios::noreplace);
    char buffer[128] {};
    std::ospanstream stream(std::span<char>(buffer));
    stream << static_cast<const volatile void*>(values.data()) << std::stacktrace::current();
    std::float32_t float_value = 1.0f;
    TakeOne(static_cast<int>(braced + forwarded.value + monadic_expected.value() + monadic_optional.value() + bound(2) + std::tuple_size_v<decltype(tuple_like)>+flat_map.size() + flat_multimap.size() + flat_set.size() + flat_multiset.size() + view.extent(1) + starts + ends + contains + contains_subrange + (last.empty() ? 0 : *last.begin()) + as_vector.size() + closure_pipeline.size() + stack.size() + queue.size() + parsed_number + reinterpret_cast<std::uintptr_t>(lifetime) + static_cast<int>(float_value)));
}

// ============================================================================
// Format style: library-heavy expressions, scoped locks, visitor indentation,
// path operators, regex construction, and C atomic function-style calls.
// Language feature: source_location, filesystem, variant/visit, any, apply,
// chrono literals, regex, mutex/jthread, bit_cast, launder, assume_aligned,
// and C stdatomic APIs.
// ============================================================================
void
ExerciseLibraryGaps(
    std::vector<int>& values
    )
{
    auto location                          = std::source_location::current();
    std::filesystem::path path             = std::filesystem::path {"root"} / "folder" / "file.cpp";
    std::variant<int, std::string> variant = std::string {"variant"};
    auto visited                           = std::visit(
        Overload{[] (int value) { return value; }, [] (const std::string& text) { return static_cast<int>(text.size()); }},
        variant
        );
    std::any any_value = visited;
    auto any_number    = std::any_cast<int>(any_value);
    auto applied       = std::apply(
        [] (auto... items) { return (... + items); },
        std::tuple
        {
            1,
            2,
            3
        }
        );
    auto duration = 250ms + std::chrono::seconds {1};
    std::regex regex {"[a-z]+"};
    bool regex_matched = std::regex_match(
        std::string{"formatter"},
        regex
        );
    std::mutex mutex;
    {
        std::scoped_lock lock(mutex);
        values.push_back(applied);
    }
    // *INDENT-OFF*
    std::jthread worker([](std::stop_token token) {if(token.stop_requested()){return;} });
    auto bits = std::bit_cast<std::uint32_t>(1.0f);
    alignas(int) std::byte storage[sizeof(int)]{};
    std::memcpy(storage, &bits, sizeof(int));
    auto laundered = std::launder(reinterpret_cast<int*>(storage));
    auto aligned = std::assume_aligned<alignof(int)>(values.data());
    atomic_int c_atomic;
    atomic_init(&c_atomic, 0);
    atomic_fetch_add(&c_atomic, 1);
    auto c_loaded = atomic_load(&c_atomic);
    std::optional<int> maybe = any_number;
    bool optional_equal = maybe == std::optional<int>{any_number};
    TakeOne(static_cast<int>(location.line() + path.string().size() + visited + any_number + std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() + regex_matched + bits + *laundered + aligned[0] + c_loaded + optional_equal));
    // *INDENT-ON*
}

// ============================================================================
// Format style: end-to-end orchestration -- most-common patterns first, rarest
// last. Object setup, comments/control/calls, formatting, operators, templates,
// language gaps, C++23 library, rare inheritance, explicit destructor call.
// Language feature: placement new, raw-pointer arithmetic/null/void**/atomic
// patterns, custom deleters, smart pointers, volatile pointer chains,
// structured bindings, C allocation APIs, and explicit destructor calls.
// ============================================================================
void
ExerciseObjects()
{
    // --- Core objects, placement new, and pointer declarations ---
    using PlacedInt = int;
    alignas(PlacedInt) unsigned char storage[sizeof(PlacedInt)];
    PlacedInt* placed = new(storage) PlacedInt(17);
    OwnedHandle handle(new int(*placed));
    std::vector<int> values = {3, 1, 2};
    auto shared             = std::make_shared<const int>(9);
    Derived derived(*handle, std::make_unique<int>(8), shared);
    const volatile int scalar                     = 11;
    const volatile int* pointer                   = &scalar;
    const volatile int* const* pointer_to_pointer = &pointer;
    int output                                    = 0;
    int raw_values[]                              = {4, 5, 6};
    int* raw_begin                                = raw_values;
    int* raw_end                                  = raw_values + 3;
    int* raw_cursor                               = raw_begin;
    int* maybe_raw                                = nullptr;
    derived.pointers(
        pointer_to_pointer,
        output
        );
    std::array<std::byte, 4> callback_data {};
    int callback_count   = 0;
    auto callback_result = ExerciseRdcoreSystemPatterns(
        callback_data.data(),
        callback_data.size(),
        &callback_count
        );
    TakeOne(static_cast<int>(callback_result) + callback_count);

    // --- Raw-pointer arithmetic, erasure, atomics, and lifetime APIs ---
    for (; raw_cursor != raw_end; ++raw_cursor)
    {
        output += *raw_cursor;
    }

    if (maybe_raw == nullptr)
    {
        maybe_raw = raw_begin;
    }

    int* raw_array[2]  = {maybe_raw, raw_end - 1};
    void* erased       = static_cast<void*>(raw_array[0]);
    void** erased_slot = &erased;
    *erased_slot = static_cast<void*>(raw_array[1]);
    auto* restored = static_cast<int*>(*erased_slot);
    std::atomic<int*> atomic_pointer {restored};
    int* previous_raw = atomic_pointer.exchange(raw_begin);
    std::allocator<int> object_allocator;
    int* constructed = object_allocator.allocate(1);
    std::construct_at(
        constructed,
        previous_raw[0] + raw_begin[1]
        );
    int* constructed_address = std::to_address(constructed);
    int* c_buffer            = static_cast<int*>(std::calloc(
        2,
        sizeof(int)
        ));
    int* c_resized = (c_buffer != nullptr) ? static_cast<int*>(std::realloc(
        c_buffer,
        3 * sizeof(int)
        )) :
        nullptr;

    if (c_resized != nullptr)
    {
        c_resized[2] = raw_end - raw_begin;
        TakeOne(output + raw_begin[0] + raw_array[1][0] + *constructed_address + c_resized[2]);
    }
    else
    {
        TakeOne(output + raw_begin[0] + raw_array[1][0] + *constructed_address);
    }

    std::destroy_at(constructed);
    object_allocator.deallocate(
        constructed,
        1
        );
    std::free((c_resized != nullptr) ? c_resized : c_buffer);

    // --- Feature exercise orchestration ---
    ExerciseComments(values);
    ExerciseControl(
        Mode::Alpha,
        values
        );
    ExerciseCalls(
        derived,
        values
        );
    ExerciseNumericLiterals();
    ExerciseMathComputations();

    // --- Value-type operator usage ---
    Counter ctr {0};
    ++ctr;
    ctr++;
    --ctr;
    ctr--;
    ctr += 3;
    ctr -= 1;
    Counter rem = ctr % 3;
    TakeOne(rem.value);
    constexpr Vec2 zero;
    constexpr Vec2 a {3.0, 4.0};
    constexpr Vec2 b {1.0, 2.0};
    auto vsum      = a + b;
    auto vdiff     = a - b;
    auto vscaled   = a * 2.0;
    auto vdivided  = a / 2.0;
    auto vscaled2  = 2.0 * a;
    Vec2 mutable_v = a;
    mutable_v += b;
    mutable_v -= b;
    mutable_v *= 2.0;
    mutable_v /= 2.0;
    bool veq  = (a == a);
    bool vneq = (a != b);
    std::cout << a << "\n";
    TakeOne(static_cast<int>(vsum.x + vdiff.x + vscaled.x + vdivided.x + vscaled2.x + mutable_v.x + zero.x) + static_cast<int>(veq) + static_cast<int>(vneq));

    // --- Macros, formatting, and template exercises ---
    ExerciseMacros(values);
    ExerciseFormatting(values);
    auto [first, second] = std::pair<int, int>{1, 2};
    auto result = Describe(first) + ":" + Describe(second) + ":" + std::to_string(ExerciseTemplates(output));
    TakeOne(static_cast<int>(result.size()));
    TakeOne(ExerciseTemplateGaps(output));
    Base& base_ref = derived;
    ExerciseLanguageGaps(
        base_ref,
        values
        );

    // --- RAII and custom pointer usage ---
    {
#pragma warning(push)
#pragma warning(disable : 4996)
        Resource r;
        r.int_payload = 42;
        r.state       = Resource::State::Active;
        TakeOne(Resource::instance_count + r.int_payload + static_cast<int>(r.state));
#pragma warning(pop)
    }
    TakeOne(Resource::instance_count);
    LegacyHandle<int> h1(new int(7));
    LegacyHandle<int> h2(std::move(h1));
    h2 = LegacyHandle<int>(new int(8));
    TakeOne(*h2 + (*h2.operator->()) + LegacyHandle<int>::live_count);

    // --- C++23 library and class hierarchy exercises ---
    ExerciseCxx23SyntaxAndLibrary(values);
    ExerciseLibraryGaps(values);
    Rectangle rect(3.0, 4.0, "test", 1, {1.0, 2.0, 3.0});
    rect.scale(2.0);
    TakeOne(static_cast<int>(rect.area() + rect.perimeter()));
    std::unique_ptr<IShape> shape = std::make_unique<Rectangle>(
        1.5,
        2.5,
        "dynamic",
        2,
        std::vector<double>{}
        );
    TakeOne(static_cast<int>(shape->area()));
    Repository<int> repo("test_repo");
    repo.insert(1);
    repo.insert(2);
    repo.insert(3);
    auto big     = repo.count_if([] (int v) { return v > 1; });
    auto doubled = repo.transform_all([] (const int& v) { return v * 2; });
    TakeOne(static_cast<int>(repo.size() + big + doubled.front()));
    DiamondDerived diamond;
    DiamondBase& diamond_base = diamond;
    TakeOne(diamond.compute() + diamond_base.compute());
    CovariantDerived* cov = static_cast<CovariantDerived*>(static_cast<CovariantBase&>(*(new CovariantDerived())).clone());
    TakeOne(static_cast<int>(cov != nullptr));
    delete cov;
    WithProtectedInheritance prot;
    WithPrivateInheritance priv;
    TakeOne(prot.get() + priv.get());
    placed->~PlacedInt();
}

} // namespace coverage

} // namespace zcf

// ============================================================================
// Format style: template specialization outside its primary namespace, parse/
// format method bodies, constexpr iterator return types.
// Language feature: std::formatter<T> customization point with full parse() and
// format() implementation for Comparable so it can be used with std::format.
// ============================================================================
template <>
struct std::formatter<zcf::coverage::Comparable>
{
    constexpr auto
    parse(
        std::format_parse_context& ctx
        )
    {
        auto it = ctx.begin();

        if ((it != ctx.end()) && (*it == 'v'))
        {
            ++it;
            verbose_ = true;
        }

        return it;
    }

    auto
    format(
        const zcf::coverage::Comparable& c,
        std::format_context& ctx
        ) const
    {
        if (verbose_)
        {
            return std::format_to(
                ctx.out(),
                "Comparable{{value={}}}",
                c.value
                );
        }

        return std::format_to(
            ctx.out(),
            "{}",
            c.value
            );
    }

private:

    bool verbose_ = false;
};

namespace zcf
{

namespace coverage
{

// ============================================================================
// Format style: std::format short/long/multiline calls, format_to, vformat,
// formatted_size, custom formatter usage, cout << chains, printf, snprintf,
// ostringstream, and to_string concatenation.
// Language feature: all C++ text-formatting and output approaches in one section.
// ============================================================================
void
ExerciseFormatting(
    std::vector<int>& values
    )
{
    auto short_fmt = std::format(
        "{}",
        values.size()
        );
    auto multi_fmt = std::format(
        "{} {} {}",
        values.size(),
        values.front(),
        values.back()
        );
    auto spec_fmt = std::format(
        "size={:04d} front={:+.2f} back={:#010x} label={}",
        static_cast<int>(values.size()),
        static_cast<double>(values.front()),
        static_cast<unsigned>(values.back()),
        short_fmt
        );
    auto multiline_fmt = std::format(
        "zcf formatter coverage:\n"
        "  size    = {:>8}\n"
        "  front   = {:>8}\n"
        "  back    = {:>8}\n"
        "  derived = {}\n",
        values.size(),
        values.front(),
        values.back(),
        multi_fmt
        );
    std::string fmt_target;
    std::format_to(
        std::back_inserter(fmt_target),
        "[format_to] size={} spec={}",
        values.size(),
        spec_fmt
        );
    char bounded_buffer[64] {};
    auto fmt_to_n_result = std::format_to_n(
        bounded_buffer,
        sizeof(bounded_buffer) - 1,
        "format_to_n: size={} front={}",
        values.size(),
        values.front()
        );
    *fmt_to_n_result.out = '\0';
    auto fmt_size = std::formatted_size(
        "{:08d}",
        static_cast<int>(values.size())
        );
    std::string runtime_fmt_str = "{} items";
    auto vfmt_result            = std::vformat(
        runtime_fmt_str,
        std::make_format_args(values.size())
        );
    Comparable cmp {static_cast<int>(values.size())};
    auto cmp_default = std::format(
        "{}",
        cmp
        );
    auto cmp_verbose = std::format(
        "{:v}",
        cmp
        );
    std::cout << "short chain: " << values.size() << "\n";
    std::cout
        << "multiline chain start\n"
        << "  size  = " << values.size() << "\n"
        << "  front = " << values.front() << "\n"
        << "  back  = " << values.back() << "\n"
        << "  fmt   = " << multiline_fmt
        << std::flush;
    printf(
        "[printf] size=%zu front=%d back=%d\n",
        values.size(),
        values.front(),
        values.back()
        );
    char snprintf_buffer[128] {};
    snprintf(
        snprintf_buffer,
        sizeof(snprintf_buffer),
        "[snprintf] size=%zu front=%d",
        values.size(),
        values.front()
        );
    std::ostringstream oss;
    oss << "oss: " << values.size() << " items, front=" << values.front()
        << ", fmt_size=" << fmt_size;
    auto oss_str = oss.str();
    auto concat  = std::string("concat: ") + std::to_string(values.size()) + ", " + std::to_string(values.front());
    TakeOne(static_cast<int>(short_fmt.size() + multi_fmt.size() + spec_fmt.size() + multiline_fmt.size() + fmt_target.size() + fmt_to_n_result.size + fmt_size + vfmt_result.size() + cmp_default.size() + cmp_verbose.size() + std::strlen(bounded_buffer) + std::strlen(snprintf_buffer) + oss_str.size() + concat.size()));
}

void
zcf::coverage::TakeOne(
    int value
    )
{
    (void) value;
}

void
zcf::coverage::LogImpl(
    const char* file,
    int line,
    const char* format,
    ...
    )
{
    TakeOne(static_cast<int>(line + format[0] + file[0]));
}

void
zcf::coverage::LegacyCreateInt(
    int** output
    )
{
    *output = new int(1);
}

void
zcf::coverage::LegacyResetInt(
    int** output
    )
{
    delete *output;
    *output = new int(2);
}

// ============================================================================
// DEEP AGGREGATE INITIALIZATION: NESTED ARRAYS AND HETEROGENEOUS RECORDS
// Format style: nested braces for CTAD, explicitly typed, and built-in arrays;
// mixed expressions distributed across root, layer, branch, leaf, and nested
// array levels; designated fields; comma placement; empty or partial
// initialization.
// Language feature: class template argument deduction, multidimensional arrays,
// nested record types, variables, operators, function calls, constructors,
// factories, lambdas, aggregate initialization, and value initialization.
// ============================================================================

struct DeepAggregateVolume
{
    struct Voxel
    {
        int value;
        std::array<int, 3> color;
    };

    struct Plane
    {
        Voxel cells[2][2];
        bool active;
    };

    Plane planes[2];
    std::array<int, 3> dimensions;
};

struct MixedInitializerAggregate
{
    struct Leaf
    {
        int from_variable;
        int from_function_call;
        int from_subscript;
        Box<int> braced_constructor;
        std::function<int(int)> stored_lambda;
        std::array<int, 4> nested_expressions;
    };

    struct Branch
    {
        int from_arithmetic;
        int from_standard_call;
        int from_member_call;
        int from_nested_call;
        Box<int> parenthesized_constructor;
        std::optional<int> optional_constructor;
        std::unique_ptr<int> factory_call;
        Leaf leaves[1];
        std::size_t size_expression;
    };

    struct Layer
    {
        int from_conditional;
        int from_static_call;
        int from_template_call;
        int from_callable_object;
        int from_invoke_call;
        int from_cast;
        std::variant<int, std::string> variant_constructor;
        std::tuple<int, std::string> ctad_constructor;
        int (*function_pointer)(
            int
            );
        Branch branches[1];
        bool noexcept_expression;
    };

    std::string string_constructor;
    int from_invoked_lambda;
    int from_generic_lambda;
    Permission enum_expression;
    const int* address_expression;
    std::reference_wrapper<const int> reference_call;
    Layer layers[1];
    std::size_t alignment_expression;
};

void
ExerciseDeepAggregateInitialization()
{
    std::array deduced_matrix
    {
        std::array {1, 2},
        std::array {3, 4},
    };
    std::array deduced_tensor
    {
        std::array
        {
            std::array {1, 2},
            std::array {3, 4},
        },
        std::array
        {
            std::array {5, 6},
            std::array {7, 8},
        },
    };
    std::array commented_tensor // trailing comment between a tensor declarator and initializer
    {
        // trailing comment after the outer tensor brace
        std::array  // trailing comment after the first CTAD plane type
        {
            std::array
            {
                1, // trailing comment at plane zero, row zero, column zero
                Add(
                    1,
                    1
                    ) // trailing comment after a call at the first leaf
            },  // trailing comment after plane zero, row zero
            std::array
            {
                [] { return 3; } (), // trailing comment after a lambda at a nested leaf
                4                    // trailing comment at plane zero, row one, column one
            } // trailing comment after plane zero, row one
        },         // trailing comment after the first tensor plane
        std::array // trailing comment after the second CTAD plane type
        {
            std::array
            {
                static_cast<int>(5.0), // trailing comment after a cast at a nested leaf
                6                      // trailing comment at plane one, row zero, column one
            }, // trailing comment after plane one, row zero
            std::array
            {
                std::abs(-7), // trailing comment after a function call at a nested leaf
                8             // trailing comment at plane one, row one, column one
            } // trailing comment after the final tensor row
        }  // trailing comment after the final tensor plane
    }; // trailing comment after a commented 3D tensor
    using ArrayTensor3D = std::array<std::array<std::array<int, 2>, 2>, 2>;
    ArrayTensor3D standard_tensor =
    {
        {
            {
                {
                    {
                        1, 2
                    }, {
                        3, 4
                    }
                }
            }, {
                {
                    {
                        5, 6
                    }, {
                        7, 8
                    }
                }
            }
        }
    };
    int built_in_tensor[2][2][3] =
    {
        {
            {
                1, 2, 3
            }, {
                4, 5, 6
            }
        },
        {
            {
                7, 8, 9
            }, {
                10, 11, 12
            }
        }
    };
    DeepAggregateVolume volume =
    {
        {
            {
                {
                    {
                        {
                            1, {
                                255, 0, 0
                            }
                        }, {
                            2, {
                                0, 255, 0
                            }
                        }
                    },
                    {
                        {
                            3, {
                                0, 0, 255
                            }
                        }, {
                            4, {
                                255, 255, 0
                            }
                        }
                    }
                },
                true
            },
            {
                {
                    {
                        {
                            5, {
                                255, 0, 255
                            }
                        }, {
                            6, {
                                0, 255, 255
                            }
                        }
                    },
                    {
                        {
                            7, {
                                255, 255, 255
                            }
                        }, {
                            8, {
                                0, 0, 0
                            }
                        }
                    }
                },
                false
            }
        },
        {2, 2, 2}
    };
    DeepAggregateVolume partial_volume =
    {
        {
            {
                {
                    {
                        {
                            9, {
                                1, 2, 3
                            }
                        }
                    }
                },
                true
            }
        },
        {1, 1, 1}
    };
    DeepAggregateVolume empty_volume {};
    (void) deduced_matrix;
    (void) deduced_tensor;
    (void) commented_tensor;
    (void) standard_tensor;
    (void) built_in_tensor;
    (void) volume;
    (void) partial_volume;
    (void) empty_volume;
}

void
ExerciseMixedInitializerExpressions()
{
    int base_value          = 3;
    int offset              = 4;
    bool choose_left        = true;
    std::string source_text = "source";
    std::array matrix
    {
        std::array {1, 2},
        std::array {3, 4},
    };
    MixedInitializerAggregate designated_mixed =
    {
        .string_constructor = std::string(
            3,
            'x'
            ),
        .from_invoked_lambda = [base_value] { return base_value * 2; } (),
        .from_generic_lambda = [] (auto value) { return value + 1; } (offset),
        .enum_expression     = Permission::Read | Permission::Write,
        .address_expression  = &base_value,
        .reference_call      = std::cref(base_value),
        .layers              =
        {
            {
                .from_conditional   = choose_left ? base_value : offset,
                .from_static_call   = static_cast<int>(std::char_traits<char>::length("static")),
                .from_template_call = std::max<int>(
                    base_value,
                    offset
                    ),
                .from_callable_object = std::plus<>{}(
                    base_value,
                    offset
                    ),
                .from_invoke_call = std::invoke(
                    std::plus<>{},
                    base_value,
                    offset
                    ),
                .from_cast           = static_cast<int>(source_text.substr(1).size()),
                .variant_constructor = std::variant<int, std::string>{std::in_place_type<std::string>, "variant"},
                .ctad_constructor    = std::tuple {base_value, source_text},
                .function_pointer    = +[] (int value) { return value - 1; },
                .branches            =
                {
                    {
                        .from_arithmetic    = base_value + offset * 2,
                        .from_standard_call = std::abs(-base_value),
                        .from_member_call   = static_cast<int>(source_text.size()),
                        .from_nested_call   = std::clamp(
                            std::abs(-offset),
                            0,
                            10
                            ),
                        .parenthesized_constructor = Box<int>(offset),
                        .optional_constructor      = std::optional<int>{std::in_place, base_value + offset},
                        .factory_call              = std::make_unique<int>(base_value * offset),
                        .leaves                    =
                        {
                            {
                                .from_variable      = base_value,
                                .from_function_call = Add(
                                    base_value,
                                    offset
                                    ),
                                .from_subscript     = matrix[1][0],
                                .braced_constructor = Box<int>{base_value},
                                .stored_lambda      = [base_value] (int value) { return base_value + value; },
                                .nested_expressions =
                                {
                                    base_value,
                                    std::abs(-offset),
                                    matrix[0][1],
                                    [offset] { return offset * 2; } ()
                                }
                            }
                        },
                        .size_expression = sizeof(decltype(matrix))
                    }
                },
                .noexcept_expression = noexcept(std::abs(base_value))
            }
        },
        .alignment_expression = alignof(MixedInitializerAggregate::Leaf)
    };
    // Positional aggregate initialization follows member declaration order at every nested level.
    MixedInitializerAggregate positional_mixed =
    {
        std::string(
            4,
            'p'
            ),
        [offset] { return offset * 3; } (),
        [] (auto value) { return value - 1; } (base_value),
        Permission::Read | Permission::Write,
        &offset,
        std::cref(offset),
        {
            {
                choose_left ? offset : base_value,
                static_cast<int>(std::char_traits<char>::length("position")),
                std::min<int>(
                    base_value,
                    offset
                    ),
                std::multiplies<>{}(
                    base_value,
                    offset
                    ),
                std::invoke(
                    std::minus<>{},
                    offset,
                    base_value
                    ),
                static_cast<int>(source_text.find('u')),
                std::variant<int, std::string>{std::in_place_index<0>, offset},
                std::tuple
                {
                    offset,
                    source_text.substr(
                        0,
                        3
                        )
                },
                +[] (int value) { return value + 1; },
                {
                    {
                        (base_value + offset) * 2,
                        std::abs(base_value - offset),
                        static_cast<int>(source_text.substr(1).size()),
                        std::clamp(
                            Add(
                                base_value,
                                offset
                                ),
                            0,
                            10
                            ),
                        Box<int>(base_value + offset),
                        std::optional<int>{std::in_place, offset},
                        std::make_unique<int>(offset),
                        {
                            {
                                offset,
                                Add(
                                    offset,
                                    base_value
                                    ),
                                matrix[0][1],
                                Box<int>{offset},
                                [offset] (int value) { return offset + value; },
                                {
                                    offset,
                                    std::abs(-base_value),
                                    matrix[1][1],
                                    [base_value] { return base_value * 3; } ()
                                }
                            }
                        },
                        sizeof(matrix[0])
                    }
                },
                noexcept(std::min(
                    base_value,
                    offset
                    ))
            }
        },
        alignof(MixedInitializerAggregate::Branch)
    };
    (void) designated_mixed;
    (void) positional_mixed;
}

} // namespace coverage

} // namespace zcf

// ============================================================================
// COMPLEX FUNCTION-DEFINITION COLLISION SUITE
// Format style: deeply qualified names, stacked template heads, leading and
// trailing return types, conditional noexcept, ref qualifiers, trailing
// requires clauses, conversion/call/subscript operators, constructor
// function-try-blocks, attributes, and pointer-to-member parameters.
// Language feature: constrained member templates, variadic packs, explicit
// specialization, nested target types, multidimensional array-reference
// returns, nested class constructors/destructors, and template-template
// parameters. Each definition combines grammar forms covered separately above.
// ============================================================================

namespace zcf::coverage::complex_function_definitions
{

template <typename Error, template < typename ...> class Container>
class Workflow
{
public:

    template <typename Record, std::size_t Extent>
    class Stage
    {
    public:

        template <
            std::input_iterator Iterator,
            std::sentinel_for<Iterator> Sentinel,
            typename Projector>
        [[nodiscard]] constexpr auto
        Collect(
            Iterator first,
            Sentinel last,
            Projector&& projector
            ) const& noexcept(noexcept(std::invoke(
            projector,
            *first
            )))->std::variant<Container<std::invoke_result_t<Projector&, std::iter_reference_t<Iterator>>>, Error>
        requires std::invocable<Projector&, std::iter_reference_t<Iterator>>;
    };
};

template <typename State>
class DispatchTable
{
public:

    template <typename ... Handlers>
    class OverloadSet
    {
    public:

        template <typename Event, typename ... Context>
        [[nodiscard]] constexpr decltype(auto)
        operator()(
            Event && event,
            Context && ... context
            ) && noexcept(std::is_nothrow_move_constructible_v<std::remove_reference_t<Event>>)
        requires(std::invocable<Handlers&, Event, Context...>&& ...);
    };
};

template <typename Source>
class ConversionView
{
public:

    template <typename Target>
    explicit
    operator std::optional<std::reference_wrapper<const Target>>() const noexcept
    requires std::same_as<Source, Target>;
};

template <typename Allocator, typename Logger>
class ResourcePool
{
public:

    template <typename Handle>
    class Lease
    {
    public:

        explicit
        Lease(
            Handle handle,
            Logger& logger
            );

        ~Lease() noexcept(noexcept(std::declval<Logger&>().release(std::declval<const Handle&>())));

    private:

        Handle handle_;
        Logger* logger_;
    };
};

template <typename Value>
struct Decoder
{
    template <typename Input>
    [[nodiscard]] static auto
    Decode(
        Input&& input
        ) -> std::optional<Value>;
};

template <std::size_t Depth, std::size_t Rows, std::size_t Columns>
class TensorStorage
{
public:

    [[nodiscard]] auto
    operator[](
        std::size_t plane_index
        ) & noexcept->int (&)[Rows][Columns];

private:

    int values_[Depth][Rows][Columns] {};
};

template <typename Result, typename Receiver, typename ... Args>
[[nodiscard("member invocation results must be observed")]] constexpr auto
    InvokeMemberAcross(
    std::span<Receiver*> receivers,
    Result (Receiver::*operation)(Args...) const noexcept,
    std::tuple<Args...> arguments
    ) noexcept(std::is_nothrow_default_constructible_v<Result>)->std::vector<Result>
requires(
    std::is_object_v<Receiver>&& (std::copy_constructible<Args>&& ...)
    );

} // namespace zcf::coverage::complex_function_definitions

template <typename Error, template < typename ...> class Container>
template <typename Record, std::size_t Extent>
template <
    std::input_iterator Iterator,
    std::sentinel_for<Iterator> Sentinel,
    typename Projector>
[[nodiscard]] constexpr auto
zcf::                      // Project namespace.
coverage::                     // Coverage namespace.
complex_function_definitions:: // Stress namespace.
Workflow<Error, Container>::   // Outer class template.
Stage<Record, Extent>::        // Nested class template.
Collect(
    Iterator first,
    Sentinel last,
    Projector&& projector
    ) const& noexcept(noexcept(std::invoke(
    projector,
    *first
    )))
->std::variant<
    Container<
        std::invoke_result_t<
            Projector&,
            std::iter_reference_t<Iterator>>>,
    Error>
requires std::invocable<Projector&, std::iter_reference_t<Iterator>>
{
    (void) last;
    (void) projector;

    return Container<std::invoke_result_t<Projector&, std::iter_reference_t<Iterator>>>{};
}

template <typename State>
template <typename ... Handlers>
template <typename Event, typename ... Context>
[[nodiscard]] constexpr decltype(auto)
zcf::                      // Project namespace.
coverage::                     // Coverage namespace.
complex_function_definitions:: // Stress namespace.
DispatchTable<State>::         // Outer class template.
OverloadSet<Handlers...>::     // Nested variadic class template.
operator()(
    Event&& event,
    Context&&... context
    ) && noexcept(std::is_nothrow_move_constructible_v<std::remove_reference_t<Event>>)
requires(std::invocable<Handlers&, Event, Context...>&& ...)
{
    (void) sizeof...(context);

    return std::forward<Event>(event);
}

template <typename Source>
template <typename Target>
zcf::                      // Project namespace.
coverage::                     // Coverage namespace.
complex_function_definitions:: // Stress namespace.
ConversionView<Source>::       // Constrained conversion class.
operator std::optional<
    std::reference_wrapper<
        const Target>>() const noexcept
requires std::same_as<Source, Target>
{
    return std::nullopt;
}

template <typename Allocator, typename Logger>
template <typename Handle>
zcf::                         // Project namespace.
coverage::                        // Coverage namespace.
complex_function_definitions::    // Stress namespace.
ResourcePool<Allocator, Logger>:: // Outer resource class.
Lease<Handle>::                   // Nested lease class.

Lease(
    Handle handle,
    Logger& logger
    )
try:
    handle_(std::move(handle)),
    logger_(&logger)
{
}

catch (...)
{
    throw;
}

template <typename Allocator, typename Logger>
template <typename Handle>
zcf::                         // Project namespace.
coverage::                        // Coverage namespace.
complex_function_definitions::    // Stress namespace.
ResourcePool<Allocator, Logger>:: // Outer resource class.
Lease<Handle>::                   // Nested lease class.

~Lease() noexcept(noexcept(std::declval<Logger&>().release(std::declval<const Handle&>())))
{
}

template <>
template <>
[[nodiscard]] auto
zcf::                      // Project namespace.
coverage::                     // Coverage namespace.
complex_function_definitions:: // Stress namespace.
Decoder<int>::                 // Fully specialized decoder.
Decode<std::string_view>(
    std::string_view&& input
    ) -> std::optional<int>
{
    return static_cast<int>(input.size());
}

template <std::size_t Depth, std::size_t Rows, std::size_t Columns>
auto
zcf::                             // Project namespace.
coverage::                            // Coverage namespace.
complex_function_definitions::        // Stress namespace.
TensorStorage<Depth, Rows, Columns>:: // Multidimensional storage.
operator[](
    std::size_t plane_index
    ) & noexcept->int (&)[Rows][Columns]
{
    return values_[plane_index];
}

template <typename Result, typename Receiver, typename ... Args>
[[nodiscard("member invocation results must be observed")]] constexpr auto
zcf::                      // Project namespace.
coverage::                     // Coverage namespace.
complex_function_definitions:: // Stress namespace.
InvokeMemberAcross(
    std::span<Receiver*> receivers,
    Result (Receiver::*operation)(Args...) const noexcept,
    std::tuple<Args...> arguments
    ) noexcept(std::is_nothrow_default_constructible_v<Result>) -> std::vector<Result>
requires(
    std::is_object_v<Receiver> && (std::copy_constructible<Args>&& ...)
    )
{
    (void) receivers;
    (void) operation;
    (void) arguments;

    return {};
}

// ============================================================================
// DECLTYPE AND MODERN FUNCTION-DEFINITION COLLISION SUITE
// Format style: leading decltype(auto), nested trailing decltype expressions,
// explicit object parameters, conditional noexcept, requires-expressions,
// static call/subscript operators, and C++26 pack-indexing token boundaries.
// Language feature: C++23 deducing this, static operator(), multidimensional
// static operator[], if consteval, and feature-gated C++26 pack indexing,
// delete-with-reason, variadic friends, and placeholder variables.
//
// The pinned Clang 20.1.8 frontend implements the guarded C++26 forms below.
// Contracts, reflection, and expansion statements remain outside this
// compile-validated fixture because that frontend does not implement them.
// ============================================================================

namespace zcf::coverage::modern_function_definitions
{

template <typename Value, std::size_t Extent>
class ModernAccessor
{
public:

    template <typename Self, std::size_t Index>
    [[nodiscard]] constexpr decltype(auto)
    Access(
        this Self && self,
        std::integral_constant<std::size_t, Index> index
        ) noexcept(noexcept(std::forward<Self>(self).values_[index]))
    requires(
        Index < Extent &&
        std::same_as<
            decltype((std::forward<Self>(self).values_[Index])),
            decltype((std::forward<Self>(self).values_[index]))>
        );

    template <typename Self, typename Projection>
    [[nodiscard]] constexpr auto
        Project(
        this Self && self,
        Projection && projection
        ) noexcept(noexcept(std::invoke(
        std::forward<Projection>(projection),
        std::forward<Self>(self).values_[0]
        )))->decltype(std::invoke(
        std::forward<Projection>(projection),
        std::forward<Self>(self).values_[0]
        ))
    requires(
        Extent > 0 &&
        requires
        {
            typename std::type_identity<
                decltype(std::invoke(
                    std::forward<Projection>(projection),
                    std::forward<Self>(self).values_[0]
                    ))>::type;
        }
        );

private:

    std::array<Value, Extent> values_ {};
};

template <typename Value, std::size_t Extent>
template <typename Self, std::size_t Index>
[[nodiscard]] constexpr decltype(auto)
ModernAccessor<Value, Extent>::Access(
    this Self && self,
    std::integral_constant<std::size_t, Index> index
    ) noexcept(noexcept(std::forward<Self>(self).values_[index]))
requires(
    Index < Extent &&
    std::same_as<
        decltype((std::forward<Self>(self).values_[Index])),
        decltype((std::forward<Self>(self).values_[index]))>
    )
{
    // *INDENT-OFF*
    if consteval
    {
        return (std::forward<Self>(self).values_[Index]);
    }
    else
    {
        return (std::forward<Self>(self).values_[index]);
    }
    // *INDENT-ON*
}

template <typename Value, std::size_t Extent>
template <typename Self, typename Projection>
[[nodiscard]] constexpr auto
ModernAccessor<Value, Extent>::Project(
    this Self&& self,
    Projection&& projection
    ) noexcept(noexcept(std::invoke(
    std::forward<Projection>(projection),
    std::forward<Self>(self).values_[0]
    )))
-> decltype(std::invoke(
    std::forward<Projection>(projection),
    std::forward<Self>(self).values_[0]
    ))
requires(
    Extent > 0 &&
    requires
    {
        typename std::type_identity<
            decltype(std::invoke(
                std::forward<Projection>(projection),
                std::forward<Self>(self).values_[0]
                ))>::type;
    }

    )
{
    // *INDENT-OFF*
    if consteval
    {
        return std::invoke(
            std::forward<Projection>(projection),
            std::forward<Self>(self).values_[0]
        );
    }
    else
    {
        return std::invoke(
            std::forward<Projection>(projection),
            std::forward<Self>(self).values_[Extent - 1]
        );
    }
    // *INDENT-ON*
}

struct ModernStaticOperators
{
    template <typename Left, typename Right>
    [[nodiscard]] static constexpr auto
    operator()(
        Left&& left,
        Right&& right
        ) noexcept(noexcept(std::forward<Left>(left) + std::forward<Right>(right)))
    -> decltype(std::forward<Left>(left) + std::forward<Right>(right))
    requires(
        requires
        {
            std::forward<Left>(left) + std::forward<Right>(right);
        }
        );

    template <std::size_t Rows, std::size_t Columns>
    [[nodiscard]] static constexpr decltype(auto)
    operator[](
        int (&matrix)[Rows][Columns],
        std::size_t row,
        std::size_t column
        ) noexcept(noexcept(matrix[row][column]));
};

template <typename Left, typename Right>
[[nodiscard]] constexpr auto
ModernStaticOperators::operator()(
    Left&& left,
    Right&& right
    ) noexcept(noexcept(std::forward<Left>(left) + std::forward<Right>(right)))
-> decltype(std::forward<Left>(left) + std::forward<Right>(right))
requires(
    requires
    {
        std::forward<Left>(left) + std::forward<Right>(right);
    }

    )
{
    return std::forward<Left>(left) + std::forward<Right>(right);
}

template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] constexpr decltype(auto)
ModernStaticOperators::operator[](
    int (&matrix)[Rows][Columns],
    std::size_t row,
    std::size_t column
    ) noexcept(noexcept(matrix[row][column]))
{
    return (matrix[row][column]);
}

#if defined(__cpp_pack_indexing) && __cpp_pack_indexing >= 202311L
template <std::size_t Index, typename ... Args>
[[nodiscard]] constexpr decltype(auto)
SelectPackArgument(
    Args && ... args
    ) noexcept(noexcept(std::forward<Args...[Index]>(args ...[Index])))
requires(
    Index < sizeof...(Args) &&
    std::same_as<
        decltype(std::forward<Args...[Index]>(args ...[Index])),
        Args...[Index] &&>
    )
{
    return std::forward<Args...[Index]>(args ...[Index]);
}

template <typename ... Values>
struct PackIndexedFactory
{
    template <std::size_t Index>
    [[nodiscard]] static constexpr auto
    Create() noexcept(std::is_nothrow_default_constructible_v<Values...[Index]>)
    -> Values...[Index]
    requires(Index < sizeof...(Values));
};

template <typename ... Values>
template <std::size_t Index>
[[nodiscard]] constexpr auto
PackIndexedFactory<Values...>::Create() noexcept(
    std::is_nothrow_default_constructible_v<Values...[Index]>
    )
-> Values...[Index]
requires(Index < sizeof...(Values))
{
    return Values ...[Index] {};
}

using PackIndexedType = decltype(SelectPackArgument<1>(
    std::declval<int&>(),
    std::declval<const long&>(),
    std::declval<double &&>()
    ));
static_assert(std::same_as<PackIndexedType, const long&>);
#endif

#if defined(__cpp_deleted_function) && __cpp_deleted_function >= 202403L
template <typename Value>
struct ReasonedConversion
{
    template <typename Other>
    [[nodiscard]] auto
    Convert(
        Other&&
        ) const -> Value = delete ("conversion must preserve the registered value category");
};
#endif

#if defined(__cpp_variadic_friend) && __cpp_variadic_friend >= 202403L
template <typename ... TrustedTypes>
class VariadicFriendVault
{
    friend TrustedTypes ...;

    [[nodiscard]] static constexpr int
    Read() noexcept
    {
        return 42;
    }
};
#endif

#if defined(__cpp_placeholder_variables) && __cpp_placeholder_variables >= 202306L
constexpr int
ExercisePlaceholderVariables()
{
    int _ = 1;
    int _ = 2;
    auto [_, value] = std::array {3, 4};

    return value;
}
#endif

} // namespace zcf::coverage::modern_function_definitions

// Format style: final end-of-file line comment coverage.