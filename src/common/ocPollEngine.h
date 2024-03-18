#pragma once

#include "ocTime.h"

#include <sys/epoll.h> // epoll_event

enum class ocPollReport
{
    Success,
    Failure
};

enum class ocPollDirection
{
    Read       = 1,
    Write      = 2,
    Read_Write = 3
};

class ocPollEngine final
{
private:
    int32_t _ev_fd             = -1;
    uint32_t _max_events       =  0;
    int32_t _last_event_count  =  0;
    epoll_event* _event_buffer =  nullptr;
    uint32_t _fd_count         =  0;

public:
    /**
     * Creates a new epoll instance with a buffer of the given size.
     * The buffer is filled by the poll() function and can only hold
     * as many entries as set by the max_size parameter.
     */
    ocPollEngine(uint32_t max_size);
    ~ocPollEngine();

    /**
     * This type can't be copied, because it handles a resource that
     * must be released by the destructor. Copies could try to release
     * it early or twice, which would be bad.
     */
    ocPollEngine(const ocPollEngine&) = delete;
    void operator=(const ocPollEngine&) = delete;

    /**
     * Returns the file descriptor of this epoll instance. It can be
     * Given to other epoll instances to create a hierarchy.
     */
    int32_t get_fd() const { return _ev_fd; }

    /**
     * Adds a new file descriptor to the watchlist.
     */
    ocPollReport add_fd(int fd, ocPollDirection direction = ocPollDirection::Read);

    /**
     * Removes a file descriptors from the watchlist.
     */
    ocPollReport delete_fd(int fd);

    /**
     * Calling this function will block until a change happens to any of
     * the file descriptors on the watchlist. A timeout can be specified
     * to limit the blocking time. The values ocTime::forever and ocTime::null
     * will work as expected: blocking until an event arrives or returning
     * immediately respectively. After this function returned, it will have
     * refreshed the internal event buffer. The function was_triggered() can be
     * used to check if  a given file descriptor is listed in that buffer.
     */
    int await(ocTime timeout = ocTime::forever());

    /**
     * Equivalent to calling await(ocTime::null()).
     */
    int update() { return await(ocTime::null()); }

    /**
     * This function returns weather or not a given file descriptor had
     * changes when the poll() function was last called.
     */
    bool was_triggered(int fd) const;
};
