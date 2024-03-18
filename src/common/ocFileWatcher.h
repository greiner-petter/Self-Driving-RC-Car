#pragma once

#include <cstddef> // std::byte, size_t

struct ocWatchedFile
{
  int wd;
  bool changed;
};

class ocFileWatcher final
{
private:

  int            _fd          = -1;
  std::byte     *_events_buf  =  nullptr;
  ocWatchedFile *_files       =  nullptr;
  size_t         _max_files   =  0;
  size_t         _num_files   =  0;
  bool           _any_changes = false;

public:
  ocFileWatcher(size_t max_files);
  virtual ~ocFileWatcher();

  ocFileWatcher(const ocFileWatcher &) = delete;
  ocFileWatcher &operator=(const ocFileWatcher &) = delete;

  int get_fd() const { return _fd; }

  const ocWatchedFile *add_file(const char *pathname);
  bool remove_file(const ocWatchedFile *file);

  bool has_changed() const;
  bool has_changed(const ocWatchedFile *file) const;

  bool check_for_changes();
  bool await();
};
