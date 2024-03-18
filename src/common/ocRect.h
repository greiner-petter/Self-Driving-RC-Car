#pragma once

#include "ocVec.h"

struct Rect
{
  float min_x;
  float min_y;
  float max_x;
  float max_y;

  constexpr Rect() = default;
  constexpr Rect(float x0, float y0, float x1, float y1)
  {
    min_x = std::min(x0, x1);
    min_y = std::min(y0, y1);
    max_x = std::max(x0, x1);
    max_y = std::max(y0, y1);
  }
  constexpr Rect(Vec2 p0, Vec2 p1)
  {
    min_x = std::min(p0.x, p1.x);
    min_y = std::min(p0.y, p1.y);
    max_x = std::max(p0.x, p1.x);
    max_y = std::max(p0.y, p1.y);
  }

  [[nodiscard]] constexpr float width()  const { return max_x - min_x; };
  [[nodiscard]] constexpr float height() const { return max_y - min_y; };
  [[nodiscard]] constexpr float area()   const { return width() * height(); };

  [[nodiscard]]
  constexpr bool contains(float x, float y) const
  {
    return min_x <= x && x <= max_x && min_y <= y && y <= max_y;
  }
  [[nodiscard]]
  constexpr bool contains(Vec2 v) const
  {
    return min_x <= v.x && v.x <= max_x && min_y <= v.y && v.y <= max_y;
  }
  [[nodiscard]]
  constexpr bool contains(Rect r) const
  {
    return min_x <= r.min_x && min_y <= r.min_y && r.max_x <= max_x && r.max_y <= max_y;
  }
  [[nodiscard]]
  constexpr bool intersects(Rect r) const
  {
    return min_x <= r.max_x && min_y <= r.max_y && r.min_x <= max_x && r.min_y <= max_y;
  }

  [[nodiscard]]
  constexpr Rect merge(Rect r) const
  {
    return Rect(std::min(min_x, r.min_x),
                std::min(min_y, r.min_y),
                std::max(max_x, r.max_x),
                std::max(max_y, r.max_y));
  }
  [[nodiscard]]
  constexpr Rect merge(float x, float y) const
  {
    return Rect(std::min(min_x, x),
                std::min(min_y, y),
                std::max(max_x, x),
                std::max(max_y, y));
  }
  [[nodiscard]]
  constexpr Rect merge(Vec2 v) const
  {
    return Rect(std::min(min_x, v.x),
                std::min(min_y, v.y),
                std::max(max_x, v.x),
                std::max(max_y, v.y));
  }
  [[nodiscard]]
  constexpr Rect intersection(Rect r) const
  {
    return Rect(std::max(min_x, r.min_x),
                std::max(min_y, r.min_y),
                std::min(max_x, r.max_x),
                std::min(max_y, r.max_y));
  }
  [[nodiscard]]
  constexpr float overlap(Rect r)
  {
    return intersection(r).area();
  }

  [[nodiscard]] constexpr Vec2 top_left()     const { return Vec2(min_x, min_y); }
  [[nodiscard]] constexpr Vec2 top_right()    const { return Vec2(max_x, min_y); }
  [[nodiscard]] constexpr Vec2 center()       const { return Vec2((min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f); }
  [[nodiscard]] constexpr Vec2 bottom_left()  const { return Vec2(min_x, max_y); }
  [[nodiscard]] constexpr Vec2 bottom_right() const { return Vec2(max_x, max_y); }

  [[nodiscard]] constexpr Rect translated(float x, float y) const
  {
    return Rect(min_x + x, min_y + y, max_x + x, max_y + y);
  }
  [[nodiscard]] constexpr Rect translated(Vec2 v) const
  {
    return Rect(min_x + v.x, min_y + v.y, max_x + v.x, max_y + v.y);
  }
};
