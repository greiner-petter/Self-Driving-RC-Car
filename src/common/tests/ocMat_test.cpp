#include "../ocAssert.h"
#include "../ocMat.h"
#include "../ocVec.h"

int main()
{
  const float PI = 3.14159265358979f;

  oc_assert(distance(Mat2::rotate( PI * 0.5f) * Vec2(1, 0), Vec2( 0,  1)) < 0.001f);
  oc_assert(distance(Mat2::rotate(-PI * 0.5f) * Vec2(1, 0), Vec2( 0, -1)) < 0.001f);
  oc_assert(distance(Mat2::rotate( PI       ) * Vec2(1, 0), Vec2(-1,  0)) < 0.001f);

  oc_assert(distance(Mat3::rotate_x( PI * 0.5f) * Vec3(0, 1, 0), Vec3(0,  0,  1)) < 0.001f);
  oc_assert(distance(Mat3::rotate_x(-PI * 0.5f) * Vec3(0, 1, 0), Vec3(0,  0, -1)) < 0.001f);
  oc_assert(distance(Mat3::rotate_x( PI       ) * Vec3(0, 1, 0), Vec3(0, -1,  0)) < 0.001f);

  oc_assert(distance(Mat3::rotate_y( PI * 0.5f) * Vec3(0, 0, 1), Vec3( 1,  0, 0)) < 0.001f);
  oc_assert(distance(Mat3::rotate_y(-PI * 0.5f) * Vec3(0, 0, 1), Vec3(-1,  0, 0)) < 0.001f);
  oc_assert(distance(Mat3::rotate_y( PI       ) * Vec3(0, 0, 1), Vec3( 0, 0, -1)) < 0.001f);

  oc_assert(distance(Mat3::rotate_z( PI * 0.5f) * Vec3(1, 0, 0), Vec3( 0,  1, 0)) < 0.001f);
  oc_assert(distance(Mat3::rotate_z(-PI * 0.5f) * Vec3(1, 0, 0), Vec3( 0, -1, 0)) < 0.001f);
  oc_assert(distance(Mat3::rotate_z( PI       ) * Vec3(1, 0, 0), Vec3(-1,  0, 0)) < 0.001f);

  oc_assert(distance(Mat4::rotate_x( PI * 0.5f) * Vec4(0, 1, 0, 1), Vec4(0,  0,  1, 1)) < 0.001f);
  oc_assert(distance(Mat4::rotate_x(-PI * 0.5f) * Vec4(0, 1, 0, 1), Vec4(0,  0, -1, 1)) < 0.001f);
  oc_assert(distance(Mat4::rotate_x( PI       ) * Vec4(0, 1, 0, 1), Vec4(0, -1,  0, 1)) < 0.001f);

  oc_assert(distance(Mat4::rotate_y( PI * 0.5f) * Vec4(0, 0, 1, 1), Vec4( 1,  0, 0, 1)) < 0.001f);
  oc_assert(distance(Mat4::rotate_y(-PI * 0.5f) * Vec4(0, 0, 1, 1), Vec4(-1,  0, 0, 1)) < 0.001f);
  oc_assert(distance(Mat4::rotate_y( PI       ) * Vec4(0, 0, 1, 1), Vec4( 0, 0, -1, 1)) < 0.001f);

  oc_assert(distance(Mat4::rotate_z( PI * 0.5f) * Vec4(1, 0, 0, 1), Vec4( 0,  1, 0, 1)) < 0.001f);
  oc_assert(distance(Mat4::rotate_z(-PI * 0.5f) * Vec4(1, 0, 0, 1), Vec4( 0, -1, 0, 1)) < 0.001f);
  oc_assert(distance(Mat4::rotate_z( PI       ) * Vec4(1, 0, 0, 1), Vec4(-1,  0, 0, 1)) < 0.001f);

  oc_assert(distance(Mat3::scale(2, 3, 4) * Vec3(2, 2, 2), Vec3(4, 6, 8)) < 0.001f);
  oc_assert(distance(Mat4::scale(2, 3, 4, 5) * Vec4(2, 2, 2, 2), Vec4(4, 6, 8, 10)) < 0.001f);

  return 0;
}
