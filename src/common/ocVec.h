#pragma once

#include <cmath>
#include <iostream>

struct Vec2 final
{
    union
    {
      struct { float x, y; };
      float elements[2];
    };

    constexpr Vec2() = default;
    constexpr Vec2(float x, float y);

    [[nodiscard]] constexpr float  operator()(size_t i) const { return elements[i]; }
    [[nodiscard]] constexpr float &operator()(size_t i)       { return elements[i]; }

    constexpr void operator+=(const Vec2& v);
    constexpr void operator-=(const Vec2& v);
    constexpr void operator*=(const Vec2& v);
    constexpr void operator*=(float f);
    constexpr void operator/=(const Vec2& v);
    constexpr void operator/=(float f);
};

struct Vec3 final
{
    union
    {
      struct { float x, y, z; };
      float elements[3];
    };

    constexpr Vec3() = default;
    constexpr Vec3(float x, float y, float z);
    constexpr Vec3(const Vec2& xy, float z);
    constexpr Vec3(float x, const Vec2& yz);

    [[nodiscard]] constexpr float  operator()(size_t i) const { return elements[i]; }
    [[nodiscard]] constexpr float &operator()(size_t i)       { return elements[i]; }

    [[nodiscard]] constexpr Vec2 xy() const { return Vec2(x, y); }
    [[nodiscard]] constexpr Vec2 xz() const { return Vec2(x, z); }
    [[nodiscard]] constexpr Vec2 yz() const { return Vec2(y, z); }

    constexpr void operator+=(const Vec3& v);
    constexpr void operator-=(const Vec3& v);
    constexpr void operator*=(const Vec3& v);
    constexpr void operator*=(float f);
    constexpr void operator/=(const Vec3& v);
    constexpr void operator/=(float f);
};

struct Vec4 final
{
    union
    {
      struct { float x, y, z, w; };
      float elements[4];
    };

    constexpr Vec4() = default;
    constexpr Vec4(float x, float y, float z, float w);
    constexpr Vec4(const Vec2& xy, float z, float w);
    constexpr Vec4(float x, float y, const Vec2& zw);
    constexpr Vec4(const Vec2& xy, const Vec2& zw);
    constexpr Vec4(float x, const Vec2& yz, float w);
    constexpr Vec4(const Vec3& xyz, float w);
    constexpr Vec4(float x, const Vec3& yzw);

    [[nodiscard]] constexpr float  operator()(size_t i) const { return elements[i]; }
    [[nodiscard]] constexpr float &operator()(size_t i)       { return elements[i]; }

    [[nodiscard]] constexpr Vec2 xy() const { return Vec2(x, y); }
    [[nodiscard]] constexpr Vec2 yz() const { return Vec2(y, z); }
    [[nodiscard]] constexpr Vec2 yw() const { return Vec2(z, w); }

    [[nodiscard]] constexpr Vec3 xyz() const { return Vec3(x, y, z); }
    [[nodiscard]] constexpr Vec3 yzw() const { return Vec3(y, z, w); }

    constexpr void operator+=(const Vec4& v);
    constexpr void operator-=(const Vec4& v);
    constexpr void operator*=(const Vec4& v);
    constexpr void operator*=(float f);
    constexpr void operator/=(const Vec4& v);
    constexpr void operator/=(float f);
};

constexpr Vec2::Vec2(float x, float y)
{
    this->x = x;
    this->y = y;
}
constexpr Vec3::Vec3(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}
constexpr Vec3::Vec3(const Vec2& xy, float z)
{
    this->x = xy.x;
    this->y = xy.y;
    this->z = z;
}
constexpr Vec3::Vec3(float x, const Vec2& yz)
{
    this->x = x;
    this->y = yz.x;
    this->z = yz.y;
}
constexpr Vec4::Vec4(float x, float y, float z, float w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}
constexpr Vec4::Vec4(const Vec2& xy, float z, float w)
{
    this->x = xy.x;
    this->y = xy.y;
    this->z = z;
    this->w = w;
}
constexpr Vec4::Vec4(float x, const Vec2& yz, float w)
{
    this->x = x;
    this->y = yz.x;
    this->z = yz.y;
    this->w = w;
}
constexpr Vec4::Vec4(float x, float y, const Vec2& zw)
{
    this->x = x;
    this->y = y;
    this->z = zw.x;
    this->w = zw.y;
}
constexpr Vec4::Vec4(const Vec2& xy, const Vec2& zw)
{
    this->x = xy.x;
    this->y = xy.y;
    this->z = zw.x;
    this->w = zw.y;
}
constexpr Vec4::Vec4(const Vec3& xyz, float w)
{
    this->x = xyz.x;
    this->y = xyz.y;
    this->z = xyz.z;
    this->w = w;
}
constexpr Vec4::Vec4(float x, const Vec3& yzw)
{
    this->x = x;
    this->y = yzw.x;
    this->z = yzw.y;
    this->w = yzw.z;
}

[[nodiscard]] inline Vec2 angle_to_vector(float radians) { return Vec2(cosf(radians), sinf(radians)); }
[[nodiscard]] inline float vector_to_angle(Vec2 vec) { return atan2f(vec.y, vec.x); }

[[nodiscard]] constexpr const Vec2& operator+(const Vec2& v) { return v; }
[[nodiscard]] constexpr const Vec3& operator+(const Vec3& v) { return v; }
[[nodiscard]] constexpr const Vec4& operator+(const Vec4& v) { return v; }

[[nodiscard]] constexpr Vec2 operator+(Vec2 v1, const Vec2& v2) { v1 += v2; return v1; }
[[nodiscard]] constexpr Vec3 operator+(Vec3 v1, const Vec3& v2) { v1 += v2; return v1; }
[[nodiscard]] constexpr Vec4 operator+(Vec4 v1, const Vec4& v2) { v1 += v2; return v1; }

constexpr void Vec2::operator+=(const Vec2& v) { x += v.x; y += v.y; }
constexpr void Vec3::operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; }
constexpr void Vec4::operator+=(const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; }

[[nodiscard]] constexpr Vec2 operator-(const Vec2& v) { return Vec2(-v.x, -v.y); }
[[nodiscard]] constexpr Vec3 operator-(const Vec3& v) { return Vec3(-v.x, -v.y, -v.z); }
[[nodiscard]] constexpr Vec4 operator-(const Vec4& v) { return Vec4(-v.x, -v.y, -v.z, -v.w); }

[[nodiscard]] constexpr Vec2 operator-(Vec2 v1, const Vec2& v2) { v1 -= v2; return v1; }
[[nodiscard]] constexpr Vec3 operator-(Vec3 v1, const Vec3& v2) { v1 -= v2; return v1; }
[[nodiscard]] constexpr Vec4 operator-(Vec4 v1, const Vec4& v2) { v1 -= v2; return v1; }

constexpr void Vec2::operator-=(const Vec2& v) { x -= v.x; y -= v.y;  }
constexpr void Vec3::operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; }
constexpr void Vec4::operator-=(const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; }

[[nodiscard]] constexpr Vec2 operator*(Vec2 v1, const Vec2& v2) { v1 *= v2; return v1;  }
[[nodiscard]] constexpr Vec3 operator*(Vec3 v1, const Vec3& v2) { v1 *= v2; return v1;  }
[[nodiscard]] constexpr Vec4 operator*(Vec4 v1, const Vec4& v2) { v1 *= v2; return v1;  }

[[nodiscard]] constexpr Vec2 operator*(Vec2 v, float f) { v *= f; return v; }
[[nodiscard]] constexpr Vec3 operator*(Vec3 v, float f) { v *= f; return v; }
[[nodiscard]] constexpr Vec4 operator*(Vec4 v, float f) { v *= f; return v; }

[[nodiscard]] constexpr Vec2 operator*(float f, Vec2 v) { v *= f; return v; }
[[nodiscard]] constexpr Vec3 operator*(float f, Vec3 v) { v *= f; return v; }
[[nodiscard]] constexpr Vec4 operator*(float f, Vec4 v) { v *= f; return v; }

constexpr void Vec2::operator*=(const Vec2& v) { x *= v.x; y *= v.y; }
constexpr void Vec3::operator*=(const Vec3& v) { x *= v.x; y *= v.y; z *= v.z; }
constexpr void Vec4::operator*=(const Vec4& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; }
constexpr void Vec2::operator*=(float f) { x *= f; y *= f; }
constexpr void Vec3::operator*=(float f) { x *= f; y *= f; z *= f; }
constexpr void Vec4::operator*=(float f) { x *= f; y *= f; z *= f; w *= f; }

[[nodiscard]] constexpr Vec2 operator/(Vec2 v1, const Vec2& v2) { v1 /= v2; return v1; }
[[nodiscard]] constexpr Vec3 operator/(Vec3 v1, const Vec3& v2) { v1 /= v2; return v1; }
[[nodiscard]] constexpr Vec4 operator/(Vec4 v1, const Vec4& v2) { v1 /= v2; return v1; }

[[nodiscard]] constexpr Vec2 operator/(Vec2  v, float f) { v /= f; return v; }
[[nodiscard]] constexpr Vec3 operator/(Vec3  v, float f) { v /= f; return v; }
[[nodiscard]] constexpr Vec4 operator/(Vec4  v, float f) { v /= f; return v; }

constexpr void Vec2::operator/=(const Vec2& v) { x /= v.x; y /= v.y; }
constexpr void Vec3::operator/=(const Vec3& v) { x /= v.x; y /= v.y; z /= v.z; }
constexpr void Vec4::operator/=(const Vec4& v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; }
constexpr void Vec2::operator/=(float f) { x /= f; y /= f; }
constexpr void Vec3::operator/=(float f) { x /= f; y /= f; z /= f; }
constexpr void Vec4::operator/=(float f) { x /= f; y /= f; z /= f; w /= f; }

[[nodiscard]]
constexpr bool operator==(const Vec2& v1, const Vec2& v2)
{
  return v1.x == v2.x
      && v1.y == v2.y;
}
[[nodiscard]]
constexpr bool operator==(const Vec3& v1, const Vec3& v2)
{
  return v1.x == v2.x
      && v1.y == v2.y
      && v1.z == v2.z;
}
[[nodiscard]]
constexpr bool operator==(const Vec4& v1, const Vec4& v2)
{
  return v1.x == v2.x
      && v1.y == v2.y
      && v1.z == v2.z
      && v1.w == v2.w;
}

[[nodiscard]]
constexpr bool operator!=(const Vec2& v1, const Vec2& v2)
{
  return v1.x != v2.x
      || v1.y != v2.y;
}
[[nodiscard]]
constexpr bool operator!=(const Vec3& v1, const Vec3& v2)
{
  return v1.x != v2.x
      || v1.y != v2.y
      || v1.z != v2.z;
}
[[nodiscard]]
constexpr bool operator!=(const Vec4& v1, const Vec4& v2)
{
  return v1.x != v2.x
      || v1.y != v2.y
      || v1.z != v2.z
      || v1.w != v2.w;
}

[[nodiscard]]
constexpr float dot(const Vec2& v1, const Vec2& v2)
{
  return v1.x * v2.x
       + v1.y * v2.y;
}

[[nodiscard]]
constexpr float dot(const Vec3& v1, const Vec3& v2)
{
  return v1.x * v2.x
       + v1.y * v2.y
       + v1.z * v2.z;
}

[[nodiscard]]
constexpr float dot(const Vec4& v1, const Vec4& v2)
{
  return v1.x * v2.x
       + v1.y * v2.y
       + v1.z * v2.z
       + v1.w * v2.w;
}

[[nodiscard]]
constexpr float cross(const Vec2& v1, const Vec2& v2)
{
  return (v1.x * v2.y) - (v1.y * v2.x);
}

[[nodiscard]]
constexpr Vec3 cross(const Vec3& v1, const Vec3& v2)
{
  return Vec3(
    (v1.y * v2.z) - (v1.z * v2.y),
    (v1.z * v2.x) - (v1.x * v2.z),
    (v1.x * v2.y) - (v1.y * v2.x));
}

[[nodiscard]] constexpr float length_squared(const Vec2& v) { return dot(v, v); }
[[nodiscard]] constexpr float length_squared(const Vec3& v) { return dot(v, v); }
[[nodiscard]] constexpr float length_squared(const Vec4& v) { return dot(v, v); }

[[nodiscard]] inline float length(const Vec2& v) { return sqrtf(length_squared(v)); }
[[nodiscard]] inline float length(const Vec3& v) { return sqrtf(length_squared(v)); }
[[nodiscard]] inline float length(const Vec4& v) { return sqrtf(length_squared(v)); }

[[nodiscard]] inline Vec2 normalize(const Vec2& v) { return v / length(v); }
[[nodiscard]] inline Vec3 normalize(const Vec3& v) { return v / length(v); }
[[nodiscard]] inline Vec4 normalize(const Vec4& v) { return v / length(v); }

[[nodiscard]] inline float distance(const Vec2& a, const Vec2& b) { return length(a - b); }
[[nodiscard]] inline float distance(const Vec3& a, const Vec3& b) { return length(a - b); }
[[nodiscard]] inline float distance(const Vec4& a, const Vec4& b) { return length(a - b); }

[[nodiscard]] constexpr Vec2  left(const Vec2& v) { return Vec2( v.y, -v.x); }
[[nodiscard]] constexpr Vec2 right(const Vec2& v) { return Vec2(-v.y,  v.x); }

[[nodiscard]] inline Vec2 floor(const Vec2& v) { return {std::floor(v.x), std::floor(v.y)}; }
[[nodiscard]] inline Vec3 floor(const Vec3& v) { return {std::floor(v.x), std::floor(v.y), std::floor(v.z)}; }
[[nodiscard]] inline Vec4 floor(const Vec4& v) { return {std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w)}; }
[[nodiscard]] inline Vec2 ceil (const Vec2& v) { return {std::ceil (v.x), std::ceil (v.y)}; }
[[nodiscard]] inline Vec3 ceil (const Vec3& v) { return {std::ceil (v.x), std::ceil (v.y), std::ceil (v.z)}; }
[[nodiscard]] inline Vec4 ceil (const Vec4& v) { return {std::ceil (v.x), std::ceil (v.y), std::ceil (v.z), std::ceil (v.w)}; }
[[nodiscard]] inline Vec2 round(const Vec2& v) { return {std::round(v.x), std::round(v.y)}; }
[[nodiscard]] inline Vec3 round(const Vec3& v) { return {std::round(v.x), std::round(v.y), std::round(v.z)}; }
[[nodiscard]] inline Vec4 round(const Vec4& v) { return {std::round(v.x), std::round(v.y), std::round(v.z), std::round(v.w)}; }

inline std::ostream &operator<<(std::ostream &out, const Vec2& v)
{
  out << "Vec2(" << v.x << ", " << v.y << ")";
  return out;
}
inline std::ostream &operator<<(std::ostream &out, const Vec3& v)
{
  out << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
  return out;
}
inline std::ostream &operator<<(std::ostream &out, const Vec4& v)
{
  out << "Vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
  return out;
}
