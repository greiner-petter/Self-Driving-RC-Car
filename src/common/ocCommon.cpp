#include "ocCommon.h"

#include "ocAssert.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cmath>

bool die()
{
  exit(-1);
}

bool parse_signed_int(std::string_view str, int64_t *target)
{
  if (str.empty()) return false;
  int base = 10;
  if (3 <= str.size() && '0' == str[0] && ('x' == str[1] || 'X' == str[1]))
  {
    base = 16;
    str = str.substr(2);
  }
  int64_t value = 0;
  auto result = std::from_chars(str.begin(), str.end(), value, base);
  if (std::errc() != result.ec || '\0' != *result.ptr) return false;
  *target = value;
  return true;
}

bool parse_unsigned_int(std::string_view str, uint64_t *target)
{
  if (str.empty()) return false;
  int base = 10;
  if (3 <= str.size() && '0' == str[0] && ('x' == str[1] || 'X' == str[1]))
  {
    base = 16;
    str = str.substr(2);
  }
  uint64_t value = 0;
  auto result = std::from_chars(str.begin(), str.end(), value, base);
  if (std::errc() != result.ec || '\0' != *result.ptr) return false;
  *target = value;
  return true;
}

bool parse_float32(std::string_view str, float *target)
{
  if (str.empty()) return false;
#if 0
  // TODO: change to this implementation once compilers support it.
  // As of 2020-08-19, GCC and Clang don't yet.
  float value = 0;
  auto result = std::from_chars(str.begin(), str.end(), value);
  if (std::errc() != result.ec || '\0' != *result.ptr) return false;
  *target = value;
  return true;
#else

  // To preserve precision as much as possible, we use an integer
  // to store the digits of the number. A float has ~24 bits of
  // precision, so using a 32-bit integer is good enough.
  uint32_t value = 0;
  int      scale = 0;
  float    sign  = 1.0f;
  bool point  = false;
  if ('-' == str[0])
  {
    sign = -1.0f;
    str = str.substr(1);
  }
  if (str.empty()) return false;

  // Loop over the significant digits and store them into the
  // intermediate integer, while keeping track of the scale
  // that needs to be applied afterwards.
  // If we run out of space in the integer, we stop caring about
  // the values of the digits, and just count them to get the
  // correct scale.
  for (char c : str)
  {
    if ('.' == c)
    {
      if (point) return false;
      point = true;
    }
    else if ('0' <= c && c <= '9')
    {
      if (value & 0xFF000000)
      {
        // we can stop this loop early, if the integer is full
        // and th exponent won't change anymore.
        if (point) break;
        scale += 1;
      }
      else
      {
        value *= 10;
        value += (uint32_t)(c - '0');
      }
      if (point) scale -= 1;
    }
    else
    {
      return false;
    }
  }
  *target = sign * (float)value * powf(10.0f, (float)scale);
  return true;
#endif
}

bool parse_float64(std::string_view str, double *target)
{
  if (str.empty()) return false;
#if 0
  // TODO: change to this implementation once compilers support it.
  // As of 2020-08-19, GCC and Clang don't yet.
  double value = 0;
  auto result = std::from_chars(str.begin(), str.end(), value);
  if (std::errc() != result.ec || '\0' != *result.ptr) return false;
  *target = value;
  return true;
#else
  // To preserve precision as much as possible, we use an integer
  // to store the digits of the number. A double has ~53 bits of
  // precision, so using a 64-bit integer is good enough.
  uint64_t value = 0;
  int      scale = 0;
  double   sign  = 1.0f;
  bool point  = false;
  if ('-' == str[0])
  {
    sign = -1.0f;
    str = str.substr(1);
  }
  if (str.empty()) return false;

  // Loop over the significant digits and store them into the
  // intermediate integer, while keeping track of the scale
  // that needs to be applied afterwards.
  // If we run out of space in the integer, we stop caring about
  // the values of the digits, and just count them to get the
  // correct scale.
  for (char c : str)
  {
    if ('.' == c)
    {
      if (point) return false;
      point = true;
    }
    else if ('0' <= c && c <= '9')
    {
      if (value & 0xFF000000)
      {
        // we can stop this loop early, if the integer is full
        // and the exponent won't change anymore.
        if (point) break;
        scale += 1;
      }
      else
      {
        value *= 10;
        value += (uint64_t)(c - '0');
      }
      if (point) scale -= 1;
    }
    else
    {
      return false;
    }
  }
  *target = sign * (double)value * pow(10.0, (double)scale);
  return true;
#endif
}

uint32_t byteswap(uint32_t value)
{
  return ((value >> 24) & 0x000000FF) |
         ((value >>  8) & 0x0000FF00) |
         ((value <<  8) & 0x00FF0000) |
         ((value << 24) & 0xFF000000);
}

bool are_close(float a, float b, int max_ulps)
{
  float mi = fminf(a, b);
  float ma = fmaxf(a, b);
  for (int i = 0; i < max_ulps; ++i) mi = std::nextafterf(mi, INFINITY);
  return ma <= mi;
}

bool are_close(double a, double b, int max_ulps)
{
  double mi = fmin(a, b);
  double ma = fmax(a, b);
  for (int i = 0; i < max_ulps; ++i) mi = std::nextafter(mi, INFINITY);
  return ma <= mi;
}

float normalize_radians(float radians)
{
  const float PI = 3.14159265358979f;
  if (std::isinf(radians) || std::isnan(radians)) return radians;
  while (radians < -PI) radians += 2 * PI;
  while (PI <= radians) radians -= 2 * PI;
  return radians;
}

float normalize_degrees(float degrees)
{
  if (std::isinf(degrees) || std::isnan(degrees)) return degrees;
  while (degrees < -180) degrees += 360;
  while (180 <= degrees) degrees -= 360;
  return degrees;
}

float sign(float f)
{
  return std::copysign(f, 1.0f);
}
float sign_or_zero(float f)
{
  if (f < 0.0f) return -1.0f;
  if (0.0f < f) return  1.0f;
  return 0.0f;
}

// minimal PCG32 Random Number Generator from pcg-random.org
// random state is a global here, which is fine as long as we don't use threads
struct
{
  uint64_t state = 0x853c49e6748fea9bULL;
} global_random_state;

uint32_t random_uint32()
{
  // minimal PCG32 Random Number Generator from pcg-random.org
  uint64_t oldstate = global_random_state.state;
  global_random_state.state = oldstate * 6364136223846793005ULL + 0xda3e39cb94b95bdbULL;
  uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
  uint32_t rot = (uint32_t)(oldstate >> 59u);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t random_uint32(uint32_t min, uint32_t max)
{
  oc_assert(min != max, min, max);
  uint32_t rand = random_uint32();
  uint32_t dist = max - min;
  return min + (rand % dist);
}

uint64_t random_uint64()
{
  return ((uint64_t)random_uint32() << 32) | random_uint32();
}

uint64_t random_uint64(uint64_t min, uint64_t max)
{
  oc_assert(min != max, min, max);
  uint64_t rand = random_uint64();
  uint64_t dist = max - min;
  return min + (rand % dist);
}

int32_t random_int32()
{
  union {uint32_t u; int32_t s;} i;
  i.u = random_uint32();
  return i.s;
}

int32_t random_int32(int32_t min, int32_t max)
{
  oc_assert(min != max, min, max);
  int32_t rand = random_int32();
  int32_t dist = max - min;
  return min + abs(rand % dist);
}

int64_t random_int64()
{
  union {uint64_t u; int64_t s;} i;
  i.u = random_uint64();
  return i.s;
}

int64_t random_int64(int64_t min, int64_t max)
{
  oc_assert(min != max, min, max);
  int64_t rand = random_int64();
  int64_t dist = max - min;
  return min + labs(rand % dist);
}

float random_float()
{
  // We take a random integer with at least 24 bits and map it to the floating point
  // range [0.0, 1.0). The smallest possible delta for floats in [0.5, 1.0) is
  // 1/(2^24), so that is the minimum distance we want to have between any two floats
  // this function may produce.
  // We mask off any additional bits and divide the integer by a value one larger than
  // the mask, this way the value can never be 1.0
  // I printed all possible 2^24 floats that this function may produce and can say:
  // it is perfect (at least with the default rounding mode). The difference between
  // all adjacent floats is equal (~0000000596...), the lowest value is true 0.0, and
  // the highest value is the largest representable float smaller than 1.0.
  return (float)(random_uint32() & 0xFFFFFF) / 16777216.0f;
}

float random_float(float min, float max)
{
  return min + random_float() * (max - min);
}

float normal_random(float mean, float variance)
{
  float x = random_float(-1.0f, 1.0f);
  return sign(x) * std::sqrt(-2.0f * std::log(std::abs(x)) * variance) + mean;
}

float topheavy_random(float min, float max)
{
  float r = random_float(0.0f, 1.0f);
  return (1.0f - r * r) * (max - min) + min;
}
