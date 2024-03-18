#pragma once

class ocLogger final
{
private:
    const char *_name;

    void _log_prolog(const char *color1, const char *color2) const;

public:
    ocLogger(const char *name);

    void log(const char *format, ...) const;

    void warn(const char *format, ...) const;

    void error(const char *format, ...) const;
};
