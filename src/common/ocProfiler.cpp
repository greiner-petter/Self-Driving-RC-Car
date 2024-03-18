#include "ocAssert.h"
#include "ocProfiler.h"

#include <cstring> // memmove, strlen
#include <cstdlib> // size_t
#include <ctime> // clock_gettime, timespec

static ocTimingEvent timing_event_store[TIMING_EVENT_STORE_SIZE] = {};

// pointer to the next place to write a new event or site
ocTimingSite *next_timing_site  = nullptr;
static ocTimingEvent *next_timing_event = &timing_event_store[0];

// pointers to the events and sites that have not been transmitted
// over ipc yet. Should be reset after transmission.
static ocTimingSite  *last_written_timing_site  = nullptr;

void _log_timing_event(uint16_t site_index, ocTimingEventType event_type)
{
  timespec real_ts = {};
  timespec cpu_ts = {};
  clock_gettime(CLOCK_MONOTONIC, &real_ts);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_ts);

  int64_t real_time_ns = 0;
  real_time_ns += real_ts.tv_nsec;
  real_time_ns += real_ts.tv_sec * 1000000000L;
  int64_t cpu_time_ns = 0;
  cpu_time_ns += cpu_ts.tv_nsec;
  cpu_time_ns += cpu_ts.tv_sec * 1000000000L;

  oc_assert(next_timing_event < &timing_event_store[TIMING_EVENT_STORE_SIZE]);
  ocTimingEvent *event = next_timing_event++;
  event->real_time  = (uint32_t)real_time_ns;
  event->cpu_time   = (uint32_t)cpu_time_ns;
  event->type       = event_type;
  event->site_index = site_index;
}

void clear_timing_events()
{
  next_timing_event = &timing_event_store[0];
}

uint32_t timing_site_count()
{
  if (next_timing_site) return (uint32_t)next_timing_site->index + 1;
  return 0;
}

uint32_t timing_event_count()
{
  return (uint32_t) (next_timing_event - &timing_event_store[0]);
}

uint32_t write_timing_sites_to_buffer(ocBuffer *buffer)
{
  uint32_t counter = 0;
  auto editor = buffer->clear_and_edit();
  const ocTimingSite *site = next_timing_site;
  auto str_len = [](const char *s){ return s ? strlen(s) : 0; };
  while (last_written_timing_site != site)
  {
    if (!editor.can_write(str_len(site->userstring)
                        + str_len(site->filename)
                        + str_len(site->functionname)
                        + 3 * sizeof(uint32_t)
                        + 2 * sizeof(uint16_t)))
    {
      break;
    }
    editor.write<uint16_t>(site->index);
    editor.write<uint16_t>(site->linenumber);
    editor.write_string(site->userstring);
    editor.write_string(site->filename);
    editor.write_string(site->functionname);
    ++counter;
    site = site->next;
  }
  last_written_timing_site = next_timing_site;
  return counter;
}

uint32_t write_timing_events_to_buffer(ocBuffer *buffer)
{
  uint32_t counter = 0;
  ocTimingEvent *event = &timing_event_store[0];
  auto editor = buffer->clear_and_edit();
  while (event != next_timing_event && editor.can_write<ocTimingEvent>())
  {
    editor.write<ocTimingEvent>(*event++);
    ++counter;
  }
  if (event != next_timing_event)
  {
    size_t remaining = (size_t)(next_timing_event - event);
    memmove(&timing_event_store[0], event, remaining * sizeof(ocTimingEvent));
    next_timing_event = &timing_event_store[remaining];
  }
  else
  {
    next_timing_event = &timing_event_store[0];
  }
  return counter;
}
