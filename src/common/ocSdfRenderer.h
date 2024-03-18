#pragma once

#include "ocRect.h"
#include "ocRenderTarget.h"
#include "ocVec.h"

#include <algorithm>
#include <cmath>
#include <cfloat>

namespace oc
{

template<typename T>
concept Sdf = requires (T t, float f) {{t(f, f)} -> std::convertible_to<float>;};

// ---------------------------- Circle ----------------------------------------

struct CircleSdf
{
  float center_x, center_y, radius;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float dx = center_x - x;
    float dy = center_y - y;
    return std::sqrt(dx * dx + dy * dy) - radius;
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    return {
      center_x - radius,
      center_y - radius,
      center_x + radius,
      center_y + radius
    };
  }
};

[[nodiscard]]
constexpr auto circle(float center_x, float center_y, float radius)
{
  return CircleSdf {center_x, center_y, radius};
}

[[nodiscard]]
constexpr auto circle(Vec2 center, float radius)
{
  return CircleSdf {center.x, center.y, radius};
}


// ---------------------------- Line ------------------------------------------

struct LineSdf
{
  float x0, y0;
  float dx, dy;
  float nx, ny;
  float half_width;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float x2 = x - x0;
    float y2 = y - y0;
    float a = std::clamp(x2 * nx + y2 * ny, 0.0f, 1.0f);
    float dist_x = x2 - dx * a;
    float dist_y = y2 - dy * a;
    return std::sqrt(dist_x * dist_x + dist_y * dist_y) - half_width;
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    return {
      std::min(x0, x0 + dx) - half_width,
      std::min(y0, y0 + dy) - half_width,
      std::max(x0, x0 + dx) + half_width,
      std::max(y0, y0 + dy) + half_width
    };
  }
};

[[nodiscard]]
constexpr auto line(float x0, float y0, float x1, float y1, float width)
{
  float dx = x1 - x0;
  float dy = y1 - y0;
  float inverse_length_squared = 1.0f / (dx * dx + dy * dy);
  float nx = dx * inverse_length_squared;
  float ny = dy * inverse_length_squared;

  return LineSdf {
    x0, y0,
    dx, dy,
    nx, ny,
    width * 0.5f
  };
}

[[nodiscard]]
constexpr auto line(Vec2 p0, Vec2 p1, float width)
{
  float dx = p1.x - p0.x;
  float dy = p1.y - p0.y;
  float inverse_length_squared = 1.0f / (dx * dx + dy * dy);
  float nx = dx * inverse_length_squared;
  float ny = dy * inverse_length_squared;

  return LineSdf {
    p0.x, p0.y,
    dx, dy,
    nx, ny,
    width * 0.5f
  };
}

// ---------------------------- Half ------------------------------------------

inline auto half(float nx, float ny, float d)
{
  // nx * nx + ny * ny must be 1
  return [=](float x, float y) {
    return x * nx + y * ny - d;
  };
}

inline auto half(float nx, float ny, float px, float py)
{
  float d = half(nx, ny, 0)(px, py);
  return half(nx, ny, d);
}

// ---------------------------- Box -------------------------------------------

struct BoxSdf
{
  float center_x, center_y;
  float half_width, half_height;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float dx = std::abs(center_x - x) - half_width;
    float dy = std::abs(center_y - y) - half_height;
    return std::min(0.0f, std::max(dx, dy)) + std::sqrt(std::max(0.0f, dx) * dx + std::max(0.0f, dy) * dy);
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    return {
      center_x - half_width,
      center_y - half_height,
      center_x + half_width,
      center_y + half_height
    };
  }
};

[[nodiscard]]
inline auto box(float x0, float y0, float x1, float y1)
{
  return BoxSdf {
    .center_x = (x0 + x1) * 0.5f,
    .center_y = (y0 + y1) * 0.5f,
    .half_width  = std::abs(x0 - x1) * 0.5f,
    .half_height = std::abs(y0 - y1) * 0.5f
  };
}

[[nodiscard]]
inline auto box(Vec2 p0, Vec2 p1)
{
  return BoxSdf {
    .center_x = (p0.x + p1.x) * 0.5f,
    .center_y = (p0.y + p1.y) * 0.5f,
    .half_width  = std::abs(p0.x - p1.x) * 0.5f,
    .half_height = std::abs(p0.y - p1.y) * 0.5f
  };
}


// ---------------------------- Rounded Box -----------------------------------

struct RoundedBoxSdf
{
  float center_x, center_y;
  float half_width, half_height;
  float radius;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float dx = std::abs(center_x - x) - half_width  + radius;
    float dy = std::abs(center_y - y) - half_height + radius;
    return std::min(0.0f, std::max(dx, dy)) + std::sqrt(std::max(0.0f, dx) * dx + std::max(0.0f, dy) * dy) - radius;
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    return {
      center_x - half_width,
      center_y - half_height,
      center_x + half_width,
      center_y + half_height
    };
  }
};

[[nodiscard]]
inline auto box(float x0, float y0, float x1, float y1, float radius)
{
  return RoundedBoxSdf {
    .center_x = (x0 + x1) * 0.5f,
    .center_y = (y0 + y1) * 0.5f,
    .half_width  = std::abs(x0 - x1) * 0.5f,
    .half_height = std::abs(y0 - y1) * 0.5f,
    .radius = radius
  };
}


// ---------------------------- Polygon ---------------------------------------

template<typename ...Args>
auto polygon(Args... fs)
{
  static_assert(0 == (sizeof...(Args) & 1));
  const size_t count = sizeof...(Args)/2;
  float vals[] = {fs...};
  struct
  {
    float x, y;
    float dx, dy;
    float nx, ny;
  } cached[count];
  for (size_t j = count-1, i = 0; i < count; j = i, ++i)
  {
    cached[i].x = vals[i * 2 + 0];
    cached[i].y = vals[i * 2 + 1];
    cached[i].dx = vals[j * 2 + 0] - cached[i].x;
    cached[i].dy = vals[j * 2 + 1] - cached[i].y;
    float sq = cached[i].dx * cached[i].dx + cached[i].dy * cached[i].dy;
    cached[i].nx = cached[i].dx / sq;
    cached[i].ny = cached[i].dy / sq;
  }
  return [=](float x, float y)
  {
    float d = FLT_MAX;
    float s = 1.0f;
    float prev_y = cached[count - 1].y;
    for (size_t i = 0; i < count; ++i)
    {
      float dx = x - cached[i].x;
      float dy = y - cached[i].y;
      float f = std::clamp(dx * cached[i].nx + dy * cached[i].ny, 0.0f, 1.0f);
      float bx = dx - cached[i].dx * f;
      float by = dy - cached[i].dy * f;
      d = std::min(d, bx * bx + by * by);
      bool a = cached[i].y <= y;
      bool b = y < prev_y;
      bool c = cached[i].dy * dx < cached[i].dx * dy;
      if ((a == b) && (b == c)) s *= -1.0f;
      prev_y = cached[i].y;
    }
    return s * std::sqrt(d);
  };
}


// ---------------------------- Polyline --------------------------------------

template<bool closed = true, typename ...Args>
auto polyline(float line_width, Args... fs)
{
  static_assert(0 == (sizeof...(Args) & 1));
  const size_t vert_count = sizeof...(Args)/2;
  const size_t edge_count = vert_count - (closed ? 0 : 1);
  float vals[] = {fs...};
  struct
  {
    float x, y;
    float dx, dy;
    float nx, ny;
  } cached[edge_count];
  auto *cursor = &cached[0];
  for (size_t j = (closed ? edge_count-1 : 0), i = (closed ? 0 : 1); i < vert_count; j = i, ++i)
  {
    cursor->x = vals[i * 2 + 0];
    cursor->y = vals[i * 2 + 1];
    cursor->dx = vals[j * 2 + 0] - cursor->x;
    cursor->dy = vals[j * 2 + 1] - cursor->y;
    float sq = cursor->dx * cursor->dx + cursor->dy * cursor->dy;
    cursor->nx = cursor->dx / sq;
    cursor->ny = cursor->dy / sq;
    cursor++;
  }
  return [=](float x, float y)
  {
    float d = FLT_MAX;
    for (size_t i = 0; i < edge_count; ++i)
    {
      float dx = x - cached[i].x;
      float dy = y - cached[i].y;
      float f = std::clamp(dx * cached[i].nx + dy * cached[i].ny, 0.0f, 1.0f);
      float bx = dx - cached[i].dx * f;
      float by = dy - cached[i].dy * f;
      d = std::min(d, bx * bx + by * by);
    }
    return std::sqrt(d) - line_width * 0.5f;
  };
}


// ---------------------------- Arc -------------------------------------------

auto arc(
  float ax, float ay,
  float bx, float by,
  float cx, float cy,
  float line_width)
{
  // TODO: Handle the collinear and degenerate case.
  float abx = bx - ax;
  float aby = by - ay;
  float bcx = cx - bx;
  float bcy = cy - by;
  float dx = (ax + bx) * 0.5f;
  float dy = (ay + by) * 0.5f;
  float ex = (bx + cx) * 0.5f;
  float ey = (by + cy) * 0.5f;
  float edx = dx - ex;
  float edy = dy - ey;
  float n = (bcx * edx + bcy * edy) / (abx *-bcy + aby * bcx);
  float center_x = dx - aby * n;
  float center_y = dy - abx * n;
  float dax = ax - center_x;
  float day = ay - center_y;
  float dbx = bx - center_x;
  float dby = by - center_y;
  float dcx = cx - center_x;
  float dcy = cy - center_y;
  float radius = std::sqrt(dax * dax + day * day);
  float angle_a = std::atan2(day, dax);
  float angle_b = std::atan2(dby, dbx);
  float angle_c = std::atan2(dcy, dcx);
  if (angle_c < angle_a) std::swap(angle_a, angle_c);
  return [=](float x, float y)
  {
    float rx = x - center_x;
    float ry = y - center_y;
    float angle = std::atan2(ry, rx);
    if ((angle_a < angle_b && angle_b < angle_c) == (angle_a < angle && angle < angle_c))
    {
      return std::abs(std::abs(rx * rx + ry * ry) - radius) - line_width * 0.5f;
    }
    else
    {
      float dxa = x - ax;
      float dya = y - ay;
      float dxc = x - cx;
      float dyc = y - cy;
      return std::sqrt(std::min(dxa * dxa + dya * dya, dxc * dxc + dyc * dyc)) - line_width * 0.5f;
    }
  };
}


// ---------------------------- Quadratic Bezier ------------------------------

struct BezierSdf
{
  float origin_x, origin_y;
  float nx, ny;
  float min_x, max_x;
  float scale;
  float line_width;

  Rect aabb;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float px = ((x - origin_x) * ny - (y - origin_y) * nx) * scale;
    float py = ((x - origin_x) * nx + (y - origin_y) * ny) * scale;
    double e = ((1.5 - py) * py - 0.75) * py + 0.125;
    double f = 0.0625 * px * px + e / 27.0;
    if (0.0 <= f)
    {
      double g = std::sqrt(f);
      double cx = std::clamp<double>(std::cbrt(0.25 * px + g) + std::cbrt(0.25 * px - g), min_x, max_x);
      return (float)std::sqrt(std::pow(cx - px, 2.0) + pow(cx * cx - py, 2.0)) / scale - line_width * 0.5f;
    }
    else
    {
      double v = std::acos(std::sqrt(27.0 / -e) * px * 0.25) / 3.0;
      double m = cos(v);
      double n = sin(v) * std::sqrt(3.0f);
      double o = std::sqrt((py - 0.5) / 3.0);
      double cx1 = std::clamp<double>( (m + m) * o, min_x, max_x);
      double cx2 = std::clamp<double>(-(n + m) * o, min_x, max_x);
      float d1 = (float)(pow(cx1 - px, 2.0) + pow(cx1 * cx1 - py, 2.0));
      float d2 = (float)(pow(cx2 - px, 2.0) + pow(cx2 * cx2 - py, 2.0));
      return std::sqrt(std::min(d1, d2)) / scale - line_width * 0.5f;
    }
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    return aabb;
  }
};

auto bezier(
  float ax, float ay,
  float bx, float by,
  float cx, float cy,
  float line_width)
{
  float bdx = (ax + cx) * 0.5f - bx;
  float bdy = (ay + cy) * 0.5f - by;
  float ilbd = 1.0f / std::sqrt(bdx * bdx + bdy * bdy); // TODO: possible division by 0
  float nx = bdx * ilbd;
  float ny = bdy * ilbd;
  float abx = bx - ax;
  float aby = by - ay;
  float bcx = cx - bx;
  float bcy = cy - by;
  float slope_a = ((abx * nx + aby * ny) / (abx * ny - aby * nx)) * 0.5f;
  float slope_c = ((bcx * nx + bcy * ny) / (bcx * ny - bcy * nx)) * 0.5f;
  float scale = (slope_c - slope_a) / ((cx - ax) * ny - (cy - ay) * nx);
  float tx = std::clamp(abx / (abx - bcx), 0.0f, 1.0f);
  float ty = std::clamp(aby / (aby - bcy), 0.0f, 1.0f);
  float ntx = 1.0f - tx;
  float nty = 1.0f - ty;
  float dx = ax * ntx * ntx + 2.0f * bx * ntx * tx + cx * tx * tx;
  float dy = ay * nty * nty + 2.0f * by * nty * ty + cy * ty * ty;

  return BezierSdf {
    .origin_x   = ax - ny * slope_a / scale - nx * slope_a * slope_a / scale,
    .origin_y   = ay + nx * slope_a / scale - ny * slope_a * slope_a / scale,
    .nx         = nx,
    .ny         = ny,
    .min_x      = std::min(slope_a, slope_c),
    .max_x      = std::max(slope_a, slope_c),
    .scale      = scale,
    .line_width = line_width,
    .aabb       = Rect {
      std::min(std::min(ax, cx), dx) - line_width * 0.5f,
      std::min(std::min(ay, cy), dy) - line_width * 0.5f,
      std::max(std::max(ax, cx), dx) + line_width * 0.5f,
      std::max(std::max(ay, cy), dy) + line_width * 0.5f
    }
  };
}

auto bezier(Vec2 a, Vec2 b, Vec2 c, float line_width)
{
  return bezier(a.x, a.y, b.x, b.y, c.x, c.y, line_width);
}

// ---------------------------- Modifier --------------------------------------

auto translate(Sdf auto sdf, float tx, float ty)
{
  return [=](float x, float y) { return sdf(x - tx, y - ty); };
}

auto scale(Sdf auto sdf, float factor, float origin_x = 0.0f, float origin_y = 0.0f)
{
  return [=](float x, float y)
  {
    return sdf((x - origin_x) / factor + origin_x, (y - origin_y) / factor + origin_y) * factor;
  };
}

template<Sdf Child>
struct RotatedSdf
{
  Child child;
  float cos_radians;
  float sin_radians;
  float pivot_x;
  float pivot_y;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float dx = x - pivot_x;
    float dy = y - pivot_y;
    return child(dx * cos_radians + dy * sin_radians + pivot_x,
                 dy * cos_radians - dx * sin_radians + pivot_y);
  }
};

template<Sdf Child>
struct BoundedRotatedSdf
{
  Child child;
  float cos_radians;
  float sin_radians;
  float pivot_x;
  float pivot_y;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    float dx = x - pivot_x;
    float dy = y - pivot_y;
    return child(dx * cos_radians + dy * sin_radians + pivot_x,
                 dy * cos_radians - dx * sin_radians + pivot_y);
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    Rect result = child.bounds();
    float cl = cos_radians * result.min_x;
    float sl = sin_radians * result.min_x;
    float ct = cos_radians * result.min_y;
    float st = sin_radians * result.min_y;
    float cr = cos_radians * result.max_x;
    float sr = sin_radians * result.max_x;
    float cb = cos_radians * result.max_y;
    float sb = sin_radians * result.max_y;
    return {
      std::min(cl, cr) - std::max(st, sb),
      std::min(ct, cb) + std::min(sl, sr),
      std::max(cl, cr) - std::min(st, sb),
      std::max(ct, cb) + std::max(sl, sr)
    };
  }
};

auto rotate_cw(Sdf auto sdf, float radians, float pivot_x = 0.0f, float pivot_y = 0.0f)
{
  float c = std::cos(radians);
  float s = std::sin(radians);
  
  if constexpr (requires { sdf.bounds(); })
  {
    return BoundedRotatedSdf<decltype(sdf)> {
      .child = sdf,
      .cos_radians = c,
      .sin_radians = s,
      .pivot_x = pivot_x,
      .pivot_y = pivot_y
    };
  }
  else
  {
    return RotatedSdf<decltype(sdf)> {
      .child = sdf,
      .cos_radians = c,
      .sin_radians = s,
      .pivot_x = pivot_x,
      .pivot_y = pivot_y
    };
  }
}

auto rotate_ccw(Sdf auto sdf, float radians, float pivot_x = 0.0f, float pivot_y = 0.0f)
{
  return rotate_cw(sdf, -radians, pivot_x, pivot_y);
}

template<Sdf Child>
struct OutlineSdf
{
  Child child;
  float half_line_width;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    return std::abs(child(x, y)) - half_line_width;
  }
};

template<Sdf Child>
struct BoundedOutlineSdf
{
  Child child;
  float half_line_width;

  [[nodiscard]]
  inline float operator()(float x, float y) const
  {
    return std::abs(child(x, y)) - half_line_width;
  }

  [[nodiscard]]
  constexpr Rect bounds() const
  {
    Rect result = child.bounds();
    result.min_x -= half_line_width;
    result.min_y -= half_line_width;
    result.max_x += half_line_width;
    result.max_y += half_line_width;
    return result;
  }
};

auto outline(Sdf auto sdf, float line_width)
{
  if constexpr (requires { sdf.bounds(); })
  {
    return BoundedOutlineSdf<decltype(sdf)> {
      .child           = sdf,
      .half_line_width = line_width * 0.5f
    };
  }
  else
  {
    return OutlineSdf<decltype(sdf)> {
      .child           = sdf,
      .half_line_width = line_width * 0.5f
    };
  }
}


// ---------------------------- Combiner --------------------------------------

template<Sdf ...Args>
auto merge(Args ...sdfs)
{
  static_assert(2 <= sizeof...(sdfs));
  return [=](float x, float y)
  {
    float distances[] = { sdfs(x, y)... };
    float sum = 0.0f;
    float min = FLT_MAX;
    for (size_t i = 0; i < sizeof...(sdfs); ++i)
    {
      float d = std::max(0.0f, -distances[i]);
      sum += d * d;
      min = std::min(min, distances[i]);
    }
    return std::max(0.0f, min) - std::sqrt(sum);
  };
}

template<Sdf ...Args>
auto merge(float radius, Args ...sdfs)
{
  static_assert(2 <= sizeof...(sdfs));
  return [=](float x, float y)
  {
    float distances[] = { sdfs(x, y)... };
    float sum = 0.0f;
    float min = FLT_MAX;
    for (size_t i = 0; i < sizeof...(sdfs); ++i)
    {
      float d = std::max(0.0f, radius - distances[i]);
      sum += d * d;
      min = std::min(min, distances[i]);
    }
    return std::max(radius, min) - std::sqrt(sum);
  };
}

template<Sdf ...Args>
auto intersect(Args ...sdfs)
{
  static_assert(2 <= sizeof...(sdfs));
  return [=](float x, float y)
  {
    float distances[] = { sdfs(x, y)... };
    float sum = 0.0f;
    float max = -FLT_MAX;
    for (size_t i = 0; i < sizeof...(sdfs); ++i)
    {
      float d = std::max(0.0f, distances[i]);
      sum += d * d;
      max = std::max(max, distances[i]);
    }
    return std::min(0.0f, max) + std::sqrt(sum);
  };
}

template<Sdf ...Args>
auto intersect(float radius, Args ...sdfs)
{
  static_assert(2 <= sizeof...(sdfs));
  return [=](float x, float y)
  {
    float distances[] = { sdfs(x, y)... };
    float sum = 0.0f;
    float max = -FLT_MAX;
    for (size_t i = 0; i < sizeof...(sdfs); ++i)
    {
      float d = std::max(0.0f, radius + distances[i]);
      sum += d * d;
      max = std::max(max, distances[i]);
    }
    return std::min(-radius, max) + std::sqrt(sum);
  };
}

auto subtract(Sdf auto sdf0, Sdf auto sdf1)
{
  return [=](float x, float y)
  {
    float a =  sdf0(x, y);
    float b = -sdf1(x, y);
    float da = std::max(0.0f, a);
    float db = std::max(0.0f, b);
    return std::min(0.0f, std::max(a, b)) + std::sqrt(da * da + db * db);
  };
}

auto subtract(float radius, Sdf auto sdf0, Sdf auto sdf1)
{
  return [=](float x, float y)
  {
    float a =  sdf0(x, y);
    float b = -sdf1(x, y);
    float da = std::max(0.0f, radius + a);
    float db = std::max(0.0f, radius + b);
    return std::min(-radius, std::max(a, b)) + std::sqrt(da * da + db * db);
  };
}

auto exclusive(Sdf auto sdf0, Sdf auto sdf1)
{
  return [=](float x, float y)
  {
    float a =  sdf0(x, y);
    float b = -sdf1(x, y);
    float da = std::max(0.0f, -a);
    float db = std::max(0.0f, -b);
    return std::min(0.0f, std::max(a, b)) - std::sqrt(da * da + db * db);
  };
}

auto exclusive(float radius, Sdf auto sdf0, Sdf auto sdf1)
{
  return [=](float x, float y)
  {
    float a =  sdf0(x, y);
    float b = -sdf1(x, y);
    float da = std::max(0.0f, radius - a);
    float db = std::max(0.0f, radius - b);
    return std::min(-radius, std::max(a, b)) - std::sqrt(da * da + db * db);
  };
}


// ---------------------------- Rendering -------------------------------------

float coverage(
  float center_x,
  float center_y,
  Sdf auto sdf)
{
  return std::clamp(0.5f - sdf(center_x, center_y), 0.0f, 1.0f);
}

template<RenderTarget T, Sdf S>
void render(
  T& target,
  int left, int top,
  int width, int height,
  const S sdf,
  const Color& color,
  float opacity)
{
  if (left < 0)
  {
    width += left;
    left = 0;
  }
  if (top < 0)
  {
    height += top;
    top = 0;
  }
  if (target.get_width() < left + width)
  {
    width = target.get_width() - left;
  }
  if (target.get_height() < top + height)
  {
    height = target.get_height() - top;
  }

  if (width <= 0 || height <= 0) return;
  // This algorithm recursively subdivides the given area
  // into smaller subsections, until they are a few pixels
  // in size. Then the coverage of the pixels is computed
  // and drawn to the render_target.
  // If the algorithm can determine early, that an area is
  // completely inside or outside of the given sdf, it will
  // not recurse further and instead skip or fill the area.

  struct RectI {int16_t x, y, w, h;};
  RectI stack[48];
  int stack_i = 0;

  auto push = [&](int32_t x, int32_t y, int32_t w, int32_t h) {
    oc_assert(0 < w);
    oc_assert(0 < h);
    stack[stack_i].x = (int16_t)x;
    stack[stack_i].y = (int16_t)y;
    stack[stack_i].w = (int16_t)w;
    stack[stack_i].h = (int16_t)h;
    stack_i += 1;
  };

  push(left, top, width, height);

  while (stack_i)
  {
    oc_assert(stack_i <= 48);
    stack_i -= 1; 
    int32_t x = stack[stack_i].x;
    int32_t y = stack[stack_i].y;
    int32_t w = stack[stack_i].w;
    int32_t h = stack[stack_i].h;
    int32_t area = w * h;

    oc_assert(area);

    if (1 == area)
    {
      auto c = coverage((float)x, (float)y, sdf);
      target.draw_pixel(x, y, color * c, opacity * c);
      continue;
    }

    {
      float half_width  = (float)w * 0.5f;
      float half_height = (float)h * 0.5f;
      float center_x    = (float)x - 0.5f + half_width;
      float center_y    = (float)y - 0.5f + half_height;

      float dist = sdf(center_x, center_y);
      float dist_sq = dist * dist;

      // Check if the distance goes further than the diagonal of our current
      // rect. If so, we can fill or skip the whole rect and be done with it.
      if (half_width * half_width + half_height * half_height <= dist_sq + 0.001f)
      {
        if (dist < 0.0f) fill_rect(target, x, y, x + w, y + h, color, opacity);
        continue;
      }

      float short_side  = std::min(half_width, half_height);
      float long_side   = std::max(half_width, half_height);

      {
        // Check if the distance is so large, that at least one row or column of pixels
        // would be covered along the long side of the area. In that case, we can fill
        // in a plus shaped area and put the four remaining corners back on the stack.
        float a = long_side;
        float b = 1.0f - std::fmod(short_side, 1.0f);
        if (a * a + b * b <= dist_sq)
        {
          int side_width  = (int)std::ceil(half_width  - std::sqrt(dist_sq - half_height * half_height));
          int side_height = (int)std::ceil(half_height - std::sqrt(dist_sq - half_width  * half_width ));
          if (dist < 0.0f)
          {
            fill_rect(target, x + side_width, y,                   x + w - side_width, y     + side_height, color, opacity);
            fill_rect(target, x,              y     + side_height, x + w,              y + h - side_height, color, opacity);
            fill_rect(target, x + side_width, y + h - side_height, x + w - side_width, y + h,               color, opacity);
          }
          push(x,                  y,                   side_width, side_height);
          push(x + w - side_width, y,                   side_width, side_height);
          push(x,                  y + h - side_height, side_width, side_height);
          push(x + w - side_width, y + h - side_height, side_width, side_height);
          continue;
        }
      }

      {
        // Check if the distance is at least long enough to cover a row or column of
        // pixels along the short side of the area. In that case we can fill in that
        // row or column and put the remaining sides back on the stack.
        float a = short_side;
        float b = 1.0f - std::fmod(long_side, 1.0f); // either 1.0f or 0.5f
        if (a * a + b * b <= dist_sq)
        {
          int side_length = (int)std::ceil(long_side - std::sqrt(dist_sq - short_side * short_side));
          if (w < h)
          {
            if (dist < 0.0f)
            {
              fill_rect(target, x, y + side_length, x + w, y + h - side_length, color, opacity);
            }
            push(x, y,                   w, side_length);
            push(x, y + h - side_length, w, side_length);
          }
          else
          {
            if (dist < 0.0f)
            {
              fill_rect(target, x + side_length, y, x + w - side_length, y + h, color, opacity);
            }
            push(x,                   y, side_length, h);
            push(x + w - side_length, y, side_length, h);
          }
          continue;
        }
      }

      {
        // Check if the distance is large enough to fill any area. The remaining
        // area around it will be split into 4 new areas.
        float a = 1.0f - std::fmod(short_side, 1.0f);
        float b = 1.0f - std::fmod( long_side, 1.0f);
        if (a * a + b * b <= dist_sq)
        {
          float abs_dist = std::abs(dist) / std::sqrt(2.0f);
          int side_width  = (int)std::ceil(half_width  - abs_dist);
          int side_height = (int)std::ceil(half_height - abs_dist);
          float tlx = half_width  - (float)side_width;
          float tly = half_height - (float)side_height;

          if ((tlx + 1.0f) * (tlx + 1.0f) + tly * tly <= dist_sq)
          {
            side_width -= 1;
          }
          else if (tlx * tlx + (tly + 1.0f) * (tly + 1.0f) <= dist_sq)
          {
            side_height -= 1;
          }

          if (dist < 0.0f)
          {
            fill_rect(target, x + side_width, y + side_height, x + w - side_width, y + h - side_height, color, opacity);
          }
          if (w < h)
          {
            push(x,                  y,                   w,          side_height);
            push(x,                  y     + side_height, side_width, h - 2 * side_height);
            push(x + w - side_width, y     + side_height, side_width, h - 2 * side_height);
            push(x,                  y + h - side_height, w,          side_height);
          }
          else
          {
            push(x,                  y,                   side_width,         h);
            push(x     + side_width, y,                   w - side_width * 2, side_height);
            push(x + w - side_width, y,                   side_width,         h);
            push(x     + side_width, y + h - side_height, w - 2 * side_width, side_height);
          }
          continue;
        }
      }

      int side_width  = (int)std::floor(half_width);
      int side_height = (int)std::floor(half_height);
      if (0 != (w - 2 * side_width) * (h - 2 * side_height))
      {
        // If both sides are of uneven length, we handle the center pixel and
        // divide the area around it into four evenly sized rectangles.
        auto c = std::clamp(0.5f - dist, 0.0f, 1.0f);
        target.draw_pixel(x + side_width, y + side_height, color * c, opacity * c);
        if (1 == w)
        {
          push(x, y,                   1, side_height);
          push(x, y + h - side_height, 1, side_height);
        }
        else if (1 == h)
        {
          push(x,                  y, side_width, 1);
          push(x + w - side_width, y, side_width, 1);
        }
        else
        {
          push(x,                  y,                   w - side_width,     side_height);
          push(x + w - side_width, y,                       side_width, h - side_height);
          push(x     + side_width, y + h - side_height, w - side_width,     side_height);
          push(x,                  y     + side_height,     side_width, h - side_height);
        }
      }
      else if (w < h)
      {
        push(x, y,               w,     side_height);
        push(x, y + side_height, w, h - side_height);
      }
      else
      {
        push(x,              y,     side_width, h);
        push(x + side_width, y, w - side_width, h);
      }
    }
  }
}

template<RenderTarget T, Sdf S>
void render(
  T& target,
  const S sdf,
  const Color& color,
  float opacity = 1.0f)
{
  int left   = 0;
  int top    = 0;
  int right  = target.get_width();
  int bottom = target.get_height();

  if constexpr (requires { sdf.bounds(); })
  {
    Rect bounds = sdf.bounds();
    left   = std::max(left,   (int)std::floor(bounds.min_x));
    top    = std::max(top,    (int)std::floor(bounds.min_y));
    right  = std::min(right,  (int)std::ceil(bounds.max_x));
    bottom = std::min(bottom, (int)std::ceil(bounds.max_y));
  }

  render(
    target,
    left, top,
    right - left, bottom - top,
    sdf,
    color, opacity);
}

} // namespace oc
