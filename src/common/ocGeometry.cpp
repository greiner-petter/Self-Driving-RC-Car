#include "ocGeometry.h"

#include <cmath>

float intersect_ray_box(const Ray& ray, const Box& box)
{
  Vec3 ro = box.pose.specialize_pos(ray.origin);
  Vec3 rd = box.pose.specialize_dir(ray.direction);

  Vec3 bounds[] = {
    box.size * -0.5f,
    box.size *  0.5f,
  };

  float txmin = (bounds[rd.x < 0].x - ro.x) / rd.x;
  float txmax = (bounds[0 <= rd.x].x - ro.x) / rd.x;
  float tymin = (bounds[rd.y < 0].y - ro.y) / rd.y;
  float tymax = (bounds[0 <= rd.y].y - ro.y) / rd.y;
  if (txmin < tymax && tymin < txmax) // condition will be false in case of any NaN
  {
    float tmin = std::max(txmin, tymin);
    float tmax = std::min(txmax, tymax);
    float tzmin = (bounds[rd.z < 0].z - ro.z) / rd.z;
    float tzmax = (bounds[0 <= rd.z].z - ro.z) / rd.z;
    if (tmin < tzmax && tzmin < tmax) // condition will be false in case of any NaN
    {
      float tmin2 = std::max(tzmin, tmin);
      //float tmax2 = std::min(tzmax, tmax);
      if (0.0001f < tmin2)
      {
        return tmin2;
      }
    }
  }
  return INFINITY;
}

float intersect_ray_plane(const Ray& ray, const Plane& plane)
{
  auto d = dot(plane.facing_direction, ray.direction);
  auto t = -(dot(plane.facing_direction, ray.origin) - plane.offset) / d;
  if (0.0001f < t) return t;
  return INFINITY;
}

float distance_to_box(const Vec3& point, const Box& box)
{
  Vec3 p = box.pose.specialize_pos(point);
  Vec3 q = {
    std::abs(p.x) - box.size.x * 0.5f,
    std::abs(p.y) - box.size.y * 0.5f,
    std::abs(p.z) - box.size.z * 0.5f
  };
  float inside_dist  = std::min(0.0f, std::max(q.x, std::max(q.y, p.z)));
  Vec3 m = {
    std::max(0.0f, q.x),
    std::max(0.0f, q.y),
    std::max(0.0f, q.z)
  };
  float outside_dist = length(m);
  return inside_dist + outside_dist;
}

float distance_to_plane(const Vec3& point, const Plane& plane)
{
  return dot(point, plane.facing_direction) - plane.offset;
}


int circle_circle_intersection(
    Vec2 circle0_center,
    float circle0_radius,
    Vec2 circle1_center,
    float circle1_radius,
    Vec2 *result0,
    Vec2 *result1)
{
    Vec2 diff = circle1_center - circle0_center;
    float dist = length(diff);
    float y = (circle0_radius * circle0_radius - circle1_radius * circle1_radius + dist * dist) / (2.0f * dist);
    float x_sq = circle0_radius * circle0_radius - y * y;
    if (0 == x_sq)
    {
        *result0 = circle0_center + diff * (circle0_radius / dist);
        return 1;
    }
    else if (0 < x_sq)
    {
        float x = sqrtf(x_sq);
        Vec2 norm = diff / dist;
        Vec2 perp = right(norm);
        *result0 = circle0_center + norm * y + perp * x;
        *result1 = circle0_center + norm * y - perp * x;
        return 2;
    }
    return 0;
}

int line_line_intersection(
    Vec2 line0_a, Vec2 line0_b,
    Vec2 line1_a, Vec2 line1_b,
    Vec2 *result)
{
    Vec2 d0 = line0_b - line0_a;
    Vec2 d1 = line1_b - line1_a;
    Vec2 d2 = line0_a - line1_a;
    float det = d0.x * d1.y - d0.y * d1.x;
    float s =  (d0.x * d2.y - d0.y * d2.x) / det;
    float t =  (d1.x * d2.y - d1.y * d2.x) / det;
    if (0.0f != det && 0.0f <= s && s <= 1.0f && 0.0f <= t && t <= 1.0f)
    {
        if (result) *result = line0_a + d0 * t;
        return 1;
    }
    return 0;
}

int line_circle_intersection(
    Vec2 line_a, Vec2 line_b,
    Vec2 circle_center, float circle_radius,
    Vec2 *result0,
    Vec2 *result1)
{
    Vec2 dif = line_b - line_a;
    float len = length(dif);
    Vec2 line_norm = dif / len;
    float to_circle_center_parr = dot(line_norm, circle_center - line_a);
    float to_circle_center_perp = dot(line_norm, right(circle_center - line_a));

    if (circle_radius < to_circle_center_perp) return 0;
    if (circle_radius == to_circle_center_perp)
    {
        if (0.0f <= to_circle_center_parr && to_circle_center_parr <= len)
        {
            if (result0) *result0 = line_a + line_norm * to_circle_center_parr;
            return 1;
        }
        return 0;
    }
    float o = sqrtf(circle_radius * circle_radius - to_circle_center_perp * to_circle_center_perp);
    float d0 = to_circle_center_parr - o;
    float d1 = to_circle_center_parr + o;
    if (0.0f <= d0 && d0 <= len)
    {
        if (0.0f <= d1 && d1 <= len)
        {
            if (result0) *result0 = line_a + line_norm * d0;
            if (result1) *result1 = line_a + line_norm * d1;
            return 2;
        }
        if (result0) *result0 = line_a + line_norm * d0;
        return 1;
    }
    if (0.0f <= d1 && d1 <= len)
    {
        if (result0) *result0 = line_a + line_norm * d1;
        return 1;
    }
    return 0;
}

// This section computes a circle from the three points. It's actually quite simple math,
// but the implementation is still kinda big. The calculation works line this:
// - We have points a, b and c on the circumference of the circle and want to find its
//   center and radius
// - We draw a line from a to b and from b to c
// - In the middle of those lines we draw two new lines that are 90 degree rotated
// - Where these new lines meet is the center of the circle
// - We get the radius from the distance of the center to point a (or b or c really)
int circle_from_points(
    Vec2 a, Vec2 b, Vec2 c,
    Vec2 *center, float *radius)
{
    Vec2 ab = b - a;
    Vec2 bc = c - b;
    Vec2 d = a + ab * 0.5f;
    Vec2 e = b + bc * 0.5f;

    float bc_dot_ed   = dot(b, d - e);
    float ab_cross_bc = dot(ab, right(bc));
    if (0.0f != bc_dot_ed && 0.0f != ab_cross_bc)
    {
      float n = bc_dot_ed / ab_cross_bc;
      *center = d + right(ab) * n;
      *radius = length(a - *center);
      return 1;
    }
    return 0;
}
