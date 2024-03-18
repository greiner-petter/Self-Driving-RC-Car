#include "../ocAssert.h"
#include "../ocCommon.h"

#include <cmath>

int main()
{
  int64_t i;
  oc_assert(parse_signed_int("123", &i));
  oc_assert(i == 123, i);

  oc_assert(parse_signed_int("-1", &i));
  oc_assert(i == -1, i);

  oc_assert(parse_signed_int("0x123AbC", &i));
  oc_assert(i == 0x123ABC, i);

  oc_assert(parse_signed_int("0X123AbC", &i));
  oc_assert(i == 0x123ABC, i);

  i = 42;
  oc_assert(!parse_signed_int("", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("-", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("1.0", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("1a", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("a1", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("0x", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("0xG1", &i));
  oc_assert(i == 42, i);

  oc_assert(!parse_signed_int("100000000000000000000000000000000000", &i));
  oc_assert(i == 42, i);


  uint64_t u;
  oc_assert(parse_unsigned_int("123", &u));
  oc_assert(u == 123, u);

  oc_assert(parse_unsigned_int("0x123AbC", &u));
  oc_assert(u == 0x123ABC, u);

  oc_assert(parse_unsigned_int("0X123AbC", &u));
  oc_assert(u == 0x123ABC, u);

  u = 42;
  oc_assert(!parse_unsigned_int("-1", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("-", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("1.0", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("1a", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("a1", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("0x", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("0xG1", &u));
  oc_assert(u == 42, u);

  oc_assert(!parse_unsigned_int("100000000000000000000000000000000000", &u));
  oc_assert(u == 42, u);

  float f;
  oc_assert(parse_float32("1", &f));
  oc_assert(are_close(f, 1.0f, 1), f);

  oc_assert(parse_float32("1.", &f));
  oc_assert(are_close(f, 1.0f, 1), f);

  oc_assert(parse_float32("1.0", &f));
  oc_assert(are_close(f, 1.0f, 1), f);

  oc_assert(parse_float32("-1.0", &f));
  oc_assert(are_close(f, -1.0f, 1), f);

  oc_assert(parse_float32("1.0000", &f));
  oc_assert(are_close(f, 1.0f, 1), f);

  oc_assert(parse_float32(".0", &f));
  oc_assert(are_close(f, 0.0f, 1), f);

  oc_assert(parse_float32(".5", &f));
  oc_assert(are_close(f, 0.5f, 1), f);

  oc_assert(parse_float32("123.456", &f));
  oc_assert(are_close(f, 123.456f, 1), f);

  f = 42.0f;
  oc_assert(!parse_float32("", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  oc_assert(!parse_float32("a", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  oc_assert(!parse_float32("1..1", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  oc_assert(!parse_float32("1.1.1", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  oc_assert(!parse_float32("1,1", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  oc_assert(!parse_float32("a1", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  oc_assert(!parse_float32("1a", &f));
  oc_assert(are_close(f, 42.0f, 1), f);

  double d;
  oc_assert(parse_float64("1", &d));
  oc_assert(are_close(d, 1.0, 1), d);

  oc_assert(parse_float64("1.", &d));
  oc_assert(are_close(d, 1.0, 1), d);

  oc_assert(parse_float64("1.0", &d));
  oc_assert(are_close(d, 1.0, 1), d);

  oc_assert(parse_float64("-1.0", &d));
  oc_assert(are_close(d, -1.0, 1), d);

  oc_assert(parse_float64("1.0000", &d));
  oc_assert(are_close(d, 1.0, 1), d);

  oc_assert(parse_float64(".0", &d));
  oc_assert(are_close(d, 0.0, 1), d);

  oc_assert(parse_float64(".5", &d));
  oc_assert(are_close(d, 0.5, 1), d);

  oc_assert(parse_float64("123.456", &d));
  oc_assert(are_close(d, 123.456, 1), d);

  d = 42.0;
  oc_assert(!parse_float64("", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  oc_assert(!parse_float64("a", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  oc_assert(!parse_float64("1..1", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  oc_assert(!parse_float64("1.1.1", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  oc_assert(!parse_float64("1,1", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  oc_assert(!parse_float64("a1", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  oc_assert(!parse_float64("1a", &d));
  oc_assert(are_close(d, 42.0, 1), d);

  for (int i = 0; i < 100000; ++i)
  {
    float rand = random_float();
    oc_assert(0.0f <= rand);
    oc_assert(rand < 1.0f);
    oc_assert(!std::isinf(rand));
    oc_assert(!std::isnan(rand));
  }

  return 0;
}
