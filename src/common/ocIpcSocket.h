#pragma once

#include "ocPacket.h"
#include "ocTypes.h"

#include <cstdint> // int32_t
#include <type_traits> // std::is_trivial_v

class ocIpcSocket final
{
private:
    // file descriptor of the socket
    int32_t _socket_fd = -1;

    // queues to store the data for the read and send syscalls.
    ocBuffer _send_buffer;
    ocBuffer _read_buffer;

    // counters to make sure no packets were lost. Unsigned integers will just
    // wrap back to 0 on overflow.
    uint8_t _send_counter = 0;
    uint8_t _read_counter = 0;

    /**
     * Tries to recv the requested amount of data into the buffer. If not enough
     * data was received, and blocking is false, the amount that was received
     * is copied into the _read_buffer.
     */
    int32_t _read(void *buffer, size_t length, bool blocking);

public:

    /**
     * Constructs an empty IPC socket. A socket FD must be provided via set_fd before any sending or
     * receiving can happen.
     */
    ocIpcSocket();

    /**
     * There is no copy constructor or assignment operator, otherwise we would
     * need to track who/how many own the internal file descriptor. That complication
     * doesn't seem worth it when you can just pass pointers to one instance around.
     */
    ocIpcSocket(const ocIpcSocket&) = delete;
    ocIpcSocket &operator=(const ocIpcSocket &) = delete;

    /**
     * Closes its socket if it has one.
     */
    ~ocIpcSocket();

    /**
     * Returns the file descriptor of the socket. Useful for epoll for example.
     */
    int32_t get_fd() const;

    /**
     * Sets the internal socket to the given file descriptor. The old internal socket is returned (-1 if there was none before).
     */
    int32_t set_fd(int32_t fd);

    /**
     * Reads a single packet if one is available and returns its size.
     * If no packet is available, the method will block or return 0 depending on the blocking parameter.
     * If an error occurred a negative error code is returned.
     */
    int32_t read_packet(ocPacket &packet, bool blocking = true);

    /**
     * Sends the given packet over the socket and returns the number of bytes sent.
     * If the send buffer is full, the method will block or return 0 depending on the blocking parameter.
     * If an error occurred, a negative error code is returned.
     */
    int32_t send_packet(const ocPacket &packet, bool blocking = true);

    int32_t send(ocMemberId sender_id, ocMessageId message_id, const void *data, size_t length, bool blocking = true);
    int32_t send(ocMessageId message_id, bool blocking = true);

    template<typename T>
    int32_t send(ocMessageId message_id, const T& data, bool blocking = true)
    {
        static_assert(!std::is_pointer_v<T>);
        static_assert(std::is_trivial_v<T>);
        return send(ocMemberId::None, message_id, (const void *)&data, sizeof(T), blocking);
    }
};
