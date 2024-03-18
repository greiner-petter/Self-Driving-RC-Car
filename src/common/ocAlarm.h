#pragma once

#include "ocTime.h"

enum class ocAlarmType
{
  Once,
  Periodic
};

class ocAlarm final
{
private:
  int         _fd      = -1;
  bool        _running = false;
  ocTime      _period  = {};
  ocAlarmType _type    = ocAlarmType::Once;

public:

  ocAlarm();
  explicit ocAlarm(ocTime period);
  ocAlarm(ocTime period, ocAlarmType type);
  ~ocAlarm();

  ocAlarm(const ocAlarm &) = delete;
  ocAlarm &operator=(const ocAlarm &) = delete;

  int get_fd() const;

  ocTime get_period() const;
  void set_period(ocTime time);

  void start(ocAlarmType type);

  bool is_expired();

  bool await();

  void stop();
};
