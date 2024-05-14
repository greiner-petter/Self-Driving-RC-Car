#pragma once

#ifndef OC_ASSERT_DISABLED
#define OC_ASSERT_DISABLED 0
#endif

#ifndef OC_ASSERT_PRINT_IN_COLOR
#define OC_ASSERT_PRINT_IN_COLOR 1
#endif

inline bool oc_assert_exit      = true;
inline bool oc_assert_backtrace = true;
inline bool oc_assert_dump_core = true;

#include <iostream> // std::cerr
#include <type_traits> // std::is_...
#include <typeinfo> // std::type_info (returned by typeid)
#include <string>
#include <string_view>
#include <cxxabi.h>
#include <cstdlib> // abort, exit
#include <cstdint>

#if OC_ASSERT_DISABLED

#define oc_assert(expr, ...) do { (void)(bool)!(expr); } while (0)

#else // #if OC_ASSERT_DISABLED

#define oc_assert(expr, ...) do { \
    if (!(expr)) /*[[unlikely]]*/ { \
        ocAssert::print_assert_expression(__STRING(expr)); \
        ocAssert::print_additional_expressions(#__VA_ARGS__ __VA_OPT__(,) __VA_ARGS__); \
        ocAssert::print_assert_location(__FILE__, __LINE__, __PRETTY_FUNCTION__); \
        if (oc_assert_exit) { \
            if (oc_assert_dump_core) { \
                abort(); \
            } else { \
                exit(-1); \
            } \
        } \
    } \
} while (0)

namespace ocAssert
{
    template <typename T>
    std::string get_type_name()
    {
        if constexpr (std::is_same_v<std::string, T>)
        {
            return "std::string";
        }
        if constexpr (std::is_same_v<std::string_view, T>)
        {
            return "std::string_view";
        }
        else if constexpr (std::is_const<T>::value)
        {
            return "const " + get_type_name<std::remove_const_t<T>>();
        }
        else if constexpr (std::is_volatile<T>::value)
        {
            return "volatile " + get_type_name<std::remove_volatile_t<T>>();
        }
        else if constexpr (std::is_pointer<T>::value)
        {
            return get_type_name<std::remove_pointer_t<T>>() + '*';
        }
        else if constexpr (std::is_reference<T>::value)
        {
            return get_type_name<std::remove_reference_t<T>>() + '&';
        }
        else if constexpr (std::is_array<T>::value)
        {
            return get_type_name<std::remove_extent_t<T>>() + "[" + std::to_string(std::extent_v<T>) + "]";
        }
        else
        {
            char *name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
            if (name)
            {
                std::string result = name;
                free(name);
                return result;
            }
            return typeid(T).name();
        }
    }

    template<typename T> requires requires (std::ostream &os, T t) { os << t; }
    void print(std::ostream &os, const T &val) { os << val; }

    template<typename T> requires requires (T t) { to_string(t); }
    void print(std::ostream &os, const T &val)
    {
        os << to_string(val);
        if constexpr (std::is_enum_v<T>)
        {
            os << " (" << (uint64_t)val << ")";
        }
    }

    void print(std::ostream &os, char val);
    void print(std::ostream &os, signed char val);
    void print(std::ostream &os, unsigned char val);
    void print(std::ostream &os, bool val);
    void print(std::ostream &os, const char *val);
    void print(std::ostream &os, char *val);
    void print(std::ostream &os, std::string val);
    void print(std::ostream &os, std::string_view val);
    void print(std::ostream &os, decltype(nullptr));
    void print(std::ostream &os, float);
    void print(std::ostream &os, double);

    template<typename T>
    void print(std::ostream &os, const T&) { os << "{?}"; }

    void print_assert_expression(
        std::string_view expression_string);
    void print_assert_location(
        std::string_view file,
        int              line,
        std::string_view function);

    // Special case for the template below, so that no compiler complains about
    // unused parameters, local variables, etc.
    inline void print_additional_expressions(std::string_view) {}

    void print_additional_expressions(
        std::string_view names,
        const auto&... values)
    {
        size_t index = 0;
        auto printer = [&]<typename T>(const T &value) {
            size_t length = 0;
            // Technically, this can break, if a string containing a comma is
            // passed as an expression, but dealing with that is not worth the
            // effort.
            while (names[index + length] && names[index + length] != ',') length++;

            std::cerr << "(" << get_type_name<T>() << ") ";

            // Here we try to avoid printing names for literals, because they are
            // the same as the value.
            if (char c = names[index]; (c < '0' || '9' < c) && '"' != c && '\'' != c)
            {
                std::cerr << names.substr(index, length) << " = ";
            }
            print(std::cerr, value);
            std::cerr << '\n';
            // we advance 2 indices here, because the compiler inserts ", " as
            // separators. This might depend on the compiler, so if that
            // becomes a problem, move the 2 into its own platform-dependent
            // macro.
            index += length + 2;
        };

        (printer(values), ...);
    }
}

#endif // #if OC_ASSERT_DISABLED
