#include "ocBufferWriter.h"
#include "ocBuffer.h"

#include <cstring> // memcpy()

ocBufferWriter::ocBufferWriter(ocBuffer *buffer, size_t cursor)
{
    oc_assert(buffer);
    _buffer = buffer;
    _cursor = cursor;
}

size_t ocBufferWriter::available_write_space() const
{
    return _buffer->get_max_length() - _cursor;
}

size_t ocBufferWriter::immediate_write_space() const
{
    return _buffer->get_capacity() - _cursor;
}

size_t ocBufferWriter::get_pos() const
{
    return _cursor;
}
void ocBufferWriter::set_pos(size_t pos)
{
    oc_assert(pos <= _buffer->get_length(), pos, _buffer->get_length());
    _cursor = pos;
}
void ocBufferWriter::inc_pos(size_t by)
{
    oc_assert(_cursor + by <= _buffer->get_length(), _cursor, by, _buffer->get_length());
    _cursor += by;
}
void ocBufferWriter::dec_pos(size_t by)
{
    oc_assert(_cursor - by <= _buffer->get_length(), _cursor, by, _buffer->get_length());
    _cursor -= by;
}

bool ocBufferWriter::can_write(size_t len) const
{
    return (0 < len) && (len <= available_write_space());
}
bool ocBufferWriter::can_write_string(std::string_view src) const
{
    size_t len = src.size();
    return len < 4294967295 && (len + sizeof(uint32_t)) <= available_write_space();
}

ocBufferWriter &ocBufferWriter::write(const void *src, size_t len)
{
    if (0 < len)
    {
        oc_assert(src);
        oc_assert(can_write(len));
        std::byte *dst = _buffer->make_space(_cursor, len);
        memcpy(dst, src, len);
        _cursor += len;
    }
    return *this;
}

ocBufferWriter &ocBufferWriter::write_string(std::string_view src)
{
    size_t len = src.size();
    oc_assert(len < 4294967295, len);
    write<uint32_t>((uint32_t)len);
    if (0 < len) write(src.data(), len);
    return *this;
}

ocBufferWriter &ocBufferWriter::set(const void *src, size_t len)
{
    oc_assert(src);
    oc_assert(can_write(len));
    std::byte *dst = _buffer->make_space(_cursor, len);
    memcpy(dst, src, len);
    return *this;
}

ocBufferWriter &ocBufferWriter::set_string(std::string_view src)
{
    size_t len = src.size();
    size_t cur = _cursor;
    oc_assert(len < 4294967295, len);
    write<uint32_t>((uint32_t)len);
    if (0 < len) write(src.data(), len);
    _cursor = cur;
    return *this;
}

void *ocBufferWriter::get_writable_space(size_t len)
{
    oc_assert(0 < len);
    oc_assert(can_write(len));
    void *result = _buffer->make_space(_cursor, len);
    _cursor += len;
    return result;
}
