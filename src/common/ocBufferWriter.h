#pragma once

#include "ocAssert.h"

#include <cstddef> // size_t, std::byte
#include <string_view> // std::string_view
#include <type_traits> // std::is_trivial_v

class ocBuffer;

class ocBufferWriter final
{
private:
    ocBuffer *_buffer;
    size_t    _cursor;

public:
    [[nodiscard]] explicit ocBufferWriter(ocBuffer *buffer, size_t cursor = 0);

    // Returns the maximum possible space to write, up to the buffers
    // max size.
    [[nodiscard]] size_t available_write_space() const;

    // Returns the space that can be written to immediately without allocating
    // new space.
    [[nodiscard]] size_t immediate_write_space() const;

    [[nodiscard]] size_t get_pos() const;
    void set_pos(size_t pos);
    void inc_pos(size_t by);
    void dec_pos(size_t by);

    // Write
    // Places the given data into the buffer at the cursor location and
    // advances the cursor by the size of the data.

    template<typename T, typename ...Ts>
    [[nodiscard]] bool can_write() const;
    [[nodiscard]] bool can_write(size_t len = 1) const;
    [[nodiscard]] bool can_write_string(std::string_view str) const;

    template<typename T>
    ocBufferWriter& write(const T& value);
    ocBufferWriter& write(const void *src, size_t len);

    ocBufferWriter& write_string(std::string_view src);

    // Set
    // Places the given data into the buffer at the cursor location but doesn't
    // move the cursor.

    template<typename T>
    ocBufferWriter& set(const T& value);
    ocBufferWriter& set(const void *src, size_t len);

    ocBufferWriter& set_string(std::string_view src);

    // Get writable space
    // Asserts that enough space is available, advances the cursor and
    // returns a pointer to the space.

    [[nodiscard]] void *get_writable_space(size_t len);
};

template<typename T, typename ...Ts>
bool ocBufferWriter::can_write() const
{
    static_assert((std::is_trivial_v<T> && ... && std::is_trivial_v<Ts>));
    size_t size = (sizeof(T) + ... + sizeof(Ts));
    return size <= available_write_space();
}

template<typename T>
ocBufferWriter& ocBufferWriter::write(const T& value)
{
    static_assert(std::is_trivial_v<T>);
    oc_assert(can_write<T>());
    write(&value, sizeof(T));
    return *this;
}

template<typename T>
ocBufferWriter& ocBufferWriter::set(const T& value)
{
    static_assert(std::is_trivial_v<T>);
    oc_assert(can_write<T>());
    set(&value, sizeof(T));
    return *this;
}
