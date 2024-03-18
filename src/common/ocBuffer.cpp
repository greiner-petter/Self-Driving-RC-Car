#include "ocBuffer.h"

ocBuffer::ocBuffer(size_t max_length)
{
    _max_length = max_length;
}

size_t ocBuffer::get_max_length() const
{
    return _max_length;
}

void ocBuffer::clear()
{
    _buf_len = 0;
}

void ocBuffer::free_buffer()
{
    _mem.clear();
}

void ocBuffer::set_capacity(size_t new_length)
{
    oc_assert(new_length <= _max_length, new_length, _max_length);
    _mem.set_length(new_length);
}

size_t ocBuffer::get_capacity() const
{
    return _mem.get_length();
}

size_t ocBuffer::get_length() const
{
    return _buf_len;
}

void ocBuffer::set_length(size_t new_len)
{
    if (get_capacity() < new_len) set_capacity(new_len);
    _buf_len = new_len;
}

bool ocBuffer::is_empty() const
{
    return 0 == _buf_len;
}

std::byte *ocBuffer::get_space(size_t length)
{
    if (_buf_len < length) return nullptr;
    if (0 == length) return nullptr;
    return (std::byte *)_mem.get_space(0, length);
}

const std::byte *ocBuffer::get_space(size_t length) const
{
    if (_buf_len < length) return nullptr;
    if (0 == length) return nullptr;
    return (const std::byte *)_mem.get_space(0, length);
}

std::byte *ocBuffer::make_space(size_t length)
{
    if (0 == length) return nullptr;
    if (_buf_len < length) _buf_len = length;
    if (get_capacity() < length) set_capacity(length);
    return (std::byte *)_mem.get_space(0, length);
}

std::byte *ocBuffer::get_space(size_t index, size_t length)
{
    if (_buf_len < index + length) return nullptr;
    if (0 == length) return nullptr;
    return (std::byte *)_mem.get_space(index, length);
}

const std::byte *ocBuffer::get_space(size_t index, size_t length) const
{
    if (_buf_len < index + length) return nullptr;
    if (0 == length) return nullptr;
    return (const std::byte *)_mem.get_space(index, length);
}

std::byte *ocBuffer::make_space(size_t index, size_t length)
{
    if (0 == length) return nullptr;
    if (_buf_len < index + length) _buf_len = index + length;
    if (get_capacity() < index + length) set_capacity(index + length);
    return (std::byte *)_mem.get_space(index, length);
}

std::byte &ocBuffer::operator[](size_t index)
{
    oc_assert(index < _buf_len, index, _buf_len);
    return _mem[index];
}

const std::byte &ocBuffer::operator[](size_t index) const
{
    oc_assert(index < _buf_len, index, _buf_len);
    return _mem[index];
}

ocBufferWriter ocBuffer::clear_and_edit()
{
    clear();
    return ocBufferWriter(this);
}
ocBufferWriter ocBuffer::edit_from_end()
{
    return ocBufferWriter(this, _buf_len);
}

ocBufferReader ocBuffer::read_from_start() const
{
    return ocBufferReader(this, 0);
}
