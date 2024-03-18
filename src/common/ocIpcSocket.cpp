#include "ocAssert.h"
#include "ocIpcSocket.h"

#include <sys/socket.h>
#include <unistd.h> // close()

#include <cerrno> // errno
#include <cstring> // memcpy, memmove

struct ocPacketHeader
{
    ocMessageId message_id;
    ocMemberId  sender_id;
    uint32_t counter_and_length; // (length << 8) | counter
};

ocIpcSocket::ocIpcSocket() :
    _send_buffer((1 << 24) - 1 + sizeof(ocPacketHeader)),
    _read_buffer((1 << 24) - 1 + sizeof(ocPacketHeader))
{}

ocIpcSocket::~ocIpcSocket()
{
    if (0 <= _socket_fd) close(_socket_fd);
}

int32_t ocIpcSocket::get_fd() const
{
    return _socket_fd;
}

int32_t ocIpcSocket::set_fd(int32_t fd)
{
    int32_t old_fd = _socket_fd;
    _socket_fd = fd;
    return old_fd;
}

int32_t ocIpcSocket::send(
    ocMemberId sender_id,
    ocMessageId message_id,
    const void *data,
    size_t length,
    bool blocking)
{
    oc_assert(-1 != _socket_fd);
    oc_assert(length < (1 << 24), length);

    auto writer = _send_buffer.clear_and_edit();

    writer.write<ocPacketHeader>({
        .message_id = message_id,
        .sender_id = sender_id,
        .counter_and_length = (uint32_t)(length << 8) | _send_counter
    });

    if (0 < length) writer.write(data, length);
    size_t packet_length = _send_buffer.get_length();

    if (blocking)
    {
        auto reader = _send_buffer.read_from_start();
        while (reader.can_read())
        {
            size_t len = reader.available_read_space();
            ssize_t result = ::send(_socket_fd, reader.peek(len), len, 0);
            if (result <= 0)
            {
                return -1;
            }
            reader.inc_pos((size_t)result);
        }
    }
    else
    {
        const void *space = _send_buffer.get_space(packet_length);
        ssize_t result = ::send(_socket_fd, space, packet_length, MSG_DONTWAIT);
        if (result <= 0)
        {
            if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return 0;
            }
            return -1;
        }
        oc_assert((size_t)result == packet_length, result, packet_length);
    }

    _send_counter++;
    return (int32_t)packet_length;
}

int32_t ocIpcSocket::send(ocMessageId message_id, bool blocking)
{
    return send(ocMemberId::None, message_id, nullptr, 0, blocking);
}

int32_t ocIpcSocket::send_packet(const ocPacket &packet, bool blocking)
{
    ocMemberId sender_id = packet.get_sender();
    ocMessageId message_id = packet.get_message_id();
    uint32_t length = packet.get_length();
    void *data = nullptr;
    if (0 < length) data = (void *)packet.get_payload()->get_space(length);
    return send(sender_id, message_id, data, length, blocking);
}

int32_t ocIpcSocket::_read(void *buffer, size_t length, bool blocking)
{
    int flags = 0;
    if (!blocking) flags |= MSG_DONTWAIT;
    // First try the fastest thing: read the requested amount of data into the
    // target buffer. If that doesn't work, we fall back onto the _read_buffer.
    if (_read_buffer.is_empty())
    {
        ssize_t result = ::recv(_socket_fd, buffer, length, flags);
        if (result <= 0)
        {
            if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return 0; // 0 = would have to block
            }
            return -1; // -1 == error
        }
        if (length == (size_t)result)
        {
            return (int32_t)length;
        }
        // If not enough data was read by the recv, we pull it back out
        // from the target and append it to the _read_buffer.
        // (technically, we don't "append", because the buffer is empty,
        // but this matches the stuff happening below.)
        _read_buffer.edit_from_end().write(buffer, (size_t)result);
    }
    // At this point either the read_buffer isn't empty, of the recv above
    // didn't return enough data.
    size_t bytes_read = _read_buffer.get_length();
    while (bytes_read < length)
    {
        size_t len = length - bytes_read;
        ssize_t result = ::recv(_socket_fd, _read_buffer.make_space(bytes_read, len), len, flags);
        if (result <= 0)
        {
            if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return 0; // 0 = would have to block
            }
            return -1; // -1 == error
        }
        bytes_read += (size_t)result;
    }
    // If we haven't returned so far, we must have received all the bytes we asked for
    // and collected them into the read_buffer. Now we shovel them over to the real
    // target and clear the buffer for the next packet.
    _read_buffer.read_from_start().read(buffer, length);
    _read_buffer.clear();
    return (int32_t)length;
}

int32_t ocIpcSocket::read_packet(ocPacket &packet, bool blocking)
{
    oc_assert(-1 != _socket_fd);

    ocPacketHeader header = {};
    int32_t result;

    result = _read(&header, sizeof(ocPacketHeader), blocking);
    if (result <= 0) return result;

    uint8_t counter = (uint8_t)header.counter_and_length;
    size_t length = header.counter_and_length >> 8;
    oc_assert(_read_counter == counter, _read_counter, counter);

    packet.clear();

    if (0 < length)
    {
        result = _read(packet.get_payload()->make_space(length), length, blocking);
        if (result <= 0)
        {
            if (result < 0) return result;
            // if result is 0 here, that means we're non-blocking and we already
            // have received the header. So we need to stuff the header back in
            // the front of the _read_buffer and then return.
            size_t len = _send_buffer.get_length();
            void *dst = _send_buffer.make_space(sizeof(ocPacketHeader), len);
            void *src = _send_buffer.get_space(len);
            memmove(dst, src, len);
            memcpy(src, &header, sizeof(ocPacketHeader));
            return 0;
        }
    }

    _read_counter++;
    packet.set_message_id(header.message_id);
    packet.set_sender(header.sender_id);
    return (int32_t)(sizeof(ocPacketHeader) + length);
}
