#include "ocMat.h"
#include "ocPose.h"
#include "ocVec.h"

struct Ray
{
  Vec3 origin;
  Vec3 direction;
};

struct Box
{
  ocPose pose;
  Vec3 size;
};

struct Plane
{
  Vec3 facing_direction;
  float offset;
};

// Intersection functions between a ray and a shape return the distance from the rays origin to
// the closest point of intersection. If no intersection occurs, INFINITY is returned.
float intersect_ray_box(const Ray& ray, const Box& box);
float intersect_ray_plane(const Ray& ray, const Plane& plane);

// Distance from the given point to the closest point on the surface of the shape. If the point
// is inside the shape, the distance is negative.
float distance_to_box(const Vec3& point, const Box& box);
float distance_to_plane(const Vec3& point, const Plane& plane);


int circle_circle_intersection(
    Vec2 circle0_center, float circle0_radius,
    Vec2 circle1_center, float circle1_radius,
    Vec2 *result0,
    Vec2 *result1);

int line_line_intersection(
    Vec2 line0_a, Vec2 line0_b,
    Vec2 line1_a, Vec2 line1_b,
    Vec2 *result);

int line_circle_intersection(
    Vec2 line_a, Vec2 line_b,
    Vec2 circle_center, float circle_radius,
    Vec2 *result0,
    Vec2 *result1);

int circle_from_points(
    Vec2 a, Vec2 b, Vec2 c,
    Vec2 *center, float *radius);
