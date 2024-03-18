#pragma once

#include <cstdint>

#include <iostream>

class ocTime final
{
private:

  int64_t _time_ns;

  ocTime(int64_t time_ns);

public:

  static ocTime now();

  static ocTime null();
  static ocTime forever();

  static ocTime days(int64_t d);
  static ocTime hours(int64_t h);
  static ocTime minutes(int64_t m);
  static ocTime seconds(int64_t s);
  static ocTime milliseconds(int64_t ms);
  static ocTime microseconds(int64_t us);
  static ocTime nanoseconds(int64_t ns);

  static ocTime days_float(float d);
  static ocTime hours_float(float h);
  static ocTime minutes_float(float m);
  static ocTime seconds_float(float s);
  static ocTime milliseconds_float(float ms);
  static ocTime microseconds_float(float us);
  static ocTime nanoseconds_float(float ns);
  static ocTime hertz(float hz);

  ocTime() = default;

  int64_t get_days() const;
  int64_t get_hours() const;
  int64_t get_minutes() const;
  int64_t get_seconds() const;
  int64_t get_milliseconds() const;
  int64_t get_microseconds() const;
  int64_t get_nanoseconds() const;

  float get_float_days() const;
  float get_float_hours() const;
  float get_float_minutes() const;
  float get_float_seconds() const;
  float get_float_milliseconds() const;
  float get_float_microseconds() const;
  float get_float_nanoseconds() const;
  float get_hertz() const;

  ocTime operator+(ocTime t) const;
  void operator+=(ocTime t);
  ocTime operator-(ocTime t) const;
  void operator-=(ocTime t);
  ocTime operator*(float f) const;
  void operator*=(float f);
  ocTime operator/(float f) const;
  float operator/(ocTime t) const;
  void operator/=(float f);
  ocTime operator%(ocTime t) const;
  void operator%=(ocTime t);

  bool operator<(ocTime t) const;
  bool operator<=(ocTime t) const;
  bool operator>(ocTime t) const;
  bool operator>=(ocTime t) const;
  bool operator==(ocTime t) const;
  bool operator!=(ocTime t) const;

  friend std::ostream &operator<<(std::ostream &os, const ocTime &t);
};
