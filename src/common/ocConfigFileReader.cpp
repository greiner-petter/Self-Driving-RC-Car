#include "ocConfigFileReader.h"

#include "ocCommon.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // O_RDONLY
#include <unistd.h>

const char *to_string(ocConfigReadReport result)
{
  switch (result)
  {
  case ocConfigReadReport::Success:             return "ocConfigReadReport::Success";
  case ocConfigReadReport::File_Not_Accessible: return "ocConfigReadReport::File_Not_Accessible";
  case ocConfigReadReport::Bad_Format:          return "ocConfigReadReport::Bad_Format";
  case ocConfigReadReport::Key_Not_Found:       return "ocConfigReadReport::Key_Not_Found";
  case ocConfigReadReport::Key_Not_Unique:      return "ocConfigReadReport::Key_Not_Unique";
  case ocConfigReadReport::Type_Mismatch:       return "ocConfigReadReport::Type_Mismatch";
  case ocConfigReadReport::Range_Mismatch:      return "ocConfigReadReport::Range_Mismatch";
  case ocConfigReadReport::Not_Enough_Space:    return "ocConfigReadReport::Not_Enough_Space";
  }
  return "<unknown>";
}

ocConfigFileReader::~ocConfigFileReader()
{
  clear();
}

ocConfigReadReport ocConfigFileReader::read_file(const char *filename)
{
  ocConfigReadReport result = ocConfigReadReport::Success;
  int file_handle = ::open(filename, O_RDONLY);
  if (file_handle < 0)
  {
    return ocConfigReadReport::File_Not_Accessible;
  }
  struct stat file_stats = {};
  int fstat_result = ::fstat(file_handle, &file_stats);
  if (fstat_result < 0)
  {
    ::close(file_handle);
    return ocConfigReadReport::File_Not_Accessible;
  }
  size_t file_size = (size_t)file_stats.st_size;

  _file_memory = (char *)std::realloc(_file_memory, file_size);
  size_t amount_read = 0;
  while (amount_read < file_size)
  {
    size_t remaining = file_size - amount_read;
    ssize_t read_len = ::read(file_handle, _file_memory + amount_read, remaining);
    if (read_len < 0)
    {
      ::close(file_handle);
      return ocConfigReadReport::File_Not_Accessible;
    }
    if (0 == read_len)
    {
      file_size = amount_read;
      break;
    }
    amount_read += (size_t)read_len;
  }
  ::close(file_handle);

  int  allocated_entries = 100;
  int  words = 0;
  bool comment = false;
  bool space = true;
  bool escape = false;

  _entries = (Entry *)std::realloc(_entries, (size_t)allocated_entries * sizeof(Entry));

  for (int i = 0; i < (int)amount_read; ++i)
  {
    if (escape)
    {
      escape = false;
    }
    else
    {
      switch (_file_memory[i])
      {
        case '\n':
        case '\r':
        {
          if (1 == words)
          {
            result = ocConfigReadReport::Bad_Format;
          }
          if (!space)
          {
            _entries[_num_entries].value_last = i;
            _num_entries++;
          }
          words = 0;
          comment = false;
          space = true;
          _file_memory[i] = '\0';
        } break;
        case '\\':
        {
          escape = true;
        } break;
        case '#':
        {
          comment = true;
        } break;
        case ' ':
        case '\t':
        {
          if (!space)
          {
            if (1 == words)
            {
              if (_num_entries == allocated_entries)
              {
                allocated_entries += 100;
                _entries = (Entry *)std::realloc(_entries, (size_t)allocated_entries * sizeof(Entry));
              }
              _entries[_num_entries].key_last = i;
              auto key = get_key(_num_entries);
              for (int j = 0; j < _num_entries; ++j)
              {
                if (get_key(j) == key)
                {
                  return ocConfigReadReport::Key_Not_Unique;
                }
              }
            }
            else if (2 == words)
            {
              _entries[_num_entries].value_last = i;
              _num_entries++;
            }
          }
          space = true;
          _file_memory[i] = '\0';
        } break;
        default:
        {
          if (!comment && space)
          {
            switch (words)
            {
              case 0: 
              {
                // We encountered the first word in this line, it must be the key.
                // Remember the index for later when we want to iterate over all keys.
                _entries[_num_entries].key_first = i;
              } break;
              case 1: 
              {
                // The second word in this line must be the value.
                // Remember the index for later when we want to access values by index.
                _entries[_num_entries].value_first = i;
              } break;
              default:
              {
                // If we encounter more than the two words, there is an error in the config file.
                // Note that comments don't count as words.
                result = ocConfigReadReport::Bad_Format;
              } break;
            }
            words++;
            space = false;
          }
        } break;
      }
    }
  }

  return result;
}

void ocConfigFileReader::clear()
{
  std::free(_file_memory);
  std::free(_entries);
  _file_memory = nullptr;
  _entries     = nullptr;
  _num_entries = 0;
}

std::string_view ocConfigFileReader::get_key(int i) const
{
  return {
    _file_memory + _entries[i].key_first,
    _file_memory + _entries[i].key_last
  };
}
std::string_view ocConfigFileReader::get_value(int i) const
{
  return {
    _file_memory + _entries[i].value_first,
    _file_memory + _entries[i].value_last
  };
}

std::string_view ocConfigFileReader::get_value(std::string_view key) const
{
  for (int i = 0; i < _num_entries; ++i)
  {
    if (get_key(i) == key)
    {
      return get_value(i);
    }
  }
  return {};
}

ocConfigReadReport ocConfigFileReader::get_uint8(std::string_view key, uint8_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  uint64_t value;
  if (!parse_unsigned_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  if (((uint8_t)value) != value)
  {
    return ocConfigReadReport::Range_Mismatch;
  }
  *target = (uint8_t)value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_uint16(std::string_view key, uint16_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  uint64_t value;
  if (!parse_unsigned_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  if (((uint16_t)value) != value)
  {
    return ocConfigReadReport::Range_Mismatch;
  }
  *target = (uint16_t)value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_uint32(std::string_view key, uint32_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  uint64_t value;
  if (!parse_unsigned_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  if (((uint32_t)value) != value)
  {
    return ocConfigReadReport::Range_Mismatch;
  }
  *target = (uint32_t)value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_uint64(std::string_view key, uint64_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  uint64_t value;
  if (!parse_unsigned_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  *target = value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_int8(std::string_view key, int8_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  int64_t value;
  if (!parse_signed_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  if (((int8_t)value) != value)
  {
    return ocConfigReadReport::Range_Mismatch;
  }
  *target = (int8_t)value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_int16(std::string_view key, int16_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  int64_t value;
  if (!parse_signed_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  if (((int16_t)value) != value)
  {
    return ocConfigReadReport::Range_Mismatch;
  }
  *target = (int16_t)value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_int32(std::string_view key, int32_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  int64_t value;
  if (!parse_signed_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  if (((int32_t)value) != value)
  {
    return ocConfigReadReport::Range_Mismatch;
  }
  *target = (int32_t)value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_int64(std::string_view key, int64_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  int64_t value;
  if (!parse_signed_int(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  *target = value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_float32(std::string_view key, float *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  float value;
  if (!parse_float32(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  *target = value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_float64(std::string_view key, double *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  double value;
  if (!parse_float64(string, &value))
  {
    return ocConfigReadReport::Type_Mismatch;
  }
  *target = value;

  return ocConfigReadReport::Success;
}

ocConfigReadReport ocConfigFileReader::get_bool(std::string_view key, bool *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  if (string == "true")
  {
    *target = true;
    return ocConfigReadReport::Success;
  }
  if (string == "false")
  {
    *target = false;
    return ocConfigReadReport::Success;
  }
  return ocConfigReadReport::Type_Mismatch;
}

ocConfigReadReport ocConfigFileReader::get_index(std::string_view key, const char **array, size_t array_len, size_t *target) const
{
  auto string = get_value(key);
  if (string.empty()) return ocConfigReadReport::Key_Not_Found;

  for (size_t i = 0; i < array_len; ++i)
  {
    if (string == array[i])
    {
      *target = i;
      return ocConfigReadReport::Success;
    }
  }

  return ocConfigReadReport::Range_Mismatch;
}
