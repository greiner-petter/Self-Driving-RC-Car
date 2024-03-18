#include "ocBufferReader.h"
#include "ocBuffer.h"

#include <cstring> // memcpy()

ocBufferReader::ocBufferReader(const ocBuffer *buffer, size_t cursor)
{
    oc_assert(buffer);
    _buffer = buffer;
    _cursor = cursor;
}

size_t ocBufferReader::available_read_space() const
{
    return _buffer->get_length() - _cursor;
}

size_t ocBufferReader::get_pos() const
{
    return _cursor;
}
void ocBufferReader::set_pos(size_t pos)
{
    oc_assert(pos <= _buffer->get_length(), pos, _buffer->get_length());
    _cursor = pos;
}
void ocBufferReader::inc_pos(size_t by)
{
    oc_assert(_cursor + by <= _buffer->get_length(), _cursor, by, _buffer->get_length());
    _cursor += by;
}
void ocBufferReader::dec_pos(size_t by)
{
    oc_assert(_cursor - by <= _buffer->get_length(), _cursor, by, _buffer->get_length());
    _cursor -= by;
}

bool ocBufferReader::can_read(size_t len) const
{
    return (0 < len) && (len <= available_read_space());
}

const std::byte *ocBufferReader::read(size_t len)
{
    oc_assert(can_read(len));
    const std::byte *src = _buffer->get_space(_cursor, len);
    _cursor += len;
    return src;
}

ocBufferReader &ocBufferReader::read(void *dst, size_t len)
{
    if (0 < len)
    {
        oc_assert(dst);
        oc_assert(can_read(len));
        memcpy(dst, read(len), len);
    }
    return *this;
}

ocBufferReader &ocBufferReader::read_string(char *dst, uint32_t max_len)
{
    oc_assert(dst);
    uint32_t len = read<uint32_t>();
    // We assert < instead of <= here because len doesn't include the '\0'.
    oc_assert(len < max_len, len, max_len);
    read((std::byte*) dst, len);
    dst[len] = '\0';
    return *this;
}

ocBufferReader& ocBufferReader::skip(size_t len)
{
    oc_assert(can_read(len));
    _cursor += len;
    return *this;
}

const std::byte *ocBufferReader::peek(size_t len)
{
    oc_assert(can_read(len));
    const std::byte *src = _buffer->get_space(_cursor, len);
    return src;
}

ocBufferReader &ocBufferReader::peek(void *dst, size_t len)
{
    oc_assert(dst);
    oc_assert(can_read(len));
    memcpy(dst, peek(len), len);
    return *this;
}

ocBufferReader &ocBufferReader::peek_string(char *dst, uint32_t max_len)
{
    oc_assert(dst);
    uint32_t len = peek<uint32_t>();
    // We assert < instead of <= here because len doesn't include the '\0'.
    oc_assert(len < max_len, len, max_len);
    peek((std::byte*) dst, len);
    dst[len] = '\0';
    return *this;
}
