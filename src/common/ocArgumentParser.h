#pragma once

#include <cstddef> // size_t
#include <cstdint> // int32_t, etc
#include <string_view> // int32_t, etc

class ocArgumentParser final
{
private:

  int          _argc = 0;
  const char **_argv = nullptr;

public:

  ocArgumentParser(int argc, const char **argv);

  std::string_view get_value(std::string_view key) const;

  bool has_key(std::string_view key) const;
  bool has_key_with_value(std::string_view key, std::string_view value) const;

  bool get_uint8(std::string_view key, uint8_t *target) const;
  bool get_uint16(std::string_view key, uint16_t *target) const;
  bool get_uint32(std::string_view key, uint32_t *target) const;
  bool get_uint64(std::string_view key, uint64_t *target) const;
  bool get_int8(std::string_view key, int8_t *target) const;
  bool get_int16(std::string_view key, int16_t *target) const;
  bool get_int32(std::string_view key, int32_t *target) const;
  bool get_int64(std::string_view key, int64_t *target) const;
  bool get_float32(std::string_view key, float *target) const;
  bool get_float64(std::string_view key, double *target) const;

  bool get_index(std::string_view key, const char **array, size_t array_len, size_t *target) const;
};
