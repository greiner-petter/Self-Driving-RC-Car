#include "../ocAssert.h"
#include "../ocArgumentParser.h"
#include "../ocCommon.h"

int main()
{
  const int argc = 5;
  const char* argv[argc] = {"binary_name", "arg1", "arg2", "-pi", "123"};

  ocArgumentParser parser(argc, argv);

  oc_assert(parser.has_key("arg1"));
  oc_assert(parser.has_key("arg2"));
  oc_assert(parser.has_key("-pi"));
  oc_assert(!parser.has_key("-p"));
  oc_assert(parser.has_key("123"));
  oc_assert(!parser.has_key("arg3"));
  oc_assert(!parser.has_key("binary_name"));

  oc_assert(parser.get_value("-p").empty());
  oc_assert("123" == parser.get_value("-pi"));
  oc_assert("arg2" == parser.get_value("arg1"));
  oc_assert(parser.get_value("123").empty());
  oc_assert(parser.get_value("binary_name").empty());

  oc_assert(parser.has_key_with_value("-pi", "123"));
  oc_assert(!parser.has_key_with_value("-a", "123"));
  oc_assert(!parser.has_key_with_value("-pi", "12"));
  oc_assert(!parser.has_key_with_value("-p", "123"));

  uint8_t u8;
  oc_assert(parser.get_uint8("-pi", &u8));
  oc_assert(u8 == 123, u8);
  uint16_t u16;
  oc_assert(parser.get_uint16("-pi", &u16));
  oc_assert(u16 == 123, u16);
  uint32_t u32;
  oc_assert(parser.get_uint32("-pi", &u32));
  oc_assert(u32 == 123, u32);
  uint64_t u64;
  oc_assert(parser.get_uint64("-pi", &u64));
  oc_assert(u64 == 123, u64);
  int8_t s8;
  oc_assert(parser.get_int8("-pi", &s8));
  oc_assert(s8 == 123, s8);
  int16_t s16;
  oc_assert(parser.get_int16("-pi", &s16));
  oc_assert(s16 == 123, s16);
  int32_t s32;
  oc_assert(parser.get_int32("-pi", &s32));
  oc_assert(s32 == 123, s32);
  int64_t s64;
  oc_assert(parser.get_int64("-pi", &s64));
  oc_assert(s64 == 123, s64);

  oc_assert(!parser.get_uint8("a", &u8));
  oc_assert(u8 == 123, u8);
  oc_assert(!parser.get_uint16("a", &u16));
  oc_assert(u16 == 123, u16);
  oc_assert(!parser.get_uint32("a", &u32));
  oc_assert(u32 == 123, u32);
  oc_assert(!parser.get_uint64("a", &u64));
  oc_assert(u64 == 123, u64);
  oc_assert(!parser.get_int8("a", &s8));
  oc_assert(s8 == 123, s8);
  oc_assert(!parser.get_int16("a", &s16));
  oc_assert(s16 == 123, s16);
  oc_assert(!parser.get_int32("a", &s32));
  oc_assert(s32 == 123, s32);
  oc_assert(!parser.get_int64("a", &s64));
  oc_assert(s64 == 123, s64);

  return 0;
}
