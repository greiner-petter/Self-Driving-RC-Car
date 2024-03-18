#include "ocFileWatcher.h"
#include "ocAssert.h"
#include "ocTime.h"

#include <cstdlib> // malloc
#include <cstring> // memset
#include <sys/inotify.h>
#include <unistd.h> // fcntl, read
#include <fcntl.h> // fcntl
#include <limits.h> // NAME_MAX

#define inotify_event_size (sizeof(inotify_event) + NAME_MAX + 1)

ocFileWatcher::ocFileWatcher(size_t max_files)
{
  _fd = inotify_init1(IN_NONBLOCK);
  _max_files = max_files;

  _events_buf = (std::byte *)malloc(max_files * inotify_event_size);
  memset(_events_buf, 0, max_files * inotify_event_size);

  _files = (ocWatchedFile *)malloc(max_files * sizeof(ocWatchedFile));
  memset(_files, 0, max_files * sizeof(ocWatchedFile));
}

ocFileWatcher::~ocFileWatcher()
{
  if (0 <= _fd) close(_fd);
  free(_events_buf);
  free(_files);
}

const ocWatchedFile *ocFileWatcher::add_file(const char *pathname)
{
  oc_assert(nullptr != pathname);
  oc_assert(_num_files != _max_files, _num_files, _max_files);

  int result = inotify_add_watch(_fd, pathname, IN_MODIFY);
  if (result < 0) return nullptr;

  size_t insert_pos = _max_files;
  for (size_t i = 0; i < _max_files; ++i)
  {
    // pick the first free spot in the array
    if (i < insert_pos && 0 == _files[i].wd)
    {
      insert_pos = i;
    }
    // but if the file already exists in the array, we re-use it
    if (result == _files[i].wd)
    {
      insert_pos = i;
    }
  }

  if (insert_pos < _max_files) // should always be true. We assert that there is space above.
  {
    _num_files++;
    _files[insert_pos].wd = result;
    return &_files[insert_pos];
  }
  return nullptr; // this return should never be reached
}

bool ocFileWatcher::remove_file(const ocWatchedFile *file)
{
  oc_assert(nullptr != file);
  int result = inotify_rm_watch(_fd, file->wd);
  if (result < 0) return false;

  for (size_t i = 0; i < _max_files; ++i)
  {
    if (file->wd == _files[i].wd)
    {
      _num_files--;
      _files[i].wd = 0;
      return true;
    }
  }
  return false;
}

bool ocFileWatcher::has_changed() const
{
  return _any_changes;
}

bool ocFileWatcher::has_changed(const ocWatchedFile *file) const
{
  for (size_t i = 0; i < _max_files; ++i)
  {
    if (_files[i].wd == file->wd)
    {
      return _files[i].changed;
    }
  }
  return false;
}

bool ocFileWatcher::check_for_changes()
{
  for (size_t i = 0; i < _max_files; ++i)
  {
    _files[i].changed = false;
  }
  _any_changes = false;
  while (true)
  {
    ssize_t result = read(_fd, _events_buf, _max_files * inotify_event_size);
    if (result < 0) return _any_changes;

    _any_changes = true;

    const std::byte *end    = _events_buf + result;
    const std::byte *cursor = _events_buf;
    while (cursor != end)
    {
      const inotify_event *event = (const inotify_event *)cursor;

      for (size_t i = 0; i < _max_files; ++i)
      {
        if (_files[i].wd == event->wd)
        {
          _files[i].changed = true;
          break;
        }
      }

      cursor += sizeof(inotify_event);
      cursor += event->len;
    }
  }
}

bool ocFileWatcher::await()
{
  int flags = fcntl(_fd, F_GETFL);
  fcntl(_fd, F_SETFL, flags & ~O_NONBLOCK);

  bool result = check_for_changes();

  fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
  return result;
}
