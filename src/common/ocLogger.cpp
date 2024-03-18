#include "ocLogger.h"

#include <cstdio> // printf, vprintf
#include <cstdarg> // va_list
#include <ctime> // tm, localtime(), strftime()
#include <sys/time.h> // gettimeofday(), timeval

void ocLogger::_log_prolog(const char *color1, const char *color2) const
{
    struct timeval now = {};
    char buf[9];
    gettimeofday(&now, nullptr);
    strftime(buf, 9, "%H:%M:%S", localtime(&now.tv_sec));
    printf("%s[%s.%03i]%s", color1, buf, (int)(now.tv_usec / 1000), color2);

    if (_name && *_name) printf(" %s", _name);
    printf(" : ");
}

ocLogger::ocLogger(const char *name)
{
  _name = name;
}

void ocLogger::log(const char *format, ...) const
{
    _log_prolog("\033[0;1m", "\033[0m"); // 0 = reset text attributes, 1 = bold
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    puts("\033[0m");
}

void ocLogger::warn(const char *format, ...) const
{
    _log_prolog("\033[0;33;2m", "\033[0;1;33m"); // 0 = reset text attributes, 33 = yellow, 2 = dim, 1 = bright/bold
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    puts("\033[0m");
}

void ocLogger::error(const char *format, ...) const
{
    _log_prolog("\033[0;31;2m", "\033[0;1;31m"); // 0 = reset text attributes, 31 = red, 2 = dim, 1 = bright/bold
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    puts("\033[0m");
}
