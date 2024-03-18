#pragma once

#include "ocArray.h"
#include "ocArrayIterator.h"
#include "ocAssert.h"

#include <cmath> // std::lerp

template<typename Key, typename Value>
class ocHistoryBuffer final
{
public:
  // Simple type for storing the keys and values interleaved in the same array.
  // Also used for iterating over the buffer.
  struct Entry
  {
    Key   key;
    Value val;
  };

private:
  // The number of elements, that exist in the buffer right now.
  // Is increased with every push, until the buffer limit is reached.
  // After that, the _length dies not increase any more and old elements
  // are dropped off the end of the array.
  size_t _length = 0;

  // Storage for the elements of the buffer. The Array has its own size
  // attribute, that is always greater of equal to the _length attribute
  // above.
  ocArray<Entry> _entries;

public:

  // This class must be constructed with an initial size for the internal
  // storage array. The constructor is marked explicit, so that the compiler
  // never tries to convert a single uint to this type.
  [[nodiscard]] explicit ocHistoryBuffer(uint32_t max_entries) : _entries(max_entries) {}

  // Returns the maximum number of entries that can be held in this buffer.
  [[nodiscard]] size_t get_capacity() const;

  // Sets the maximum number of entries that can be held in this buffer.
  void set_capacity(size_t length);

  // Returns the number of entries that are currently lend in this buffer.
  [[nodiscard]] size_t get_entry_count() const;

  // Getters
  // get_oldest_* returns the key or value with the smallest key.
  // Since the key is assumed to represent time, a slammer time values means
  // that it is older.
  // get_newest_* returns the key or value with the largest key.
  // get_*(i) returns the key or value at the given index. For getting a value
  // by a key, use get_nearest or get_interpolated.

  [[nodiscard]] const Key& get_oldest_key() const;
  [[nodiscard]] const Key& get_key(size_t i) const;
  [[nodiscard]] const Key& get_newest_key() const;

  [[nodiscard]] const Value& get_oldest_value() const;
  [[nodiscard]] const Value& get_value(size_t i) const;
  [[nodiscard]] const Value& get_newest_value() const;

  [[nodiscard]] bool contains(const Key& key) const;

  bool push(const Key& key, const Value& value);

  // Returns the nearest entry with a key that is smaller than the one given.
  [[nodiscard]] const Entry& get_nearest(const Key& key) const;

  // Finds the two nearest entries to the given key and linearly interpolates
  // between them to get a value for the given key.
  [[nodiscard]] Value get_interpolated(const Key& key) const;

  [[nodiscard]] ocArrayIterator<Entry> begin();
  [[nodiscard]] ocConstArrayIterator<Entry> begin() const;
  [[nodiscard]] ocArrayIterator<Entry> end();
  [[nodiscard]] ocConstArrayIterator<Entry> end() const;
};

template<typename Key, typename Value>
size_t ocHistoryBuffer<Key, Value>::get_capacity() const
{
  return _entries.get_length();
}

template<typename Key, typename Value>
void ocHistoryBuffer<Key, Value>::set_capacity(size_t length)
{
  if (length < _length) _length = length;
  _entries.set_length(length);
}

template<typename Key, typename Value>
const Key& ocHistoryBuffer<Key, Value>::get_oldest_key() const
{
  oc_assert(0 < _length);
  return _entries[_length - 1].key;
}

template<typename Key, typename Value>
const Value& ocHistoryBuffer<Key, Value>::get_oldest_value() const
{
  oc_assert(0 < _length);
  return _entries[_length - 1].val;
}

template<typename Key, typename Value>
const Key& ocHistoryBuffer<Key, Value>::get_newest_key() const
{
  oc_assert(0 < _length);
  return _entries[0].key;
}

template<typename Key, typename Value>
const Value& ocHistoryBuffer<Key, Value>::get_newest_value() const
{
  oc_assert(0 < _length);
  return _entries[0].val;
}

template<typename Key, typename Value>
const Key& ocHistoryBuffer<Key, Value>::get_key(size_t i) const
{
  oc_assert(i < _length, i, _length);
  return _entries[i].key;
}

template<typename Key, typename Value>
const Value& ocHistoryBuffer<Key, Value>::get_value(size_t i) const
{
  oc_assert(i < _length, i, _length);
  return _entries[i].val;
}

template<typename Key, typename Value>
size_t ocHistoryBuffer<Key, Value>::get_entry_count() const
{
  return _length;
}

template<typename Key, typename Value>
bool ocHistoryBuffer<Key, Value>::contains(const Key& key) const
{
  if (0 == _length) return false;
  return get_oldest_key() <= key && key <= get_newest_key();
}

template<typename Key, typename Value>
bool ocHistoryBuffer<Key, Value>::push(const Key& key, const Value& value)
{
  size_t insert_pos = _length;
  for (size_t i = 0; i < _length; ++i)
  {
    // if the keys are identical, just put in the new value.
    if (_entries[i].key == key)
    {
      _entries[i].val = value;
      return true;
    }
    if (_entries[i].key < key)
    {
      insert_pos = i;
      break;
    }
  }

  // special case if no current entry is smaller than the one we want to add
  if (_length == insert_pos)
  {
    // if the array has reached its size limit, we drop the entry
    if (_length == _entries.get_length()) return false;

    _entries[_length++] = { key, value };
    return true;
  }

  // otherwise throw away oldest entry to make space
  if (_length < _entries.get_length()) _length++;
  size_t len = _length - insert_pos - 1;
  _entries.get_space(insert_pos, len).copy_to(_entries.get_space(insert_pos + 1, len));
  _entries[insert_pos] = { key, value };
  return true;
}

template<typename Key, typename Value>
auto ocHistoryBuffer<Key, Value>::get_nearest(const Key& key) const -> const Entry&
{
  oc_assert(contains(key));

  for (size_t i = 0; i < _length - 1; ++i)
  {
    if (_entries[i + 1].key < key && key <= _entries[i].key)
    {
      return _entries[i];
    }
  }
  return _entries.last();
}

template<typename Key, typename Value>
Value ocHistoryBuffer<Key, Value>::get_interpolated(const Key& key) const
{
  oc_assert(contains(key));

  for (size_t i = 0; i < _length - 1; ++i)
  {
    if (_entries[i + 1].key < key && key <= _entries[i].key)
    {
      float fract = (key - _entries[i].key) / (_entries[i + 1].key - _entries[i].key);
      return (Value)std::lerp((float)_entries[i].val, (float)_entries[i + 1].val, fract);
    }
  }
  return _entries.last().val;
}

template<typename Key, typename Value>
auto ocHistoryBuffer<Key, Value>::begin() -> ocArrayIterator<Entry>
{
  return _entries.begin();
}

template<typename Key, typename Value>
auto ocHistoryBuffer<Key, Value>::begin() const -> ocConstArrayIterator<Entry>
{
  return _entries.begin();
}

template<typename Key, typename Value>
auto ocHistoryBuffer<Key, Value>::end() -> ocArrayIterator<Entry>
{
  return ocArrayIterator<Entry>(&_entries, (size_t)_length);
}

template<typename Key, typename Value>
auto ocHistoryBuffer<Key, Value>::end() const -> ocConstArrayIterator<Entry>
{
  return ocConstArrayIterator<Entry>(&_entries, (size_t)_length);
}
