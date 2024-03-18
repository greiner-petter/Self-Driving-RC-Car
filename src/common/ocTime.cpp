#include "ocTime.h"

#include <cmath>
#include <ctime>
#include <limits>

ocTime::ocTime(int64_t time_ns)
{
  _time_ns = time_ns;
}

ocTime ocTime::now()
{
  timespec ts = {};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  int64_t time_ns = 0;
  time_ns += ts.tv_nsec;
  time_ns += ts.tv_sec * 1000000000L;
  ocTime result = ocTime(time_ns);
  return result;
}

ocTime ocTime::null()
{
  return ocTime(0);
}

ocTime ocTime::forever()
{
  return ocTime(std::numeric_limits<int64_t>::max());
}

ocTime ocTime::days(int64_t d)
{
  return ocTime(d * 24L * 60L * 60L * 1000000000L);
}

ocTime ocTime::hours(int64_t h)
{
  return ocTime(h * 60L * 60L * 1000000000L);
}

ocTime ocTime::minutes(int64_t m)
{
  return ocTime(m * 60L * 1000000000L);
}

ocTime ocTime::seconds(int64_t s)
{
  return ocTime(s * 1000000000L);
}

ocTime ocTime::milliseconds(int64_t ms)
{
  return ocTime(ms * 1000000L);
}

ocTime ocTime::microseconds(int64_t us)
{
  return ocTime(us * 1000L);
}

ocTime ocTime::nanoseconds(int64_t ns)
{
  return ocTime(ns);
}

ocTime ocTime::days_float(float d)
{
  return ocTime((int64_t)((double)d * 24.0 * 60.0 * 60.0 * 1000000000.0));
}

ocTime ocTime::hours_float(float h)
{
  return ocTime((int64_t)((double)h * 60.0 * 60.0 * 1000000000.0));
}

ocTime ocTime::minutes_float(float m)
{
  return ocTime((int64_t)((double)m * 60.0 * 1000000000.0));
}

ocTime ocTime::seconds_float(float s)
{
  return ocTime((int64_t)((double)s * 1000000000.0));
}

ocTime ocTime::milliseconds_float(float ms)
{
  return ocTime((int64_t)((double)ms * 1000000.0));
}

ocTime ocTime::microseconds_float(float us)
{
  return ocTime((int64_t)((double)us * 1000.0));
}

ocTime ocTime::nanoseconds_float(float ns)
{
  return ocTime((int64_t)(ns));
}

ocTime ocTime::hertz(float hz)
{
  return ocTime((int64_t)std::round((double)1000000000.0 / (double)hz));
}

int64_t ocTime::get_days() const
{
  return _time_ns / 24L / 60L / 60L / 1000000000L;
}

int64_t ocTime::get_hours() const
{
  return _time_ns / 60L / 60L / 1000000000L;
}

int64_t ocTime::get_minutes() const
{
  return _time_ns / 60L / 1000000000L;
}

int64_t ocTime::get_seconds() const
{
  return _time_ns / 1000000000L;
}

int64_t ocTime::get_milliseconds() const
{
  return _time_ns / 1000000L;
}

int64_t ocTime::get_microseconds() const
{
  return _time_ns / 1000L;
}

int64_t ocTime::get_nanoseconds() const
{
  return _time_ns;
}

float ocTime::get_float_days() const
{
  return (float)((double)_time_ns / 24.0 / 60.0 / 60.0 / 1000000000.0);
}

float ocTime::get_float_hours() const
{
  return (float)((double)_time_ns / 60.0 / 60.0 / 1000000000.0);
}

float ocTime::get_float_minutes() const
{
  return (float)((double)_time_ns / 60.0 / 1000000000.0);
}

float ocTime::get_float_seconds() const
{
  return (float)((double)_time_ns / 1000000000.0);
}

float ocTime::get_float_milliseconds() const
{
  return (float)((double)_time_ns / 1000000.0);
}

float ocTime::get_float_microseconds() const
{
  return (float)((double)_time_ns / 1000.0);
}

float ocTime::get_float_nanoseconds() const
{
  return (float)_time_ns;
}

float ocTime::get_hertz() const
{
  return (float)(1000000000.0 / (double)_time_ns);
}

ocTime ocTime::operator+(const ocTime t) const
{
  return ocTime(_time_ns + t._time_ns);
}
void ocTime::operator+=(const ocTime t)
{
  _time_ns += t._time_ns;
}

ocTime ocTime::operator-(const ocTime t) const
{
  return ocTime(_time_ns - t._time_ns);
}
void ocTime::operator-=(const ocTime t)
{
  _time_ns -= t._time_ns;
}

ocTime ocTime::operator*(float f) const
{
  return ocTime((int64_t)((double)_time_ns * (double)f));
}
void ocTime::operator*=(float f)
{
  _time_ns = (int64_t)((double)_time_ns * (double)f);
}

ocTime ocTime::operator/(float f) const
{
  return ocTime((int64_t)((double)_time_ns / (double)f));
}
float ocTime::operator/(ocTime t) const
{
  return (float)((double)_time_ns / (double)t._time_ns);
}
void ocTime::operator/=(float f)
{
  _time_ns = (int64_t)((double)_time_ns / (double)f);
}

ocTime ocTime::operator%(ocTime t) const
{
  return ocTime(_time_ns % t._time_ns);
}
void ocTime::operator%=(ocTime t)
{
  _time_ns %= t._time_ns;
}

bool ocTime::operator<(ocTime t) const
{
  return _time_ns < t._time_ns;
}
bool ocTime::operator<=(ocTime t) const
{
  return _time_ns <= t._time_ns;
}
bool ocTime::operator>(ocTime t) const
{
  return _time_ns > t._time_ns;
}
bool ocTime::operator>=(ocTime t) const
{
  return _time_ns >= t._time_ns;
}
bool ocTime::operator==(ocTime t) const
{
  return _time_ns == t._time_ns;
}
bool ocTime::operator!=(ocTime t) const
{
  return _time_ns != t._time_ns;
}

std::ostream &operator<<(std::ostream &os, const ocTime &t)
{
  return os << "{ _time_ns : " << t._time_ns << " }";
}
