#include "ocAssert.h"

#if OC_ASSERT_DISABLED
#else

#include <execinfo.h> // backtrace
#include <dlfcn.h> // dladdr
#include <iomanip> // floating point precision output
#include <unistd.h> // readlink

static std::string get_executable_name()
{
    char path[128] = {};
    if (0 < readlink("/proc/self/exe", path, sizeof(path) - 1))
        return path;
    return "<error>";
}

// ----------------------------------------------------------------------------
// Overloads for different types that should be printed specially by assert

void ocAssert::print(std::ostream &os, char val)
{
    char print_char = val;
    if (val < 32 || 126 < val) print_char = ' ';
    os << '\'' << print_char << "\' (" << (int) val << ')';
}
void ocAssert::print(std::ostream &os, signed char val)
{
    signed char print_char = val;
    if (val < 32 || 126 < val) print_char = ' ';
    os << '\'' << print_char << "\' (" << (int) val << ')';
}
void ocAssert::print(std::ostream &os, unsigned char val)
{
    unsigned char print_char = val;
    if (val < 32 || 126 < val) print_char = ' ';
    os << '\'' << print_char << "\' (" << (int) val << ')';
}
void ocAssert::print(std::ostream &os, bool val)
{
    os << (val ? "true" : "false");
}
void ocAssert::print(std::ostream &os, const char *val)
{
    os << '\"' << val << '\"';
}
void ocAssert::print(std::ostream &os, char *val)
{
    os << '\"' << val << '\"';
}
void ocAssert::print(std::ostream &os, std::string val)
{
    os << '\"' << val << '\"';
}
void ocAssert::print(std::ostream &os, std::string_view val)
{
    os << '\"' << val << '\"';
}
void ocAssert::print(std::ostream &os, decltype(nullptr))
{
    os << "null";
}
void ocAssert::print(std::ostream &os, float f)
{
    os << std::fixed << std::setprecision(10) << f;
}
void ocAssert::print(std::ostream &os, double d)
{
    os << std::fixed << std::setprecision(20) << d;
}

// ----------------------------------------------------------------------------

void ocAssert::print_assert_expression(
    std::string_view expression_string)
{
#if OC_ASSERT_PRINT_IN_COLOR
    std::cerr << "\033[1;31m"; // set print color to bold/bright red
#endif // #if OC_ASSERT_PRINT_IN_COLOR
    std::cerr << "Assertion (" << expression_string << ") failed.\n";
}

void ocAssert::print_assert_location(
    std::string_view file,
    int              line,
    std::string_view function)
{
    std::cerr << "location  : " << file << " (" << line << ")\n";
    std::cerr << "function  : " << function << '\n';
    std::cerr << "executable: " << get_executable_name() << '\n';

    if (oc_assert_exit && oc_assert_dump_core)
    {
        std::cerr << "core should get dumped, you might need \"ulimit -c unlimited\" though.\n";
    }

    if (oc_assert_backtrace)
    {
        std::cerr << "backtrace: " << '\n';
        const int buffer_length = 64;
        void *buffer[buffer_length];
        int trace_length = backtrace(buffer, buffer_length);
#if 0
        for (int i = 0; i < trace_length; ++i)
        {
            Dl_info info;
            if (dladdr(buffer[i], &info))
            {
                const char *function_name = info.dli_sname;
                char *name = abi::__cxa_demangle(function_name, nullptr, nullptr, nullptr);
                if (name) function_name = name;
                std::cerr << "  file: " << info.dli_fname << " func: " << function_name << '\n';
                if (name) free(name);
            }
            else
            {
                std::cerr << "  ???\n";
            }
        }
#else
        char **trace = backtrace_symbols(buffer, trace_length);
        if (trace)
        {
            for (int i = 0; i < trace_length; ++i)
            {
                std::cerr << trace[i] << '\n';
            }
        }
#endif
    }

#if OC_ASSERT_PRINT_IN_COLOR
    std::cerr << "\033[0m"; // set print color back to default
#endif // #if OC_ASSERT_PRINT_IN_COLOR

    std::cerr << std::flush; // make sure the message got printed before we kill the process
}

#endif // #if OC_ASSERT_DISABLED
