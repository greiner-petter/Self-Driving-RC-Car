#pragma once

#include <compare> // std::strong_ordering
#include <iterator> // std::random_access_iterator_tag

template<typename T>
class ocArray;

template<typename T>
class ocArrayIterator final
{
private:

  ocArray<T> *_array;
  size_t      _current_index;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = T*;
  using reference = T&;

  ocArrayIterator() = default;
  explicit ocArrayIterator(ocArray<T> *array, size_t index = 0) :
    _array(array), _current_index(index) {}

  size_t index() const
  {
    return _current_index;
  }

  ocArrayIterator& operator++()
  {
    ++_current_index;
    return *this;
  }

  ocArrayIterator operator++(int)
  {
    return ocArrayIterator(_array, _current_index++);
  }

  ocArrayIterator& operator--()
  {
    --_current_index;
    return *this;
  }

  ocArrayIterator operator--(int)
  {
    return ocArrayIterator(_array, _current_index--);
  }

  ocArrayIterator operator+(difference_type diff) const
  {
    return ocArrayIterator(_array, _current_index + (size_t)diff);
  }

  ocArrayIterator &operator+=(difference_type diff)
  {
    _current_index += diff;
    return *this;
  }

  ocArrayIterator operator-(difference_type diff)
  {
    return ocArrayIterator(_array, _current_index - (size_t)diff);
  }

  ocArrayIterator &operator-=(difference_type diff)
  {
    _current_index -= diff;
    return *this;
  }

  difference_type operator-(const ocArrayIterator &it) const
  {
    return difference_type(_current_index - it._current_index);
  }

  reference operator*() const
  {
    return (*_array)[_current_index];
  }

  reference operator[](size_t index) const
  {
    return (*_array)[_current_index + index];
  }

  bool operator==(const ocArrayIterator &it) const
  {
    return _array == it._array && _current_index == it._current_index;
  }

  std::strong_ordering operator<=>(const ocArrayIterator &it) const
  {
    return _current_index <=> it._current_index;
  }
};

template<typename T>
class ocConstArrayIterator final
{
private:

  const ocArray<T> *_array;
  size_t            _current_index;

public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = const T*;
  using reference = const T&;

  ocConstArrayIterator() = default;
  explicit ocConstArrayIterator(const ocArray<T> *array, size_t index = 0) :
    _array(array), _current_index(index) {}

  size_t index() const
  {
    return _current_index;
  }

  ocConstArrayIterator &operator++()
  {
    ++_current_index;
    return *this;
  }

  ocConstArrayIterator operator++(int)
  {
    return ocConstArrayIterator(_array, _current_index++);
  }

  ocConstArrayIterator &operator--()
  {
    --_current_index;
    return *this;
  }

  ocConstArrayIterator operator--(int)
  {
    return ocConstArrayIterator(_array, _current_index--);
  }

  ocConstArrayIterator operator+(difference_type diff) const
  {
    return ocConstArrayIterator(_array, _current_index + diff);
  }

  ocConstArrayIterator &operator+=(difference_type diff)
  {
    _current_index += diff;
    return *this;
  }

  ocConstArrayIterator operator-(difference_type diff)
  {
    return ocConstArrayIterator(_array, _current_index - diff);
  }

  ocConstArrayIterator &operator-=(difference_type diff)
  {
    _current_index -= diff;
    return *this;
  }

  difference_type operator-(const ocConstArrayIterator &it) const
  {
    return _current_index - it._current_index;
  }

  reference operator*() const
  {
    return (*_array)[_current_index];
  }

  reference operator[](size_t index) const
  {
    return (*_array)[_current_index + index];
  }

  bool operator==(const ocConstArrayIterator &it) const
  {
    return _array == it._array && _current_index == it._current_index;
  }

  std::strong_ordering operator<=>(const ocConstArrayIterator &it) const
  {
    return _current_index <=> it._current_index;
  }
};
