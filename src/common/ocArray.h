#pragma once

#include "ocAssert.h"
#include "ocArrayIterator.h"
#include "ocArrayView.h"

#include <cstdlib> // realloc(), free(), size_t
#include <initializer_list>
#include <type_traits> // std::is_..._v<T>

template<typename T>
class ocArray final
{
  static_assert(std::is_default_constructible_v<T>);
  static_assert(std::is_trivially_copyable_v<T>);

private:
  T     *_memory = nullptr;
  size_t _length = 0;

public:

  /**
   * Default constructor that creates an empty array.
   */
  [[nodiscard]] ocArray() = default;

  /**
   * Creates an array with memory allocated for the given number of elements.
   * It is declared explicit, because we never want an size_t to be implicitly
   * converted to an ocArray.
   */
  [[nodiscard]] explicit ocArray(size_t initial_length);

  /**
   * No copy constructor is provided, to avoid accidental copies. You probably
   * want to pass a reference, pointer or view of the array. If you really
   * want to copy the data, you have to do that explicitly. E.g.:
   *   ocArray<...> new_arr;
   *   new_arr.make_space(old_arr.get_length()).copy_from(old_arr.all());
   */
  ocArray(const ocArray &other) = delete;

  /**
   * Move constructor that takes the allocated memory of the given array, as
   * well as the length value. This constructor is used for example when
   * returning an array from a function by value.
   */
  [[nodiscard]] ocArray(ocArray &&other);

  /**
   * This constructor allows to write an array in list-form when an ocArray is
   * expected, for example as a function parameter:
   *
   * void foo(ocArray<char> array);
   *
   * foo({'a', 'b', 'c'});
   */
  [[nodiscard]] ocArray(std::initializer_list<T> values);

  /**
   * Destructor of the array, deallocates the internal memory.
   */
  ~ocArray();

  /**
   * No copying assignment operator is provided, to prevent accidental copies.
   * See the deleted copy constructor for more info.
   */
  ocArray &operator=(const ocArray &other) = delete;

  /**
   * Takes the allocated memory and length of the other array and uses them
   * for this array.
   */
  ocArray &operator=(ocArray &&other);

  /**
   * Deallocates the internal memory and sets the length to 0.
   */
  void clear();

  /**
   * Returns the number of elements that are currently allocated by this array.
   */
  [[nodiscard]] size_t get_length() const;

  /**
   * Changes the number of allocated elements of this array.
   */
  void set_length(size_t new_length);

  /**
   * Returns true if the array length is 0.
   */
  [[nodiscard]] bool is_empty() const;

  /**
   * Creates an array view of the given length at the beginning of the array.
   * All preexisting data is pushed to higher indices to make room for this
   * view.
   *
   * ocArray arr = {'A', 'S', 'D'};
   *
   *   0   1   2   length: 3
   * +---+---+---+
   * |'A'|'S'|'D'|
   * +---+---+---+
   *
   * arr.prepend_space(2).fill('X');
   *
   *   0   1   2   3   4   length: 5
   * +---+---+---+---+---+
   * |'X'|'X'|'A'|'S'|'D'|
   * +---+---+---+---+---+
   */
  [[nodiscard]] ocArrayView<T> prepend_space(size_t length);

  /**
   * Creates an array view of the given length at the end of the array.
   * No data needs to be moved and only the array length is increased.
   *
   * ocArray arr = {'A', 'S', 'D'};
   *
   *   0   1   2   length: 3
   * +---+---+---+
   * |'A'|'S'|'D'|
   * +---+---+---+
   *
   * arr.append_space(2).fill('X');
   *
   *   0   1   2   3   4   length: 5
   * +---+---+---+---+---+
   * |'A'|'S'|'D'|'X'|'X'|
   * +---+---+---+---+---+
   */
  [[nodiscard]] ocArrayView<T> append_space(size_t length);

  /**
   * Creates an array view of the given length starting at the given index in
   * the array. Any preexisting data at or after that index is pushed to higher
   * indices.
   *
   * ocArray arr = {'A', 'S', 'D'};
   *
   *   0   1   2   length: 3
   * +---+---+---+
   * |'A'|'S'|'D'|
   * +---+---+---+
   *
   * arr.make_space(1, 2).fill('X');
   *
   *   0   1   2   3   4   length: 5
   * +---+---+---+---+---+
   * |'A'|'X'|'X'|'S'|'D'|
   * +---+---+---+---+---+
   */
  [[nodiscard]] ocArrayView<T> make_space(size_t index, size_t length);

  /**
   * Creates an array view of the already allocated elements of this array.
   * The view starts at the given index and has the given length. If the
   * requested space does not fit inside the current array, this method will
   * assert and exit.
   */
  [[nodiscard]] ocArrayView<T> get_space(size_t index, size_t length);

  /**
   * Creates a constant array view of the already allocated elements of this
   * array. The view starts at the given index and has the given length. If the
   * requested space does not fit inside the current array, this method will
   * assert and exit.
   */
  [[nodiscard]] ocConstArrayView<T> get_space(size_t index, size_t length) const;

  /**
   * Deletes the given range of elements and pulls all elements with higher
   * indices down by the amount given by length.
   */
  void remove_space(size_t index, size_t length);

  /**
   * Creates an array view that starts at index 0 and has a length equal to the
   * length of the array at the time of this call.
   */
  [[nodiscard]] ocArrayView<T> all();

  /**
   * Creates a constant array view that starts at index 0 and has a length equal
   * to the length of the array at the time of this call.
   */
  [[nodiscard]] ocConstArrayView<T> all() const;

  /**
   * Gives bounds-checked access to the elements of the array.
   */
  [[nodiscard]] T &operator[](size_t index);

  /**
   * Gives read-only bounds-checked access to the elements of the array.
   */
  [[nodiscard]] const T &operator[](size_t index) const;

  /**
   * Returns a reference to the first element of the array. If the length is 0,
   * this methods will assert and exit.
   */
  [[nodiscard]] T &first();

  /**
   * Returns a constant reference to the first element of the array. If the length
   * is 0, this methods will assert and exit.
   */
  [[nodiscard]] const T &first() const;

  /**
   * Returns a view of the first 'length' elements of the array. Will assert, that
   * the array is at least of size 'length'.
   */
  [[nodiscard]] ocArrayView<T> first(size_t length);

  /**
   * Returns a const view of the first 'length' elements of the array. Will assert,
   * that the array is at least of size 'length'.
   */
  [[nodiscard]] ocConstArrayView<T> first(size_t length) const;

  /**
   * Returns a reference to the last element of the array. If the size is 0,
   * this methods will assert and exit.
   */
  [[nodiscard]] T &last();

  /**
   * Returns a constant reference to the last element of the array. If the size
   * is 0, this methods will assert and exit.
   */
  [[nodiscard]] const T &last() const;

  /**
   * Returns a view of the last 'length' elements of the array. Will assert, that
   * the array is at least of size 'length'.
   */
  [[nodiscard]] ocArrayView<T> last(size_t length);

  /**
   * Returns a const view of the last 'length' elements of the array. Will assert,
   * that the array is at least of size 'length'.
   */
  [[nodiscard]] ocConstArrayView<T> last(size_t length) const;

  /**
   * Prepends a single value at the start of the array, increasing the size of
   * the array by one, and pushes every other element up by one index.
   * If the array is already at its maximum capacity this function will assert
   * and exit.
   */
  void prepend(const T &value);

  /**
   * Appends a single value at the end of the array, increasing the size of the
   * array by one.
   * If the array is already at its maximum capacity this function will assert
   * and exit.
   */
  void append(const T &value);

  /**
   * Appends a another array of the same type at the end of this array.
   * If the array is already at its maximum capacity this function will assert
   * and exit.
   */
  void append(const ocArray &arr);

  /**
   * Inserts a single value at the given index into the array, increasing the
   * size of the array by one, and pushes every element at and after the index
   * up by one.
   * If the array is already at its maximum capacity this function will assert
   * and exit.
   */
  void insert(size_t index, const T &value);

  /**
   * Deletes the element at the given index and pulls all elements at higher
   * indexes down by one.
   */
  void remove_at(size_t index);

  /**
   * Deletes the first element and pulls all elements at higher  indexes down
   * by one.
   */
  void remove_first();

  /**
   * Deletes the last element and reduces the array length.
   */
  void remove_last();

  /**
   * Creates an iterator at the beginning of the array.
   */
  [[nodiscard]] ocArrayIterator<T> begin();

  /**
   * Creates an iterator at the beginning of the array that can't make changes.
   */
  [[nodiscard]] ocConstArrayIterator<T> begin() const;

  /**
   * Creates an iterator at the end of the array, pointing at one element past
   * the last valid element.
   */
  [[nodiscard]] ocArrayIterator<T> end();

  /**
   * Creates an iterator at the end of the array, pointing at one element past
   * the last valid element. This iterator can't make changes to the array.
   */
  [[nodiscard]] ocConstArrayIterator<T> end() const;

  /**
   * Searches the array front-to-back for the given value, returns the first index
   * where it was found, or one past the last index if not.
   */
  [[nodiscard]] size_t first_index_of(const T &value);

  /**
   * Searches the array back-to-front for the given value, returns the first index
   * where it was found, or one past the last index if not.
   */
  [[nodiscard]] size_t last_index_of(const T &value);
};

// ----------------------------------------------------------------------------
// Implementations

template<typename T>
ocArray<T>::ocArray(size_t initial_length)
{
  set_length(initial_length);
}

template<typename T>
ocArray<T>::ocArray(ocArray &&other)
{
  _memory = other._memory;
  _length = other._length;
  other._memory = nullptr;
  other._length   = 0;
}

template<typename T>
ocArray<T>::ocArray(std::initializer_list<T> values)
{
  set_length(values.size());
  size_t i = 0;
  for (auto &value : values)
  {
    _memory[i++] = value;
  }
}

template<typename T>
ocArray<T>::~ocArray()
{
  clear();
}

template<typename T>
ocArray<T> &ocArray<T>::operator=(ocArray &&other)
{
  _memory = other._memory;
  _length = other._length;
  other._memory = nullptr;
  other._length = 0;
  return *this;
}

template<typename T>
void ocArray<T>::clear()
{
  if (_memory)
  {
    for (T& elem : *this) elem.~T();
    free(_memory);
    _memory = nullptr;
    _length = 0;
  }
}

template<typename T>
size_t ocArray<T>::get_length() const
{
  return _length;
}

template<typename T>
void ocArray<T>::set_length(size_t new_length)
{
  if (_length == new_length) return;

  if (0 == new_length)
  {
    clear();
  }
  else
  {
    size_t old_length = _length;
    // Destruct all elements that will be dropped off the end
    // if the memory is shrinking.
    if (new_length < old_length)
    {
      for (T& elem : last(old_length - new_length)) elem.~T();
    }

    _memory = (T *)realloc(_memory, new_length * sizeof(T));
    oc_assert(_memory);
    oc_assert(0 == ((size_t)_memory) % alignof(T), (size_t)_memory, alignof(T));

    _length = new_length;
    // Default-construct all new elements that are added
    // if the memory is growing.
    if (old_length < new_length)
    {
      for (T& elem : last(new_length - old_length)) new (&elem) T();
    }
  }
}

template<typename T>
bool ocArray<T>::is_empty() const
{
  return 0 == _length;
}

template<typename T>
ocArrayView<T> ocArray<T>::prepend_space(size_t length)
{
  oc_assert(0 < length);

  size_t old_length = _length;
  set_length(_length + length);
  first(old_length).copy_to(last(old_length));

  return ocArrayView<T>(this, 0, length);
}

template<typename T>
ocArrayView<T> ocArray<T>::append_space(size_t length)
{
  oc_assert(0 < length);

  size_t old_length = _length;
  set_length(_length + length);
  return ocArrayView<T>(this, old_length, length);
}

template<typename T>
ocArrayView<T> ocArray<T>::make_space(size_t index, size_t length)
{
  oc_assert(0 < length);
  oc_assert(index <= _length, index, _length);

  size_t move_length = _length - index;
  set_length(_length + length);
  if (0 < move_length)
  {
    get_space(index, move_length).copy_to(get_space(index + length, move_length));
  }
  return ocArrayView<T>(this, index, length);
}

template<typename T>
ocArrayView<T> ocArray<T>::get_space(size_t index, size_t length)
{
  oc_assert(0 < length);
  oc_assert(index + length <= _length, index, length, _length);

  return ocArrayView<T>(this, index, length);
}

template<typename T>
ocConstArrayView<T> ocArray<T>::get_space(size_t index, size_t length) const
{
  oc_assert(0 < length);
  oc_assert(index + length <= _length, index, length, _length);

  return ocConstArrayView<T>(this, index, length);
}

template<typename T>
void ocArray<T>::remove_space(size_t index, size_t length)
{
  oc_assert(0 < length);
  oc_assert(index + length <= _length, index, length, _length);

  size_t end = index + length;
  size_t move_length = _length - end;
  if (0 < move_length)
  {
    get_space(end, move_length).copy_to(get_space(index, move_length));
  }
  set_length(_length - length);
}

template<typename T>
ocArrayView<T> ocArray<T>::all()
{
  return ocArrayView<T>(this, 0, _length);
}

template<typename T>
ocConstArrayView<T> ocArray<T>::all() const
{
  return ocConstArrayView<T>(this, 0, _length);
}

template<typename T>
T &ocArray<T>::operator[](size_t index)
{
  oc_assert(index < _length, index, _length);

  return _memory[index];
}

template<typename T>
const T &ocArray<T>::operator[](size_t index) const
{
  oc_assert(index < _length, index, _length);

  return _memory[index];
}

template<typename T>
T &ocArray<T>::first()
{
  oc_assert(0 < _length);

  return _memory[0];
}

template<typename T>
const T &ocArray<T>::first() const
{
  oc_assert(0 < _length);

  return _memory[0];
}

template<typename T>
ocArrayView<T> ocArray<T>::first(size_t length)
{
  oc_assert(length <= _length, length, _length);
  return get_space(0, length);
}

template<typename T>
ocConstArrayView<T> ocArray<T>::first(size_t length) const
{
  oc_assert(length <= _length, length, _length);
  return get_space(0, length);
}

template<typename T>
T &ocArray<T>::last()
{
  oc_assert(0 < _length);

  return _memory[_length - 1];
}

template<typename T>
const T &ocArray<T>::last() const
{
  oc_assert(0 < _length);

  return _memory[_length - 1];
}

template<typename T>
ocArrayView<T> ocArray<T>::last(size_t length)
{
  oc_assert(length <= _length, length, _length);
  return get_space(_length - length, length);
}

template<typename T>
ocConstArrayView<T> ocArray<T>::last(size_t length) const
{
  oc_assert(length <= _length, length, _length);
  return get_space(_length - length, length);
}

template<typename T>
void ocArray<T>::prepend(const T &value)
{
  size_t move_length = _length;
  set_length(_length + 1);
  if (0 < move_length) first(move_length).copy_to(last(move_length));
  first() = value;
}

template<typename T>
void ocArray<T>::append(const T &value)
{
  set_length(_length + 1);
  last() = value;
}

template<typename T>
void ocArray<T>::append(const ocArray &arr)
{
  if (arr.is_empty()) return;
  append_space(arr._length).copy_from(arr.all());
}

template<typename T>
void ocArray<T>::insert(size_t index, const T &value)
{
  oc_assert(index <= _length, index, _length);

  size_t move_length = _length - index;
  set_length(_length + 1);
  if (0 < move_length) get_space(index, move_length).copy_to(get_space(index + 1, move_length));
  _memory[index] = value;
}

template<typename T>
void ocArray<T>::remove_at(size_t index)
{
  oc_assert(index < _length, index, _length);

  size_t end = index + 1;
  size_t move_length = _length - end;
  if (0 < move_length)
  {
    get_space(end, move_length).copy_to(get_space(index, move_length));
  }
  set_length(_length - 1);
}

template<typename T>
void ocArray<T>::remove_first()
{
  oc_assert(0 < _length);

  if (1 < _length)
  {
    get_space(1, _length - 1).copy_to(get_space(0, _length - 1));
  }
  set_length(_length - 1);
}

template<typename T>
void ocArray<T>::remove_last()
{
  oc_assert(0 < _length);

  set_length(_length - 1);
}

template<typename T>
ocArrayIterator<T> ocArray<T>::begin()
{
  return ocArrayIterator<T>(this, 0);
}

template<typename T>
ocConstArrayIterator<T> ocArray<T>::begin() const
{
  return ocConstArrayIterator<T>(this, 0);
}

template<typename T>
ocArrayIterator<T> ocArray<T>::end()
{
  return ocArrayIterator<T>(this, _length);
}

template<typename T>
ocConstArrayIterator<T> ocArray<T>::end() const
{
  return ocConstArrayIterator<T>(this, _length);
}

template<typename T>
size_t ocArray<T>::first_index_of(const T &value)
{
  for (size_t i = 0; i < _length; ++i)
  {
    if (value == _memory[i]) return i;
  }
  return _length;
}

template<typename T>
size_t ocArray<T>::last_index_of(const T &value)
{
  for (size_t i = _length; 0 < i;)
  {
    --i;
    if (value == _memory[i]) return i;
  }
  return _length;
}

template<typename T>
inline ocArrayIterator<T> begin(ocArray<T>& arr)
{
  return arr.begin();
}
template<typename T>
inline ocConstArrayIterator<T> begin(const ocArray<T>& arr)
{
  return arr.begin();
}
template<typename T>
inline ocArrayIterator<T> end(ocArray<T>& arr)
{
  return arr.end();
}
template<typename T>
inline ocConstArrayIterator<T> end(const ocArray<T>& arr)
{
  return arr.end();
}

