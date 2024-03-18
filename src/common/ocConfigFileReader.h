#pragma once

#include <cstddef> // size_t
#include <cstdint> // int32_t, etc

#include <string_view>

enum class ocConfigReadReport : int32_t
{
  // Successfully read file or key
  Success = (int32_t)0x00000000,

  // the file given to read_file() doesn't exist or can't be accessed
  File_Not_Accessible = (int32_t)0x80000001,

  // the file is not formatted the way the parser expects
  // e.g. more than 2 words in an uncommented line
  Bad_Format = (int32_t)0x80000002,

  // the given key doesn't exist in the file
  Key_Not_Found = (int32_t)0x80000004,

  // the key was found more than once in the file
  Key_Not_Unique = (int32_t)0x80000008,

  // the value associated with the given key has not the type that was asked for
  // type here means: int, float, string.
  Type_Mismatch = (int32_t)0x80000010,

  // the value associated with the given key does not fit in the expected type
  // e.g. 10000 doesn't fit into int8_t and -1 doesn't fit in uint32_t
  Range_Mismatch = (int32_t)0x80000020,

  // the string associated with the given key doesn't fit into the provided buffer
  Not_Enough_Space = (int32_t)0x80000040,
};

const char *to_string(ocConfigReadReport result);

class ocConfigFileReader final
{
private:
  struct Entry
  {
    int key_first;
    int key_last;
    int value_first;
    int value_last;
  };

  char  *_file_memory = nullptr;
  Entry *_entries     = nullptr;
  int    _num_entries = 0;

public:
  ~ocConfigFileReader();

  ocConfigReadReport read_file(const char *filename);
  void clear();

  std::string_view get_key(int i) const;
  std::string_view get_value(int i) const;
  std::string_view get_value(std::string_view key) const;

  ocConfigReadReport get_uint8(std::string_view key, uint8_t *target) const;
  ocConfigReadReport get_uint16(std::string_view key, uint16_t *target) const;
  ocConfigReadReport get_uint32(std::string_view key, uint32_t *target) const;
  ocConfigReadReport get_uint64(std::string_view key, uint64_t *target) const;
  ocConfigReadReport get_int8(std::string_view key, int8_t *target) const;
  ocConfigReadReport get_int16(std::string_view key, int16_t *target) const;
  ocConfigReadReport get_int32(std::string_view key, int32_t *target) const;
  ocConfigReadReport get_int64(std::string_view key, int64_t *target) const;
  ocConfigReadReport get_float32(std::string_view key, float *target) const;
  ocConfigReadReport get_float64(std::string_view key, double *target) const;
  ocConfigReadReport get_bool(std::string_view key, bool *target) const;

  // Checks the user-passed value after the key against an array of possible values and returns
  // the index, if a match exists.
  // If the key does not exist, ocConfigReadReport::Key_Not_Found is returned.
  // If the key is found multiple times, ocConfigReadReport::Key_Not_Unique is returned.
  // If the values is not part of the array, ocConfigReadReport::Range_Mismatch is returned.
  // Otherwise ocConfigReadReport::Success is returned.
  ocConfigReadReport get_index(std::string_view key, const char **array, size_t array_len, size_t *target) const;
};
