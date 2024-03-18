#pragma once

#include "ocAssert.h"

#include <cstring> // memmove

template<typename T>
class ocArray;

template<typename T>
class ocArrayIterator;

template<typename T>
class ocConstArrayIterator;

template<typename T>
class ocArrayView;

template<typename T>
class ocConstArrayView;

template<typename T>
class ocArrayView final
{
private:
  ocArray<T> *_array;
  size_t      _offset;
  size_t      _length;

public:
  ocArrayView(
    ocArray<T> *array,
    size_t      offset,
    size_t      length)
  {
    oc_assert(array);
    oc_assert(0 < length);
    oc_assert(offset < array->get_length(), offset, array->get_length());
    oc_assert(offset + length <= array->get_length(), offset, length, array->get_length());
    _array = array;
    _offset = offset;
    _length = length;
  }
  ocArrayView(const ocArrayView&) = default;

  ocArrayView& operator=(const ocArrayView&) = default;

  bool is_valid() const
  {
    return (_offset + _length) <= _array->get_length();
  }

  T &operator[](size_t index)
  {
    oc_assert(is_valid());
    oc_assert(index < _length, index, _length);
    return (*_array)[_offset + index];
  }

  const T &operator[](size_t index) const
  {
    oc_assert(is_valid());
    oc_assert(index < _length, index, _length);
    return (*_array)[_offset + index];
  }

  T &first()
  {
    oc_assert(is_valid());
    oc_assert(0 < _length);

    return (*_array)[_offset];
  }

  const T &first() const
  {
    oc_assert(is_valid());
    oc_assert(0 < _length);

    return (*_array)[_offset];
  }

  T &last()
  {
    oc_assert(is_valid());
    oc_assert(0 < _length);

    return (*_array)[_offset + _length - 1];
  }

  const T &last() const
  {
    oc_assert(is_valid());
    oc_assert(0 < _length);

    return (*_array)[_offset + _length - 1];
  }

  ocArrayView get_space(size_t index, size_t length)
  {
    oc_assert(is_valid());
    oc_assert(0 < length);
    oc_assert(index + length <= _length, index, length, _length);
    return ocArrayView(_array, _offset + index, length);
  }

  ocConstArrayView<T> get_space(size_t index, size_t length) const
  {
    oc_assert(is_valid());
    oc_assert(0 < length);
    oc_assert(index + length <= _length, index, length, _length);
    return ocConstArrayView<T>(_array, _offset + index, length);
  }

  operator T*()
  {
    oc_assert(is_valid());
    return &(*_array)[_offset];
  }
  operator const T*() const
  {
    oc_assert(is_valid());
    return &(*_array)[_offset];
  }

  size_t get_length() const
  {
    return _length;
  }

  void fill(const T &value)
  {
    for (size_t i = _offset; i < _offset + _length; ++i)
    {
      (*_array)[i] = value;
    }
  }

  size_t copy_to(ocArrayView other) const
  {
    oc_assert(is_valid());
    oc_assert(other.is_valid());
    oc_assert(_length <= other._length, _length, other._length);
    if (0 == _length) return 0;
    memmove((void *)other, (const T *)*this, _length * sizeof(T));
    return _length;
  }
  size_t copy_to(T *dst) const
  {
    oc_assert(is_valid());
    oc_assert(dst);
    if (0 == _length) return 0;
    memmove(dst, (const T *)*this, _length * sizeof(T));
    return _length;
  }
  size_t copy_to(T *dst, size_t length) const
  {
    oc_assert(is_valid());
    oc_assert(dst);
    oc_assert(_length <= length, _length, length);
    if (0 == _length) return 0;
    memmove(dst, (const T *)*this, _length * sizeof(T));
    return _length;
  }
  size_t copy_from(const ocArrayView other)
  {
    oc_assert(is_valid());
    oc_assert(other.is_valid());
    oc_assert(other._length <= _length, other._length, _length);
    if (0 == other._length) return 0;
    memmove((T *)*this, (const T *)other, other._length * sizeof(T));
    return other._length;
  }
  size_t copy_from(const ocConstArrayView<T> other)
  {
    oc_assert(is_valid());
    oc_assert(other.is_valid());
    oc_assert(other.get_length() <= _length, other.get_length(), _length);
    if (0 == other.get_length()) return 0;
    memmove((T *)*this, (const T *)other, other.get_length() * sizeof(T));
    return other.get_length();
  }
  size_t copy_from(const T *src)
  {
    oc_assert(is_valid());
    oc_assert(src);
    if (0 == _length) return 0;
    memmove((T *)*this, src, _length * sizeof(T));
    return _length;
  }
  size_t copy_from(const T *src, size_t length)
  {
    oc_assert(is_valid());
    oc_assert(src);
    oc_assert(length <= _length, length, _length);
    if (0 == length) return 0;
    memmove((T *)*this, src, length * sizeof(T));
    return length;
  }

  ocArrayIterator<T> begin()
  {
    return ocArrayIterator<T>(_array, _offset);
  }

  ocConstArrayIterator<T> begin() const
  {
    return ocConstArrayIterator<T>(_array, _offset);
  }

  ocArrayIterator<T> end()
  {
    return ocArrayIterator<T>(_array, _offset + _length);
  }

  ocConstArrayIterator<T> end() const
  {
    return ocConstArrayIterator<T>(_array, _offset + _length);
  }
};

template<typename T>
class ocConstArrayView final
{
private:
  const ocArray<T> *_array;
  size_t            _offset;
  size_t            _length;

public:
  ocConstArrayView(
    const ocArray<T> *array,
    size_t            offset,
    size_t            length)
  {
    oc_assert(array);
    oc_assert(0 < length);
    oc_assert(offset < array->get_length(), offset, array->get_length());
    oc_assert(offset + length <= array->get_length(), offset, length, array->get_length());
    _array = array;
    _offset = offset;
    _length = length;
  }

  ocConstArrayView(const ocConstArrayView&) = default;

  ocConstArrayView& operator=(const ocConstArrayView&) = default;

  bool is_valid() const
  {
    return (_offset + _length) <= _array->get_length();
  }

  const T &operator[](size_t index) const
  {
    oc_assert(is_valid());
    oc_assert(index < _length, index, _length);
    return (*_array)[_offset + index];
  }

  const T &first() const
  {
    oc_assert(is_valid());
    oc_assert(0 < _length);

    return (*_array)[_offset];
  }

  const T &last() const
  {
    oc_assert(is_valid());
    oc_assert(0 < _length);

    return (*_array)[_offset + _length - 1];
  }

  ocConstArrayView get_space(size_t index, size_t length) const
  {
    oc_assert(is_valid());
    oc_assert(0 < length);
    oc_assert(index + length <= _length, index, length, _length);
    return ocConstArrayView(_array, _offset + index, length);
  }

  operator const T*() const
  {
    oc_assert(is_valid());
    return &(*_array)[_offset];
  }

  size_t get_length() const
  {
    return _length;
  }

  size_t copy_to(ocArrayView<T> other) const
  {
    oc_assert(is_valid());
    oc_assert(other.is_valid());
    oc_assert(_length <= other.get_length(), _length, other.get_length());
    if (0 == _length) return 0;
    memmove((void *)other, (const T *)*this, _length * sizeof(T));
    return _length;
  }
  size_t copy_to(T *dst) const
  {
    oc_assert(is_valid());
    oc_assert(dst);
    if (0 == _length) return 0;
    memmove(dst, (const T *)*this, _length * sizeof(T));
    return _length;
  }
  size_t copy_to(T *dst, size_t length) const
  {
    oc_assert(is_valid());
    oc_assert(dst);
    oc_assert(_length <= length, _length, length);
    if (0 == _length) return 0;
    memmove(dst, (const T *)*this, _length * sizeof(T));
    return _length;
  }

  ocConstArrayIterator<T> begin() const
  {
    return ocConstArrayIterator<T>(_array, _offset);
  }

  ocConstArrayIterator<T> end() const
  {
    return ocConstArrayIterator<T>(_array, _offset + _length);
  }
};
