#pragma once

#include "ocMat.h"
#include "ocVec.h"

#include <cmath>

struct ocPose final
{
  /**
   * position in 3-dimensional space relative to the parent reference frames
   * origin point.
   */
  Vec3 pos;

  /**
   * rotation in Euler-angles relative to the parent reference frame.
   * Listed in the order they are applied when entering this reference frame.
   */
  union
  {
    struct { float rot_z, rot_y, rot_x; };
    struct { float yaw, pitch, roll; };
    struct { float heading, elevation, bank; };
  };

  ocPose() = default;

  ocPose(
    float x,  float y,  float z,
    float rz, float ry, float rx)
  {
    pos.x = x;
    pos.y = y;
    pos.z = z;
    rot_z = rz;
    rot_y = ry;
    rot_x = rx;
  }

  ocPose(
    Vec3 xyz,
    float rz, float ry, float rx)
  {
    pos   = xyz;
    rot_z = rz;
    rot_y = ry;
    rot_x = rx;
  }

  ocPose(Mat4 mat)
  {
    const float PI = 3.14159265358979f;
    float sin_pitch = mat(0, 2);
    pos = mat.col(3).xyz();
    if (1 == sin_pitch || -1 == sin_pitch)
    {
      // in case of a (-)90° pitch, we can't distinguish between yawing and
      // rolling, so we assume the rolling to be 0 and recover yaw and pitch.
      pitch = -sin_pitch * PI * 0.5f;
      yaw   = std::atan2(-mat(1, 0), mat(1, 1));
      roll  = 0;
    }
    else
    {
      // asin is enough for the pitch, since a pitch > 90° is indistinguishable
      // from a pitch < 90° with different yaw and roll.
      pitch = std::asin(-sin_pitch);
      yaw   = std::atan2(mat(0, 1), mat(0, 0));
      roll  = std::atan2(mat(1, 2), mat(2, 2));
    }
  }

  static ocPose compose(ocPose parent, ocPose child)
  {
    return ocPose(parent.get_generalize_mat() * child.get_generalize_mat());
  }

  /**
   * Returns the matrix which will turn vectors from the parent reference frame
   * into vectors in this reference frame.
   */
  Mat4 get_specialize_mat() const
  {
    return Mat4::rotate_x(-roll)
         * Mat4::rotate_y(-pitch)
         * Mat4::rotate_z(-yaw)
         * Mat4::translate(-pos);
  }

  /**
   * Returns the matrix which will turn vectors from this reference frame into
   * vectors in the parent reference frame.
   */
  Mat4 get_generalize_mat() const
  {
    return Mat4::translate(pos)
         * Mat4::rotate_z(yaw)
         * Mat4::rotate_y(pitch)
         * Mat4::rotate_x(roll);
  }

  Vec3 specialize_pos(Vec3 position) const
  {
    return Mat3::rotate_x(-roll)
         * Mat3::rotate_y(-pitch)
         * Mat3::rotate_z(-yaw)
         * (position - pos);
  }
  Vec3 specialize_dir(Vec3 direction) const
  {
    return Mat3::rotate_x(-roll)
         * Mat3::rotate_y(-pitch)
         * Mat3::rotate_z(-yaw)
         * direction;
  }
  Vec4 specialize(Vec4 vec) const
  {
    return get_specialize_mat() * vec;
  }

  Vec3 generalize_pos(Vec3 position) const
  {
    return Mat3::rotate_z(yaw)
         * Mat3::rotate_y(pitch)
         * Mat3::rotate_x(roll)
         * position
         + pos;
  }
  Vec3 generalize_dir(Vec3 direction) const
  {
    return Mat3::rotate_z(yaw)
         * Mat3::rotate_y(pitch)
         * Mat3::rotate_x(roll)
         * direction;
  }
  Vec4 generalize(Vec4 vec) const
  {
    return get_generalize_mat() * vec;
  }

  Vec3 x_axis() const
  {
    return generalize_dir({1.0f, 0.0f, 0.0f});
  }

  Vec3 y_axis() const
  {
    return generalize_dir({0.0f, 1.0f, 0.0f});
  }

  Vec3 z_axis() const
  {
    return generalize_dir({0.0f, 0.0f, 1.0f});
  }
};
