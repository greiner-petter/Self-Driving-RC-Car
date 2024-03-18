#pragma once

#include "ocVec.h"

#include <cmath>
#include <iostream>

struct Mat2 final
{
    union
    {
        float elements[4];
        Vec2  rows[2];
    };

    constexpr Mat2() = default;
    constexpr Mat2(float m00, float m10,
                   float m01, float m11);

    [[nodiscard]] constexpr static Mat2 identity();
    [[nodiscard]] constexpr static Mat2 from_rows(const Vec2& r1, const Vec2& r2);
    [[nodiscard]] constexpr static Mat2 from_cols(const Vec2& c1, const Vec2& c2);

    [[nodiscard]] static Mat2 rotate(float angle_rad);

    [[nodiscard]] constexpr static Mat2 scale(float x, float y);

    [[nodiscard]]
    constexpr float &operator()(size_t col, size_t row)       { return elements[row * 2 + col]; }
    [[nodiscard]]
    constexpr float  operator()(size_t col, size_t row) const { return elements[row * 2 + col]; }

    [[nodiscard]]
    constexpr Vec2 row(size_t index) const
    {
        return rows[index];
    }

    [[nodiscard]]
    constexpr Vec2 col(size_t index) const
    {
        return Vec2(elements[index + 0],
                    elements[index + 2]);
    }
};

struct Mat3 final
{
    union
    {
        float elements[9];
        Vec3  rows[3];
    };

    constexpr Mat3() = default;
    constexpr Mat3(float m00, float m10, float m20,
                   float m01, float m11, float m21,
                   float m02, float m12, float m22);

    [[nodiscard]] constexpr static Mat3 identity();
    [[nodiscard]] constexpr static Mat3 from_rows(const Vec3& r1, const Vec3& r2, const Vec3& r3);
    [[nodiscard]] constexpr static Mat3 from_cols(const Vec3& c1, const Vec3& c2, const Vec3& c3);

    [[nodiscard]] static Mat3 rotate_x(float angle_rad);
    [[nodiscard]] static Mat3 rotate_y(float angle_rad);
    [[nodiscard]] static Mat3 rotate_z(float angle_rad);
    [[nodiscard]] static Mat3 rotate(float angle_rad, const Vec3& axis);

    [[nodiscard]] constexpr static Mat3 translate(float x, float y);
    [[nodiscard]] constexpr static Mat3 translate(const Vec2& xy);

    [[nodiscard]] constexpr static Mat3 scale(float x, float y, float z);

    [[nodiscard]]
    constexpr float &operator()(size_t col, size_t row)       { return elements[row * 3 + col]; }
    [[nodiscard]]
    constexpr float  operator()(size_t col, size_t row) const { return elements[row * 3 + col]; }

    [[nodiscard]]
    constexpr Vec3 row(size_t index) const
    {
        return rows[index];
    }

    [[nodiscard]]
    constexpr Vec3 col(size_t index) const
    {
        return Vec3(elements[index + 0],
                    elements[index + 3],
                    elements[index + 6]);
    }
};

struct Mat4 final
{
    union
    {
        float elements[16];
        Vec4 rows[4];
    };

    constexpr Mat4() = default;
    constexpr Mat4(float m00, float m10, float m20, float m30,
                   float m01, float m11, float m21, float m31,
                   float m02, float m12, float m22, float m32,
                   float m03, float m13, float m23, float m33);

    [[nodiscard]] constexpr static Mat4 identity();
    [[nodiscard]] constexpr static Mat4 from_rows(const Vec4& r1, const Vec4& r2, const Vec4& r3, const Vec4& r4);
    [[nodiscard]] constexpr static Mat4 from_cols(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4);

    [[nodiscard]] static Mat4 rotate_x(float angle_rad);
    [[nodiscard]] static Mat4 rotate_y(float angle_rad);
    [[nodiscard]] static Mat4 rotate_z(float angle_rad);
    [[nodiscard]] static Mat4 rotate(float angle_rad, const Vec3& axis);

    [[nodiscard]] constexpr static Mat4 translate(float x, float y, float z);
    [[nodiscard]] constexpr static Mat4 translate(const Vec3& xyz);
    [[nodiscard]] constexpr static Mat4 translate(const Vec2& xy, float z);
    [[nodiscard]] constexpr static Mat4 translate(float x, const Vec2& yz);

    [[nodiscard]] constexpr static Mat4 scale(float x, float y, float z, float w);

    [[nodiscard]]
    constexpr float &operator()(size_t col, size_t row)       { return elements[row * 4 + col]; }
    [[nodiscard]]
    constexpr float  operator()(size_t col, size_t row) const { return elements[row * 4 + col]; }

    [[nodiscard]]
    constexpr Vec4 row(size_t index) const
    {
        return rows[index];
    }

    [[nodiscard]]
    constexpr Vec4 col(size_t index) const
    {
        return Vec4(elements[index + 0],
                    elements[index + 4],
                    elements[index + 8],
                    elements[index + 12]);
    }
};

constexpr Mat2::Mat2(float m00, float m10,
                     float m01, float m11)
{
    this->elements[0] = m00;
    this->elements[1] = m10;
    this->elements[2] = m01;
    this->elements[3] = m11;
}

constexpr Mat3::Mat3(float m00, float m10, float m20,
                     float m01, float m11, float m21,
                     float m02, float m12, float m22)
{
    this->elements[0] = m00;
    this->elements[1] = m10;
    this->elements[2] = m20;
    this->elements[3] = m01;
    this->elements[4] = m11;
    this->elements[5] = m21;
    this->elements[6] = m02;
    this->elements[7] = m12;
    this->elements[8] = m22;
}

constexpr Mat4::Mat4(float m00, float m10, float m20, float m30,
                     float m01, float m11, float m21, float m31,
                     float m02, float m12, float m22, float m32,
                     float m03, float m13, float m23, float m33)
{
    this->elements[ 0] = m00;
    this->elements[ 1] = m10;
    this->elements[ 2] = m20;
    this->elements[ 3] = m30;
    this->elements[ 4] = m01;
    this->elements[ 5] = m11;
    this->elements[ 6] = m21;
    this->elements[ 7] = m31;
    this->elements[ 8] = m02;
    this->elements[ 9] = m12;
    this->elements[10] = m22;
    this->elements[11] = m32;
    this->elements[12] = m03;
    this->elements[13] = m13;
    this->elements[14] = m23;
    this->elements[15] = m33;
}

[[nodiscard]]
constexpr Mat2 Mat2::from_rows(const Vec2& r1, const Vec2& r2)
{
    return Mat2(r1.x, r1.y,
                r2.x, r2.y);
}
[[nodiscard]]
constexpr Mat2 Mat2::from_cols(const Vec2& c1, const Vec2& c2)
{
    return Mat2(c1.x, c2.x,
                c1.y, c2.y);
}

[[nodiscard]]
constexpr Mat3 Mat3::from_rows(const Vec3& r1, const Vec3& r2, const Vec3& r3)
{
    return Mat3(r1.x, r1.y, r1.z,
                r2.x, r2.y, r2.z,
                r3.x, r3.y, r3.z);
}
[[nodiscard]]
constexpr Mat3 Mat3::from_cols(const Vec3& c1, const Vec3& c2, const Vec3& c3)
{
    return Mat3(c1.x, c2.x, c3.x,
                c1.y, c2.y, c3.y,
                c1.z, c2.z, c3.z);
}

[[nodiscard]]
constexpr Mat4 Mat4::from_rows(const Vec4& r1, const Vec4& r2, const Vec4& r3, const Vec4& r4)
{
    return Mat4(r1.x, r1.y, r1.z, r1.w,
                r2.x, r2.y, r2.z, r2.w,
                r3.x, r3.y, r3.z, r3.w,
                r4.x, r4.y, r4.z, r4.w);
}
[[nodiscard]]
constexpr Mat4 Mat4::from_cols(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4)
{
    return Mat4(c1.x, c2.x, c3.x, c4.x,
                c1.y, c2.y, c3.y, c4.y,
                c1.z, c2.z, c3.z, c4.z,
                c1.w, c2.w, c3.w, c4.w);
}

[[nodiscard]]
constexpr Mat2 Mat2::identity()
{
    return Mat2(1.0f, 0.0f,
                0.0f, 1.0f);
}
[[nodiscard]]
constexpr Mat3 Mat3::identity()
{
    return Mat3(1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f);
}
[[nodiscard]]
constexpr Mat4 Mat4::identity()
{
    return Mat4(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

[[nodiscard]]
inline Mat2 Mat2::rotate(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat2(ca, -sa,
                sa,  ca);
}

[[nodiscard]]
inline Mat3 Mat3::rotate_x(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat3(1.0f, 0.0f, 0.0f,
                0.0f,   ca,  -sa,
                0.0f,   sa,   ca);
}
[[nodiscard]]
inline Mat3 Mat3::rotate_y(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat3(  ca, 0.0f,   sa,
                0.0f, 1.0f, 0.0f,
                 -sa, 0.0f,   ca);
}
[[nodiscard]]
inline Mat3 Mat3::rotate_z(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat3(  ca,  -sa, 0.0f,
                  sa,   ca, 0.0f,
                0.0f, 0.0f, 1.0f);
}

[[nodiscard]]
inline Mat3 Mat3::rotate(float angle_rad, const Vec3& axis)
{
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);
    float mcx = (1.0f - cos_a) * axis.x;
    float mcy = (1.0f - cos_a) * axis.y;
    float mcz = (1.0f - cos_a) * axis.z;
    float tcx = axis.x * sin_a;
    float tcy = axis.y * sin_a;
    float tcz = axis.z * sin_a;
    return Mat3(mcx * axis.x + cos_a, mcx * axis.y - tcz,   mcx * axis.z + tcy,
                mcy * axis.x + tcz,   mcy * axis.y + cos_a, mcy * axis.z - tcx,
                mcz * axis.x - tcy,   mcz * axis.y + tcx,   mcz * axis.z + cos_a);
}

[[nodiscard]]
inline Mat4 Mat4::rotate_x(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat4(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f,   ca,  -sa, 0.0f,
                0.0f,   sa,   ca, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}
[[nodiscard]]
inline Mat4 Mat4::rotate_y(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat4(  ca, 0.0f,   sa, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                 -sa, 0.0f,   ca, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}
[[nodiscard]]
inline Mat4 Mat4::rotate_z(float angle_rad)
{
    float sa = sinf(angle_rad);
    float ca = cosf(angle_rad);
    return Mat4(  ca,  -sa, 0.0f, 0.0f,
                  sa,   ca, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

[[nodiscard]]
inline Mat4 Mat4::rotate(float angle_rad, const Vec3& axis)
{
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);
    float mcx = (1.0f - cos_a) * axis.x;
    float mcy = (1.0f - cos_a) * axis.y;
    float mcz = (1.0f - cos_a) * axis.z;
    float tcx = axis.x * sin_a;
    float tcy = axis.y * sin_a;
    float tcz = axis.z * sin_a;
    return Mat4(mcx * axis.x + cos_a, mcx * axis.y - tcz,   mcx * axis.z + tcy,   0.0f,
                mcy * axis.x + tcz,   mcy * axis.y + cos_a, mcy * axis.z - tcx,   0.0f,
                mcz * axis.x - tcy,   mcz * axis.y + tcx,   mcz * axis.z + cos_a, 0.0f,
                0.0f,                 0.0f,                 0.0f,                 1.0f);
}

[[nodiscard]]
constexpr Mat3 Mat3::translate(float x, float y)
{
    return Mat3(1.0f, 0.0f,    x,
                0.0f, 1.0f,    y,
                0.0f, 0.0f, 1.0f);
}
[[nodiscard]]
constexpr Mat3 Mat3::translate(const Vec2& xy)
{
    return Mat3(1.0f, 0.0f, xy.x,
                0.0f, 1.0f, xy.y,
                0.0f, 0.0f, 1.0f);
}

[[nodiscard]]
constexpr Mat4 Mat4::translate(float x, float y, float z)
{
    return Mat4(1.0f, 0.0f, 0.0f,    x,
                0.0f, 1.0f, 0.0f,    y,
                0.0f, 0.0f, 1.0f,    z,
                0.0f, 0.0f, 0.0f, 1.0f);
}
[[nodiscard]]
constexpr Mat4 Mat4::translate(const Vec3& xyz)
{
    return Mat4(1.0f, 0.0f, 0.0f, xyz.x,
                0.0f, 1.0f, 0.0f, xyz.y,
                0.0f, 0.0f, 1.0f, xyz.z,
                0.0f, 0.0f, 0.0f,  1.0f);
}
[[nodiscard]]
constexpr Mat4 Mat4::translate(const Vec2& xy, float z)
{
    return Mat4(1.0f, 0.0f, 0.0f, xy.x,
                0.0f, 1.0f, 0.0f, xy.y,
                0.0f, 0.0f, 1.0f,    z,
                0.0f, 0.0f, 0.0f, 1.0f);
}
[[nodiscard]]
constexpr Mat4 Mat4::translate(float x, const Vec2& yz)
{
    return Mat4(1.0f, 0.0f, 0.0f,    x,
                0.0f, 1.0f, 0.0f, yz.x,
                0.0f, 0.0f, 1.0f, yz.y,
                0.0f, 0.0f, 0.0f, 1.0f);
}

[[nodiscard]]
constexpr Mat2 Mat2::scale(float x, float y)
{
    return Mat2(   x, 0.0f,
                0.0f,    y);
}
[[nodiscard]]
constexpr Mat3 Mat3::scale(float x, float y, float z)
{
    return Mat3(   x, 0.0f, 0.0f,
                0.0f,    y, 0.0f,
                0.0f, 0.0f,    z);
}
[[nodiscard]]
constexpr Mat4 Mat4::scale(float x, float y, float z, float w)
{
    return Mat4(   x, 0.0f, 0.0f, 0.0f,
                0.0f,    y, 0.0f, 0.0f,
                0.0f, 0.0f,    z, 0.0f,
                0.0f, 0.0f, 0.0f,    w);
}

[[nodiscard]]
constexpr Mat2 operator+(const Mat2& m1, const Mat2& m2)
{
    Mat2 result;
    for (size_t y = 0; y < 2; ++y)
    for (size_t x = 0; x < 2; ++x)
    {
        result(x, y) = m1(x, y) + m2(x, y);
    }
    return result;
}
[[nodiscard]]
constexpr Mat3 operator+(const Mat3& m1, const Mat3& m2)
{
    Mat3 result;
    for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x)
    {
        result(x, y) = m1(x, y) + m2(x, y);
    }
    return result;
}
[[nodiscard]]
constexpr Mat4 operator+(const Mat4 &m1, const Mat4 &m2)
{
    Mat4 result;
    for (size_t y = 0; y < 4; ++y)
    for (size_t x = 0; x < 4; ++x)
    {
        result(x, y) = m1(x, y) + m2(x, y);
    }
    return result;
}

[[nodiscard]]
constexpr Mat2 operator-(const Mat2& m1, const Mat2& m2)
{
    Mat2 result;
    for (size_t y = 0; y < 2; ++y)
    for (size_t x = 0; x < 2; ++x)
    {
        result(x, y) = m1(x, y) - m2(x, y);
    }
    return result;
}
[[nodiscard]]
constexpr Mat3 operator-(const Mat3& m1, const Mat3& m2)
{
    Mat3 result;
    for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x)
    {
        result(x, y) = m1(x, y) - m2(x, y);
    }
    return result;
}
[[nodiscard]]
constexpr Mat4 operator-(const Mat4& m1, const Mat4& m2)
{
    Mat4 result;
    for (size_t y = 0; y < 4; ++y)
    for (size_t x = 0; x < 4; ++x)
    {
        result(x, y) = m1(x, y) - m2(x, y);
    }
    return result;
}

[[nodiscard]]
constexpr Mat2 operator*(const Mat2& m1, const Mat2& m2)
{
    Mat2 result;
    for (size_t y = 0; y < 2; ++y)
    for (size_t x = 0; x < 2; ++x)
    {
        result(x, y) = dot(m1.row(y), m2.col(x));
    }
    return result;
}
[[nodiscard]]
constexpr Mat3 operator*(const Mat3& m1, const Mat3& m2)
{
    Mat3 result;
    for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x)
    {
        result(x, y) = dot(m1.row(y), m2.col(x));
    }
    return result;
}
[[nodiscard]]
constexpr Mat4 operator*(const Mat4& m1, const Mat4& m2)
{
    Mat4 result;
    for (size_t y = 0; y < 4; ++y)
    for (size_t x = 0; x < 4; ++x)
    {
        result(x, y) = dot(m1.row(y), m2.col(x));
    }
    return result;
}
[[nodiscard]]
constexpr Vec2 operator*(const Mat2& m, const Vec2& v)
{
    Vec2 result;
    for (size_t y = 0; y < 2; ++y)
    {
        result(y) = dot(m.row(y), v);
    }
    return result;
}
[[nodiscard]]
constexpr Vec3 operator*(const Mat3& m, const Vec3& v)
{
    Vec3 result;
    for (size_t y = 0; y < 3; ++y)
    {
        result(y) = dot(m.row(y), v);
    }
    return result;
}
[[nodiscard]]
constexpr Vec4 operator*(const Mat4& m, const Vec4& v)
{
    Vec4 result;
    for (size_t y = 0; y < 4; ++y)
    {
        result(y) = dot(m.row(y), v);
    }
    return result;
}
[[nodiscard]]
constexpr Mat2 operator*(const Mat2& m, float f)
{
    Mat2 result;
    for (size_t y = 0; y < 2; ++y)
    for (size_t x = 0; x < 2; ++x)
    {
        result(x, y) = m(x, y) * f;
    }
    return result;
}
[[nodiscard]]
constexpr Mat3 operator*(const Mat3& m, float f)
{
    Mat3 result;
    for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x)
    {
        result(x, y) = m(x, y) * f;
    }
    return result;
}
[[nodiscard]]
constexpr Mat4 operator*(const Mat4& m, float f)
{
    Mat4 result;
    for (size_t y = 0; y < 4; ++y)
    for (size_t x = 0; x < 4; ++x)
    {
        result(x, y) = m(x, y) * f;
    }
    return result;
}

[[nodiscard]] constexpr Mat2 operator*(float f, const Mat2& m) { return m * f; }
[[nodiscard]] constexpr Mat3 operator*(float f, const Mat3& m) { return m * f; }
[[nodiscard]] constexpr Mat4 operator*(float f, const Mat4& m) { return m * f; }

constexpr void operator*=(Mat2 &m1, const Mat2& m2) { m1 = m1 * m2; }
constexpr void operator*=(Mat3 &m1, const Mat3& m2) { m1 = m1 * m2; }
constexpr void operator*=(Mat4 &m1, const Mat4& m2) { m1 = m1 * m2; }
constexpr void operator*=(Mat2 &m, float f) { m = m * f; }
constexpr void operator*=(Mat3 &m, float f) { m = m * f; }
constexpr void operator*=(Mat4 &m, float f) { m = m * f; }

[[nodiscard]]
constexpr Mat2 operator/(const Mat2& m, float f)
{
    Mat2 result;
    for (size_t y = 0; y < 2; ++y)
    for (size_t x = 0; x < 2; ++x)
    {
        result(x, y) = m(x, y) / f;
    }
    return result;
}
[[nodiscard]]
constexpr Mat3 operator/(const Mat3& m, float f)
{
    Mat3 result;
    for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x)
    {
        result(x, y) = m(x, y) / f;
    }
    return result;
}
[[nodiscard]]
constexpr Mat4 operator/(const Mat4& m, float f)
{
    Mat4 result;
    for (size_t y = 0; y < 4; ++y)
    for (size_t x = 0; x < 4; ++x)
    {
        result(x, y) = m(x, y) / f;
    }
    return result;
}

constexpr void operator/=(Mat2 &m, float f) { m = m / f; }
constexpr void operator/=(Mat3 &m, float f) { m = m / f; }
constexpr void operator/=(Mat4 &m, float f) { m = m / f; }

[[nodiscard]]
constexpr bool operator==(const Mat2& v1, const Mat2& v2)
{
    return v1(0, 0) == v2(0, 0) && v1(1, 0) == v2(1, 0)
        && v1(0, 1) == v2(0, 1) && v1(1, 1) == v2(1, 1);
}
[[nodiscard]]
constexpr bool operator==(const Mat3& v1, const Mat3& v2)
{
    return v1(0, 0) == v2(0, 0) && v1(1, 0) == v2(1, 0) && v1(2, 0) == v2(2, 0)
        && v1(0, 1) == v2(0, 1) && v1(1, 1) == v2(1, 1) && v1(2, 1) == v2(2, 1)
        && v1(0, 2) == v2(0, 2) && v1(1, 2) == v2(1, 2) && v1(2, 2) == v2(2, 2);
}
[[nodiscard]]
constexpr bool operator==(const Mat4& v1, const Mat4& v2)
{
    return v1(0, 0) == v2(0, 0) && v1(1, 0) == v2(1, 0) && v1(2, 0) == v2(2, 0) && v1(3, 0) == v2(3, 0)
        && v1(0, 1) == v2(0, 1) && v1(1, 1) == v2(1, 1) && v1(2, 1) == v2(2, 1) && v1(3, 1) == v2(3, 1)
        && v1(0, 2) == v2(0, 2) && v1(1, 2) == v2(1, 2) && v1(2, 2) == v2(2, 2) && v1(3, 2) == v2(3, 2)
        && v1(0, 3) == v2(0, 3) && v1(1, 3) == v2(1, 3) && v1(2, 3) == v2(2, 3) && v1(3, 3) == v2(3, 3);
}

[[nodiscard]]
constexpr bool operator!=(const Mat2& v1, const Mat2& v2)
{
    return v1(0, 0) != v2(0, 0) || v1(1, 0) != v2(1, 0)
        || v1(0, 1) != v2(0, 1) || v1(1, 1) != v2(1, 1);
}
[[nodiscard]]
constexpr bool operator!=(const Mat3& v1, const Mat3& v2)
{
    return v1(0, 0) != v2(0, 0) || v1(1, 0) != v2(1, 0) || v1(2, 0) != v2(2, 0)
        || v1(0, 1) != v2(0, 1) || v1(1, 1) != v2(1, 1) || v1(2, 1) != v2(2, 1)
        || v1(0, 2) != v2(0, 2) || v1(1, 2) != v2(1, 2) || v1(2, 2) != v2(2, 2);
}
[[nodiscard]]
constexpr bool operator!=(const Mat4& v1, const Mat4& v2)
{
    return v1(0, 0) != v2(0, 0) || v1(1, 0) != v2(1, 0) || v1(2, 0) != v2(2, 0) || v1(3, 0) != v2(3, 0)
        || v1(0, 1) != v2(0, 1) || v1(1, 1) != v2(1, 1) || v1(2, 1) != v2(2, 1) || v1(3, 1) != v2(3, 1)
        || v1(0, 2) != v2(0, 2) || v1(1, 2) != v2(1, 2) || v1(2, 2) != v2(2, 2) || v1(3, 2) != v2(3, 2)
        || v1(0, 3) != v2(0, 3) || v1(1, 3) != v2(1, 3) || v1(2, 3) != v2(2, 3) || v1(3, 3) != v2(3, 3);
}

[[nodiscard]]
constexpr Mat2 transpose(const Mat2& m)
{
    Mat2 result;
    for (size_t y = 0; y < 2; ++y)
    for (size_t x = 0; x < 2; ++x)
    {
        result(x, y) = m(y, x);
    }
    return result;
}
[[nodiscard]]
constexpr Mat3 transpose(const Mat3& m)
{
    Mat3 result;
    for (size_t y = 0; y < 3; ++y)
    for (size_t x = 0; x < 3; ++x)
    {
        result(x, y) = m(y, x);
    }
    return result;
}
[[nodiscard]]
constexpr Mat4 transpose(const Mat4& m)
{
    Mat4 result;
    for (size_t y = 0; y < 4; ++y)
    for (size_t x = 0; x < 4; ++x)
    {
        result(x, y) = m(y, x);
    }
    return result;
}

[[nodiscard]]
constexpr float determinant(const Mat2& m)
{
    return m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
}

[[nodiscard]]
constexpr float determinant(const Mat3& m)
{
    return m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2))
         - m(1, 0) * (m(0, 1) * m(2, 2) - m(2, 1) * m(0, 2))
         + m(2, 0) * (m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2));
}

[[nodiscard]]
constexpr float determinant(const Mat4& m)
{
    return m(0, 0) * (m(1, 1) * (m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3))
                    - m(2, 1) * (m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3))
                    + m(3, 1) * (m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3)))
         - m(1, 0) * (m(0, 1) * (m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3))
                    - m(2, 1) * (m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3))
                    + m(3, 1) * (m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3)))
         + m(2, 0) * (m(0, 1) * (m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3))
                    - m(1, 1) * (m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3))
                    + m(3, 1) * (m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3)))
         - m(3, 0) * (m(0, 1) * (m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3))
                    - m(1, 1) * (m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3))
                    + m(2, 1) * (m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3)));
}

[[nodiscard]]
constexpr Mat2 inverse(const Mat2& m)
{
    float inv_det = 1.0f / determinant(m);
    return {
         m(1, 1) * inv_det,
        -m(1, 0) * inv_det,
        -m(0, 1) * inv_det,
         m(0, 0) * inv_det
    };
}

[[nodiscard]]
constexpr Mat3 inverse(const Mat3& m)
{
    float inv_det = 1.0f / determinant(m);
    return {
        (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) * inv_det,
        (m(0, 1) * m(2, 2) - m(0, 2) * m(2, 1)) * inv_det,
        (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * inv_det,
        (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) * inv_det,
        (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * inv_det,
        (m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) * inv_det,
        (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)) * inv_det,
        (m(0, 0) * m(2, 1) - m(0, 1) * m(2, 0)) * inv_det,
        (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * inv_det
    };
}

[[nodiscard]]
constexpr Mat4 inverse(const Mat4& m)
{
    float inv_det = 1.0f / determinant(m);
    float tmp[] = {
        m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3),
        m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3),
        m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3),
        m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3),
        m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3),
        m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3),

        m(3, 1) * m(2, 0) - m(2, 1) * m(3, 0),
        m(3, 1) * m(1, 0) - m(1, 1) * m(3, 0),
        m(2, 1) * m(1, 0) - m(1, 1) * m(2, 0),
        m(3, 1) * m(0, 0) - m(0, 1) * m(3, 0),
        m(2, 1) * m(0, 0) - m(0, 1) * m(2, 0),
        m(1, 1) * m(0, 0) - m(0, 1) * m(1, 0)
    };
    return {
        inv_det * (m(1, 1) * tmp[ 0] - m(2, 1) * tmp[ 1] + m(3, 1) * tmp[ 2]),
        inv_det * (m(2, 1) * tmp[ 3] - m(0, 1) * tmp[ 0] - m(3, 1) * tmp[ 4]),
        inv_det * (m(0, 1) * tmp[ 1] - m(1, 1) * tmp[ 3] + m(3, 1) * tmp[ 5]),
        inv_det * (m(1, 1) * tmp[ 4] - m(0, 1) * tmp[ 2] - m(2, 1) * tmp[ 5]),

        inv_det * (m(2, 0) * tmp[ 1] - m(1, 0) * tmp[ 0] - m(3, 0) * tmp[ 2]),
        inv_det * (m(0, 0) * tmp[ 0] - m(2, 0) * tmp[ 3] + m(3, 0) * tmp[ 4]),
        inv_det * (m(1, 0) * tmp[ 3] - m(0, 0) * tmp[ 1] - m(3, 0) * tmp[ 5]),
        inv_det * (m(0, 0) * tmp[ 2] - m(1, 0) * tmp[ 4] + m(2, 0) * tmp[ 5]),

        inv_det * (m(1, 3) * tmp[ 6] - m(2, 3) * tmp[ 7] + m(3, 3) * tmp[ 8]),
        inv_det * (m(2, 3) * tmp[ 9] - m(0, 3) * tmp[ 6] - m(3, 3) * tmp[10]),
        inv_det * (m(0, 3) * tmp[ 7] - m(1, 3) * tmp[ 9] + m(3, 3) * tmp[11]),
        inv_det * (m(1, 3) * tmp[10] - m(0, 3) * tmp[ 8] - m(2, 3) * tmp[11]),

        inv_det * (m(2, 2) * tmp[ 7] - m(1, 2) * tmp[ 6] - m(3, 2) * tmp[ 8]),
        inv_det * (m(0, 2) * tmp[ 6] - m(2, 2) * tmp[ 9] + m(3, 2) * tmp[10]),
        inv_det * (m(1, 2) * tmp[ 9] - m(0, 2) * tmp[ 7] - m(3, 2) * tmp[11]),
        inv_det * (m(0, 2) * tmp[ 8] - m(1, 2) * tmp[10] + m(2, 2) * tmp[11])
    };
}
