#include "ocPollEngine.h"

#include "ocAssert.h"

#include <unistd.h> // close()

ocPollEngine::~ocPollEngine()
{
    if (0 <= _ev_fd) close(_ev_fd);
    delete[] _event_buffer;
}

ocPollEngine::ocPollEngine(uint32_t max_size)
{
    _ev_fd = epoll_create((int)max_size);
    _event_buffer = new epoll_event[max_size];
    _max_events = max_size;
}

ocPollReport ocPollEngine::add_fd(int fd, ocPollDirection direction)
{
    oc_assert(_fd_count != _max_events);
    _fd_count++;
    epoll_event ev;
    ev.data.fd = fd;
    switch (direction)
    {
        case ocPollDirection::Read:       ev.events = EPOLLIN; break;
        case ocPollDirection::Write:      ev.events = EPOLLOUT; break;
        case ocPollDirection::Read_Write: ev.events = EPOLLIN | EPOLLOUT; break;
    }
    int res = epoll_ctl(_ev_fd, EPOLL_CTL_ADD, fd, &ev);
    if (-1 == res) return ocPollReport::Failure;
    return ocPollReport::Success;
}

ocPollReport ocPollEngine::delete_fd(int fd)
{
    oc_assert(_fd_count);
    _fd_count--;
    epoll_event ev;
    int res = epoll_ctl(_ev_fd, EPOLL_CTL_DEL, fd, &ev);
    if (!res)
        return ocPollReport::Success;
    return ocPollReport::Failure;
}

int ocPollEngine::await(ocTime timeout)
{
    int t = (int)timeout.get_milliseconds();
    // special case for ocTime::forever(): while it has the value -1,
    // that is -1ns. We're dealing with ms here and forever would be
    // rounded to 0. That's why this if is necessary:
    if (ocTime::forever() == timeout) t = -1;
    _last_event_count = epoll_wait(_ev_fd, _event_buffer, (int)_max_events, t);
    return _last_event_count;
}

bool ocPollEngine::was_triggered(int fd) const
{
    for (int32_t i = 0; i < _last_event_count; ++i)
    {
        if (_event_buffer[i].data.fd == fd) return true;
    }
    return false;
}
