#pragma once

#include <cstdlib> // free

struct ocRenderCamera
{
  size_t   mem_size  = 0;
  void    *mem       = nullptr;
  bool     is_ortho  = false;
  bool     is_linear = false;
  uint32_t width     = 0;
  uint32_t height    = 0;
  uint32_t channels  = 0;

  ocRenderCamera() = default;

  ~ocRenderCamera()
  {
    if (mem) free(mem);
  }

  ocRenderCamera(const ocRenderCamera&) = delete;
  void operator=(const ocRenderCamera&) = delete;
};
