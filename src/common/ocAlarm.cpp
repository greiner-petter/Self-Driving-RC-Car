#include "ocAlarm.h"

#include <cstdint>
#include <ctime> // timespec
#include <fcntl.h>
#include <sys/timerfd.h>
#include <unistd.h> // read

ocAlarm::ocAlarm()
{
  _fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
}

ocAlarm::ocAlarm(ocTime period)
{
  _fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  _period = period;
}

ocAlarm::ocAlarm(ocTime period, ocAlarmType type)
{
  _fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  _period = period;
  start(type);
}

ocAlarm::~ocAlarm()
{
  close(_fd);
}

int ocAlarm::get_fd() const { return _fd; }

ocTime ocAlarm::get_period() const { return _period; }

void ocAlarm::set_period(ocTime period) { _period = period; }

void ocAlarm::start(ocAlarmType type)
{
  itimerspec alarm_spec = {};
  alarm_spec.it_value.tv_sec  = _period.get_seconds();
  alarm_spec.it_value.tv_nsec = (_period % ocTime::seconds(1)).get_nanoseconds();
  if (ocAlarmType::Periodic == type)
  {
    alarm_spec.it_interval = alarm_spec.it_value;
  }
  timerfd_settime(_fd, 0, &alarm_spec, nullptr);
  _running = true;
  _type = type;
}

bool ocAlarm::is_expired()
{
  if (!_running) return true;
  uint64_t temp;
  ssize_t len = read(_fd, &temp, 8);
  bool result = (8 == len && 0 < temp);
  if (result && _type == ocAlarmType::Once) _running = false;
  return result;
}

bool ocAlarm::await()
{
  if (!_running) return false;
  int flags = fcntl(_fd, F_GETFL);
  fcntl(_fd, F_SETFL, flags & ~O_NONBLOCK);

  uint64_t temp;
  ssize_t len = read(_fd, &temp, 8);
  bool result = (8 == len && 0 < temp);
  if (result && _type == ocAlarmType::Once) _running = false;

  fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
  return result;
}

void ocAlarm::stop()
{
  itimerspec null_time_spec = {};
  timerfd_settime(_fd, 0, &null_time_spec, nullptr);
  _running = false;
}
