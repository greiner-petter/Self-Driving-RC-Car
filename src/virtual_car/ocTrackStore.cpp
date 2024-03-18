#include "ocTrackStore.h"

#include "../common/ocAssert.h"
#include "../common/ocBuffer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // O_RDONLY
#include <unistd.h>

#include <algorithm> // std::min

const char *to_string(ocTrackStoreReport report)
{
  switch (report)
  {
    case ocTrackStoreReport::Success: return "Success";
    case ocTrackStoreReport::File_Not_Accessible: return "File_Not_Accessible";
    case ocTrackStoreReport::File_Contains_Garbage: return "File_Contains_Garbage";
  }
  return "<unknown>";
}

ocTrackStoreReport load_track(const char *filename, ocSimulationWorld& track)
{
  ocTrackStoreReport result = ocTrackStoreReport::Success;
  ocBuffer file_buffer;

  // Read the entire file into a buffer.
  {
    int file_handle = ::open(filename, O_RDONLY);
    if (file_handle < 0) 
    {
      return ocTrackStoreReport::File_Not_Accessible;
    }
    struct stat file_stats = {};
    int fstat_result = ::fstat(file_handle, &file_stats);
    if (fstat_result < 0)
    {
      ::close(file_handle);
      return ocTrackStoreReport::File_Not_Accessible;
    }
    size_t file_size = (size_t)file_stats.st_size;

    char read_buffer[4096];
    size_t amount_read = 0;
    auto writer = file_buffer.clear_and_edit();
    while (amount_read < file_size)
    {
      size_t remaining = std::min(4096UL, file_size - amount_read);
      ssize_t read_len = ::read(file_handle, read_buffer, remaining);
      if (read_len < 0)
      {
        ::close(file_handle);
        return ocTrackStoreReport::File_Not_Accessible;
      }
      if (0 == read_len)
      {
        file_size = amount_read;
        break;
      }
      writer.write(read_buffer, (size_t)read_len);
      amount_read += (size_t)read_len;
    }
    ::close(file_handle);
  }

  // TODO: assert minimum file size

  // Parse world from buffer
  {
    auto reader = file_buffer.read_from_start();

    track.world_width  = reader.read<int>(); // TODO: check mot negative
    track.world_height = reader.read<int>(); // TODO: check mot negative

    auto tiles_size = track.world_width * track.world_height;
    track.world = (ocRoadTile *)malloc(size_t(tiles_size) * sizeof(ocRoadTile));

    reader.read((char *)track.world, size_t(tiles_size) * sizeof(ocRoadTile));

    oc_assert(!reader.can_read());
    // TODO: assert that all bytes were used
  }

  return result;
}

ocTrackStoreReport save_track(const char *filename, const ocSimulationWorld& track)
{
  ocBuffer file_buffer;

  // Write world to buffer
  {
    auto writer = file_buffer.clear_and_edit();
    writer.write<int>(track.world_width);
    writer.write<int>(track.world_height);

    auto tiles_size = track.world_width * track.world_height;
    writer.write((char *)track.world, size_t(tiles_size) * sizeof(ocRoadTile));
  }

  // Write buffer to file.
  {
    int file_handle = ::open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (file_handle < 0) 
    {
      return ocTrackStoreReport::File_Not_Accessible;
    }

    size_t written = 0;
    while (written < file_buffer.get_length())
    {
      auto to_write = file_buffer.get_length() - written;
      auto result = ::write(file_handle, file_buffer.get_space(written, to_write), to_write);
      if (result <= 0)
      {
        ::close(file_handle);
        return ocTrackStoreReport::File_Not_Accessible;
      }
      written += (size_t)result;
    }
    ::close(file_handle);
  }

  return ocTrackStoreReport::Success;
}