#pragma once

#include "ocBuffer.h"

#include <cstdint> // uint64_t

// This profiler doesn't handle multi-threading yet. Which shouldn't be a problem because
// we usually have just a single thread and multiple processes. But if you need profiling
// for multiple threads, here are some things you need to do:
// - Add a thread ID to the ocTimingEvent.
// - Put an atomic increment in the c file for the next_event_index variable.

#define TIMING_EVENT_STORE_SIZE 1024

// These Macros look pretty messy, that's due to two reasons:
// 1. varargs like our USERSTRING are not consistently implemented across compilers, so we need
// to hack around the fact that for example gcc will not remove the comma before the __VA_ARGS__.
// 2. evaluation and concatenation of __COUNTER__, __FILE_ etc also needs multiple nested macros
// Start looking at END_TIMED_BLOCK, it's the simplest since it doesn't have the vararg stuff.

// This macro should only be placed at the very top of a {} block. It automatically measures the
// time at the start and end of the block.
#define TIMED_BLOCK(...) TIMED_BLOCK_(__COUNTER__, ##__VA_ARGS__)
#define TIMED_BLOCK_(COUNT, ...) \
  const static ocTimingSite CONCAT(_timing_site, COUNT){"" __VA_ARGS__, __FILE__, __PRETTY_FUNCTION__, __LINE__}; \
  ocTimedBlock CONCAT(_timer, COUNT)(CONCAT(_timing_site, COUNT).index)


// These macros can be used if the code you want to measure doesn't line up with any block.
// For example if you just want to measure a single call.
#define BEGIN_TIMED_BLOCK(...) do { \
  const static ocTimingSite _timing_site{"" __VA_ARGS__, __FILE__, __PRETTY_FUNCTION__, __LINE__}; \
  _log_timing_event(_timing_site.index, ocTimingEvent_BeginBlock); \
} while (0)

// This macro ends the previous timing block and opens a new one. It is also compatible with the TIMED_BLOCK
#define NEXT_TIMED_BLOCK(...) do { \
  const static ocTimingSite _timing_site{"" __VA_ARGS__, __FILE__, __PRETTY_FUNCTION__, __LINE__}; \
  _log_timing_event(_timing_site.index, ocTimingEvent_EndBeginBlock); \
} while (0)

#define END_TIMED_BLOCK() do { \
  const static ocTimingSite _timing_site{"", __FILE__, __PRETTY_FUNCTION__, __LINE__}; \
  _log_timing_event(_timing_site.index, ocTimingEvent_EndBlock); \
} while (0)


// This macro is needed to correctly concatenate the expanded __COUNTER__ macro to something else.
// Without it the compiler will just paste the literal string "__COUNTER__" instead of a number...
#define CONCAT(a, b) a ## b

struct ocTimingSite;
extern ocTimingSite *next_timing_site;

// A Timing Site describes a place in the source code where time was measured. For a single site
// there can be many Timing Events.
struct ocTimingSite final
{
  const ocTimingSite *next; // Timing sites are stored as static variables in their respective functions. To access them all, we need this linked list.
  const char         *userstring;
  const char         *filename;
  const char         *functionname;
  uint16_t            linenumber;
  uint16_t            index;

  ocTimingSite(
    const char *me,
    const char *fi,
    const char *fu,
    uint16_t    li)
  {
    next         = next_timing_site;
    userstring   = me;
    filename     = fi;
    functionname = fu;
    linenumber   = li;
    index        = (next ? next->index + 1 : 0);
    next_timing_site = this;
  }
};

enum ocTimingEventType: uint8_t
{
  ocTimingEvent_Point         = 0,
  ocTimingEvent_BeginBlock    = 1,
  ocTimingEvent_EndBlock      = 2,
  ocTimingEvent_EndBeginBlock = 3
};

struct ocTimingEvent
{
  // All times in this struct are 32 bit, so we can fit about 4s worth of nanoseconds,
  // which means that the time will overflow quite often. This should not be a problem
  // however, because no timespan in our system should be that long. So we have at most
  // one overflow within a timespan, which can be detected when the end is earlier than
  // the start, and fixed by adding UINT32_MAX to the end.
  // None of the times are meaningful on their own. To get useful data, this event has to
  // be compared to another one, and the differences in the time values have meaning.

  // Monotonic time in nanoseconds since some time in the past. Modulo 2^32.
  uint32_t real_time;

  // CPU time spent since the start of the process. Modulo 2^32. Could be "faster" than the
  // real_time, if we use threads. Otherwise it will be "slower" because the process doesn't
  // execute 100% of the time.
  uint32_t cpu_time;

  // CPU time spent in the kernel on behalf of this process since its start. Modulo 2^32.
  // Currently not implemented, because the times(...) call that could provide this number
  // has a default resolution of 10ms, which is useless for our purpose here.
  //uint32_t sys_time;

  // Index in the timing_site_store of the site where the event was recorded.
  uint16_t site_index;

  // Indicator if this marks the beginning or end of a block. Or just a single point in time.
  ocTimingEventType type;

  // These bytes are there anyways because of padding. So let's make them usable!
  uint8_t stuff;
};

uint16_t _register_timing_site(
  const char *userstring,
  const char *filename,
  const char *functionname,
  uint16_t linenumber);

void _log_timing_event(uint16_t site_index, ocTimingEventType type);
void clear_timing_events();
uint32_t timing_site_count();
uint32_t timing_event_count();
uint32_t write_timing_sites_to_buffer(ocBuffer *buffer);
uint32_t write_timing_events_to_buffer(ocBuffer *buffer);

// C++ magic that uses a constructor-destructor pair to inject code at the end of a block.
struct ocTimedBlock final
{
  ocTimedBlock(uint16_t site_index)
  {
    _log_timing_event(site_index, ocTimingEvent_BeginBlock);
  }
  ~ocTimedBlock()
  {
    _log_timing_event(0, ocTimingEvent_EndBlock);
  }
};
