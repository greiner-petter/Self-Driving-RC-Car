#pragma once

#include "ocAssert.h"

#include <cstddef> // size_t, std::byte
#include <cstdint> // uint32_t
#include <type_traits> // std::is_trivial_v

class ocBuffer;

class ocBufferReader final
{
private:
    const ocBuffer *_buffer;
    size_t          _cursor;

public:
    [[nodiscard]] explicit ocBufferReader(const ocBuffer *buffer, size_t cursor = 0);

    [[nodiscard]] size_t available_read_space() const;

    [[nodiscard]] size_t get_pos() const;
    void set_pos(size_t pos);
    void inc_pos(size_t by);
    void dec_pos(size_t by);

    // Read
    // Reads the inquired amount of data from the buffer starting at the cursor
    // location and advances the cursor by the same amount.
    // Defaults may be given to return data while reading off the end of a
    // buffer.

    template<typename T, typename ...Ts>
    [[nodiscard]] bool can_read() const;
    [[nodiscard]] bool can_read(size_t len = 1) const;

    template <typename T>
    [[nodiscard]] T read();
    template <typename T>
    ocBufferReader& read(T *dst);
    [[nodiscard]] const std::byte *read(size_t len);

    template<typename T>
    [[nodiscard]] T read_or_default(const T& default_value);
    template<typename T>
    ocBufferReader& read_or_default(T *dst, const T& default_value);
    ocBufferReader& read(void *dst, size_t len);
    ocBufferReader& read_string(char *dst, uint32_t max_len);


    // Skip
    // Advances the read cursor by the size of the given type or the given length.
    // Read is marked [[nodiscard]] and shouldn't be used for skipping.

    template <typename T, typename ...Ts>
    ocBufferReader& skip();
    ocBufferReader& skip(size_t len);


    // Peek
    // Reads the inquired amount of data from the buffer starting at the cursor
    // location but doesn't advance the cursor.
    // Defaults may be given to return data while reading off the end of a
    // buffer.

    template <typename T>
    [[nodiscard]] T peek();
    template <typename T>
    ocBufferReader& peek(T *dst);
    [[nodiscard]] const std::byte *peek(size_t len);

    template<typename T>
    [[nodiscard]] T peek_or_default(const T& default_value);
    template<typename T>
    ocBufferReader& peek_or_default(T *dst, const T& default_value);
    ocBufferReader& peek(void *dst, size_t len);
    ocBufferReader& peek_string(char *dst, uint32_t max_len);
};

template<typename T, typename ...Ts>
bool ocBufferReader::can_read() const
{
    static_assert((std::is_trivial_v<T> && ... && std::is_trivial_v<Ts>));
    size_t size = (sizeof(T) + ... + sizeof(Ts));
    return size <= available_read_space();
}

template <typename T>
T ocBufferReader::read()
{
    static_assert(std::is_trivial_v<T>);
    oc_assert(can_read<T>());
    T result;
    read(&result, sizeof(T));
    return result;
}

template<typename T>
T ocBufferReader::read_or_default(const T& default_value)
{
    static_assert(std::is_trivial_v<T>);
    if (can_read<T>()) return read<T>();
    return default_value;
}

template <typename T>
ocBufferReader& ocBufferReader::read(T *dst)
{
    static_assert(std::is_trivial_v<T>);
    oc_assert(can_read<T>());
    *dst = read<T>();
    return *this;
}

template<typename T>
ocBufferReader& ocBufferReader::read_or_default(T *dst, const T& default_value)
{
    static_assert(std::is_trivial_v<T>);
    *dst = read_or_default<T>(default_value);
    return *this;
}

template <typename T, typename ...Ts>
ocBufferReader& ocBufferReader::skip()
{
    size_t size = (sizeof(T) + ... + sizeof(Ts));
    oc_assert(can_read(size));
    _cursor += size;
    return *this;
}

template <typename T>
T ocBufferReader::peek()
{
    static_assert(std::is_trivial_v<T>);
    oc_assert(can_read<T>());
    T result;
    peek(&result, sizeof(T));
    return result;
}

template<typename T>
T ocBufferReader::peek_or_default(const T& default_value)
{
    static_assert(std::is_trivial_v<T>);
    if (can_read<T>()) return peek<T>();
    return default_value;
}

template <typename T>
ocBufferReader& ocBufferReader::peek(T *dst)
{
    static_assert(std::is_trivial_v<T>);
    oc_assert(can_read<T>());
    *dst = peek<T>();
    return *this;
}

template<typename T>
ocBufferReader& ocBufferReader::peek_or_default(T *dst, const T& default_value)
{
    static_assert(std::is_trivial_v<T>);
    *dst = peek_or_default<T>(default_value);
    return *this;
}
