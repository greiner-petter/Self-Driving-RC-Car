#pragma once

#include "ocArray.h"
#include "ocAssert.h"
#include "ocBufferReader.h"
#include "ocBufferWriter.h"

#include <cstddef> // size_t, std::byte

class ocBuffer final
{
private:
    // memory that stores the buffer contents and can grow and shrink
    ocArray<std::byte> _mem;

    // current written bytes in buffer. Gets reset through clear
    size_t _buf_len = 0;

    // limit that the buffer can grow to
    size_t _max_length = 0x7FFFFFFF;

public:
    // constructors & destructor
    [[nodiscard]] ocBuffer() = default;
    [[nodiscard]] explicit ocBuffer(size_t max_length);

    // No copy constructor or assignment due to internally allocated memory.
    ocBuffer(const ocBuffer&) = delete;
    ocBuffer &operator=(const ocBuffer&) = delete;

    // Resets the buffer length to zero, effectively discarding the buffers
    // content, but keeping the memory.
    void clear();

    // Frees the buffer memory and resets length and size.
    void free_buffer();

    // Returns the size limit set by the constructor or the default.
    [[nodiscard]] size_t get_max_length() const;

    // Returns the current size of the internal memory.
    [[nodiscard]] size_t get_capacity() const;

    // Sets the physical size of the buffer.
    void set_capacity(size_t new_length);

    // The length is the amount of meaningful bytes that reside in the buffer.
    // Always more or equal to zero and less or equal to the buffers size.
    [[nodiscard]] size_t get_length() const;
    void set_length(size_t new_len);

    [[nodiscard]] bool is_empty() const;

    // make_space will resize the buffer if necessary, get_space will assert if
    // the length doesn't fit. These functions are effectively bounds-checked
    // range access, e.g. to copy large amounts data into or out of the buffer.
    [[nodiscard]] std::byte *make_space(size_t length);
    [[nodiscard]] std::byte *get_space(size_t length);
    [[nodiscard]] const std::byte *get_space(size_t length) const;

    // similar to methods above, but with an additional offset from the start of
    // the buffer.
    [[nodiscard]] std::byte *make_space(size_t index, size_t length);
    [[nodiscard]] std::byte *get_space(size_t index, size_t length);
    [[nodiscard]] const std::byte *get_space(size_t index, size_t length) const;

    // direct access to a single byte inside the buffer.
    [[nodiscard]] std::byte &operator[](size_t index);
    [[nodiscard]] const std::byte &operator[](size_t index) const;

    // Functions to get a buffer editors to read and write the buffers memory.
    [[nodiscard]] ocBufferWriter clear_and_edit();
    [[nodiscard]] ocBufferWriter edit_from_end();

    [[nodiscard]] ocBufferReader read_from_start() const;
};
