#pragma once

#include <algorithm> // std::min, std::clamp
#include <concepts> // std::convertible_to

namespace oc
{

struct Color final
{
  float r, g, b;

  [[nodiscard]] constexpr Color operator+(const Color& c) const
  {
    return Color{r + c.r, g + c.g, b + c.b};
  }
  [[nodiscard]] constexpr Color operator-(const Color& c) const
  {
    return Color{r - c.r, g - c.g, b - c.b};
  }
  [[nodiscard]] constexpr Color operator*(float f) const
  {
    return Color{r * f, g * f, b * f};
  }
  [[nodiscard]] constexpr Color operator/(float f) const
  {
    return Color{r / f, g / f, b / f};
  }

  constexpr void operator+=(const Color& c) { *this = *this + c; }
  constexpr void operator-=(const Color& c) { *this = *this - c; }
  constexpr void operator*=(float f) { *this = *this * f; }
  constexpr void operator/=(float f) { *this = *this / f; }
};

[[nodiscard]] constexpr Color operator*(float f, Color c) {return c * f;}

template<typename T>
concept RenderTarget =
  requires (T target, int x, int y, Color color, float mask) {
    {target.get_width()}  -> std::convertible_to<int>;
    {target.get_height()} -> std::convertible_to<int>;
    {target.draw_pixel(x, y, color)};
    {target.draw_pixel(x, y, color, mask)};
  };

template <RenderTarget T>
inline void fill(T& target, const Color& color)
{
  if constexpr (requires {target.fill(color);})
  {
    target.fill(color);
  }
  else
  {
    int w = target.get_width();
    int h = target.get_height();
    for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
    {
      target.draw_pixel(x, y, color);
    }
  }
}

template <RenderTarget T>
inline void fill(T& target, const Color& color, float mask)
{
  if constexpr (requires {target.fill(color, mask);})
  {
    target.fill(color, mask);
  }
  else
  {
    int w = target.get_width();
    int h = target.get_height();
    for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
    {
      target.draw_pixel(x, y, color, mask);
    }
  }
}

template <RenderTarget T>
inline void fill_rect(T& target, int x0, int y0, int x1, int y1, const Color& color)
{
  int w = target.get_width();
  int h = target.get_height();
  int start_x = std::clamp(std::min(x0, x1), 0, w);
  int start_y = std::clamp(std::min(y0, y1), 0, h);
  int end_x   = std::clamp(std::max(x0, x1), 0, w);
  int end_y   = std::clamp(std::max(y0, y1), 0, h);
  if constexpr (requires {target.fill_rect(start_x, start_y, end_x, end_y, color);})
  {
    target.fill_rect(start_x, start_y, end_x, end_y, color);
  }
  else
  {
    for (int y = start_y; y < end_y; ++y)
    for (int x = start_x; x < end_x; ++x)
    {
      target.draw_pixel(x, y, color);
    }
  }
}

template <RenderTarget T>
inline void fill_rect(T& target, int x0, int y0, int x1, int y1, const Color& color, float mask)
{
  int w = target.get_width();
  int h = target.get_height();
  int start_x = std::clamp(std::min(x0, x1), 0, w);
  int start_y = std::clamp(std::min(y0, y1), 0, h);
  int end_x   = std::clamp(std::max(x0, x1), 0, w);
  int end_y   = std::clamp(std::max(y0, y1), 0, h);
  if constexpr (requires {target.fill_rect(start_x, start_y, end_x, end_y, color, mask);})
  {
    target.fill_rect(start_x, start_y, end_x, end_y, color, mask);
  }
  else
  {
    for (int y = start_y; y < end_y; ++y)
    for (int x = start_x; x < end_x; ++x)
    {
      target.draw_pixel(x, y, color, mask);
    }
  }
}

template <RenderTarget T>
inline void draw_rect(T& target, int x0, int y0, int x1, int y1, const Color& color)
{
  if constexpr (requires {target.draw_rect(x0, y0, x1, y1, color);})
  {
    target.draw_rect(x0, y0, x1, y1, color);
  }
  else
  {
    int start_x = std::min(x0, x1);
    int start_y = std::min(y0, y1);
    int end_x   = std::max(x0, x1);
    int end_y   = std::max(y0, y1);
    for (int x = start_x; x < end_x; ++x)
    {
      target.draw_pixel(x, start_y,   color);
      target.draw_pixel(x, end_y - 1, color);
    }
    for (int y = start_y + 1; y < end_y - 1; ++y)
    {
      target.draw_pixel(start_x,   y, color);
      target.draw_pixel(end_x - 1, y, color);
    }
  }
}

template <RenderTarget T>
inline void draw_rect(T& target, int x0, int y0, int x1, int y1, const Color& color, float mask)
{
  if constexpr (requires {target.draw_rect(x0, y0, x1, y1, color, mask);})
  {
    target.draw_rect(x0, y0, x1, y1, color, mask);
  }
  else
  {
    int start_x = std::min(x0, x1);
    int start_y = std::min(y0, y1);
    int end_x   = std::max(x0, x1);
    int end_y   = std::max(y0, y1);
    for (int x = start_x; x < end_x; ++x)
    {
      target.draw_pixel(x, start_y,   color, mask);
      target.draw_pixel(x, end_y - 1, color, mask);
    }
    for (int y = start_y + 1; y < end_y - 1; ++y)
    {
      target.draw_pixel(start_x,   y, color, mask);
      target.draw_pixel(end_x - 1, y, color, mask);
    }
  }
}

template <RenderTarget T>
inline void draw_line(T& target, int x0, int y0, int x1, int y1, const Color& color)
{
  if constexpr (requires {target.draw_line(x0, y0, x1, y1, color);})
  {
    target.draw_line(x0, y0, x1, y1, color);
  }
  else
  {
    if (y0 == y1)
    {
      if (y0 < 0 || target.get_height() <= y0) return;
      int start = std::max(                     0, std::min(x0, x1));
      int end   = std::min(target.get_width() - 1, std::max(x0, x1));
      for (int x = start; x <= end; ++x)
      {
        target.draw_pixel(x, y0, color);
      }
    }
    else if (x0 == x1)
    {
      if (x0 < 0 || target.get_width() <= x0) return;
      int start = std::max(                      0, std::min(y0, y1));
      int end   = std::min(target.get_height() - 1, std::max(y0, y1));
      for (int y = start; y <= end; ++y)
      {
        target.draw_pixel(x0, y, color);
      }
    }
    else
    {
      int dx = x1 - x0;
      int dy = y1 - y0;

      if (std::abs(dx) < std::abs(dy))
      {
        int sy = (dy < 0 ? -1 : 1);
        for (int y = 0; y <= dy; y += sy)
        {
          target.draw_pixel(x0 + dx * y / dy, y0 + y, color);
        }
      }
      else
      {
        int sx = (dx < 0 ? -1 : 1);
        for (int x = 0; x <= dx; x += sx)
        {
          target.draw_pixel(x0 + x, y0 + dy * x / dx, color);
        }
      }
    }
  }
}

template <RenderTarget T>
inline void draw_line(T& target, int x0, int y0, int x1, int y1, const Color& color, float mask)
{
  if constexpr (requires {target.draw_line(x0, y0, x1, y1, color, mask);})
  {
    target.draw_line(x0, y0, x1, y1, color, mask);
  }
  else
  {
    if (y0 == y1)
    {
      if (y0 < 0 || target.get_height() <= y0) return;
      int start = std::max(                     0, std::min(x0, x1));
      int end   = std::min(target.get_width() - 1, std::max(x0, x1));
      for (int x = start; x <= end; ++x)
      {
        target.draw_pixel(x, y0, color, mask);
      }
    }
    else if (x0 == x1)
    {
      if (x0 < 0 || target.get_width() <= x0) return;
      int start = std::max(                      0, std::min(y0, y1));
      int end   = std::min(target.get_height() - 1, std::max(y0, y1));
      for (int y = start; y <= end; ++y)
      {
        target.draw_pixel(x0, y, color, mask);
      }
    }
    else
    {
      int dx = x1 - x0;
      int dy = y1 - y0;

      if (std::abs(dx) < std::abs(dy))
      {
        int sy = (dy < 0 ? -1 : 1);
        for (int y = 0; y <= dy; y += sy)
        {
          target.draw_pixel(x0 + dx * y / dy, y0 + y, color, mask);
        }
      }
      else
      {
        int sx = (dx < 0 ? -1 : 1);
        for (int x = 0; x <= dx; x += sx)
        {
          target.draw_pixel(x0 + x, y0 + dy * x / dx, color, mask);
        }
      }
    }
  }
}
/*
template<RenderTarget T>
inline void draw_image(
  T& target,
  int target_x, int target_y,
  int width, int height,
  const Image<C>& source,
  int source_x, int source_y)
{
  int start_x = -std::min(0, target_x);
  int start_y = -std::min(0, target_y);
  int end_x = std::min(target_x + width,  target.get_width())  - target_x;
  int end_y = std::min(target_y + height, target.get_height()) - target_y;
  for (int wy = start_y; wy < end_y; ++wy)
  {
    int iy = source_y + wy;
    for (int wx = start_x; wx < end_x; ++wx)
    {
      int ix = source_x + wx;
      target.draw_pixel(target_x + wx, target_y + wy, source(ix, iy));
    }
  }
}

template<RenderTarget T>
inline void draw_image(
  T& target,
  int target_x, int target_y,
  int width, int height,
  const MaskedImage<C, M>& source,
  int source_x, int source_y)
{
  int start_x = -std::min(0, target_x);
  int start_y = -std::min(0, target_y);
  int end_x = std::min(target_x + width,  target.get_width())  - target_x;
  int end_y = std::min(target_y + height, target.get_height()) - target_y;
  for (int wy = start_y; wy < end_y; ++wy)
  {
    int iy = source_y + wy;
    for (int wx = start_x; wx < end_x; ++wx)
    {
      int ix = source_x + wx;
      target.draw_pixel(target_x + wx, target_y + wy, source(ix, iy), source.get_mask(ix, iy));
    }
  }
}

template<typename T, typename I>
inline void draw_image(
  T& target,
  int target_x, int target_y,
  const I& source)
{
  draw_image(
    target,
    target_x, target_y,
    source.width, source.height,
    source,
    0, 0);
}
*/
} // namespace oc
