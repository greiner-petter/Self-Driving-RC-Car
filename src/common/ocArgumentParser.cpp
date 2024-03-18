#include "ocArgumentParser.h"

#include "ocCommon.h"

ocArgumentParser::ocArgumentParser(int argc, const char **argv)
{
  // skip the first argument, because it's just the path the executable was
  // called with.
  _argc = argc - 1;
  _argv = &argv[1];
}

std::string_view ocArgumentParser::get_value(std::string_view key) const
{
  // loop backwards, so that later values are prioritized if a key exists twice.
  // But skip the last value, since that can't be a key with a value after it.
  for (int i = _argc - 2; 0 <= i; --i)
  {
    // Is the key equal to the argument?
    if (key == _argv[i])
    {
        return _argv[i + 1];
    }
  }
  // return an empty view, if none of the keys matched
  return {};
}

bool ocArgumentParser::has_key(std::string_view key) const
{
  for (int i = 0; i < _argc; ++i)
  {
    // check if the argument completely matches the key
    if (key == _argv[i])
    {
      return true;
    }
    // otherwise loop over the next key.
  }
  // return false, if none of the keys matched
  return false;
}

bool ocArgumentParser::has_key_with_value(std::string_view key, std::string_view value) const
{
  return value == get_value(key);
}

bool ocArgumentParser::get_uint8(std::string_view key, uint8_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  uint64_t number;
  if (!parse_unsigned_int(value, &number)) return false;
  if (number != (uint8_t) number) return false;
  if (target) *target = (uint8_t)number;
  return true;
}
bool ocArgumentParser::get_uint16(std::string_view key, uint16_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  uint64_t number;
  if (!parse_unsigned_int(value, &number)) return false;
  if (number != (uint16_t) number) return false;
  if (target) *target = (uint16_t)number;
  return true;
}
bool ocArgumentParser::get_uint32(std::string_view key, uint32_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  uint64_t number;
  if (!parse_unsigned_int(value, &number)) return false;
  if (number != (uint32_t) number) return false;
  if (target) *target = (uint32_t)number;
  return true;
}
bool ocArgumentParser::get_uint64(std::string_view key, uint64_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  uint64_t number;
  if (!parse_unsigned_int(value, &number)) return false;
  if (target) *target = number;
  return true;
}
bool ocArgumentParser::get_int8(std::string_view key, int8_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  int64_t number;
  if (!parse_signed_int(value, &number)) return false;
  if (number != (int8_t) number) return false;
  if (target) *target = (int8_t)number;
  return true;
}
bool ocArgumentParser::get_int16(std::string_view key, int16_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  int64_t number;
  if (!parse_signed_int(value, &number)) return false;
  if (number != (int16_t) number) return false;
  if (target) *target = (int16_t)number;
  return true;
}
bool ocArgumentParser::get_int32(std::string_view key, int32_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  int64_t number;
  if (!parse_signed_int(value, &number)) return false;
  if (number != (int32_t) number) return false;
  if (target) *target = (int32_t)number;
  return true;
}
bool ocArgumentParser::get_int64(std::string_view key, int64_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  int64_t number;
  if (!parse_signed_int(value, &number)) return false;
  if (target) *target = number;
  return true;
}
bool ocArgumentParser::get_float32(std::string_view key, float *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  float number;
  if (!parse_float32(value, &number)) return false;
  if (target) *target = (float)number;
  return true;
}
bool ocArgumentParser::get_float64(std::string_view key, double *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  double number;
  if (!parse_float64(value, &number)) return false;
  if (target) *target = number;
  return true;
}

bool ocArgumentParser::get_index(std::string_view key, const char **array, size_t array_len, size_t *target) const
{
  auto value = get_value(key);
  if (value.empty()) return false;
  for (size_t i = 0; i < array_len; ++i)
  {
    if (value == array[i])
    {
      *target = i;
      return true;
    }
  }
  return false;
}
