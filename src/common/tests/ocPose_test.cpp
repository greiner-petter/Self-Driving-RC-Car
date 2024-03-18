#include "../ocAssert.h"
#include "../ocCommon.h"
#include "../ocPose.h"

#include <cmath>

int main()
{
  for (int i = 0; i < 1000; ++i)
  {
    float x = random_float(-100.0f, 100.0f);
    float y = random_float(-100.0f, 100.0f);
    float z = random_float(-100.0f, 100.0f);
    float yaw   = random_float((float)-M_PI, (float)M_PI);
    float pitch = random_float((float)-M_PI, (float)M_PI);
    float roll  = random_float((float)-M_PI, (float)M_PI);

    ocPose pose(x, y, z, yaw, pitch, roll);

    Mat4 spec_mat = pose.get_specialize_mat();
    Mat4 gene_mat = pose.get_generalize_mat();

    for (int j = 0; j < 10; ++j)
    {
      Vec3 v(random_float(-100.0f, 100.0f),
             random_float(-100.0f, 100.0f),
             random_float(-100.0f, 100.0f));

      // Test that specialize&generalize pos&dir do thie same as the 4x4 matrix
      Vec3 spec_pos0 = (spec_mat * Vec4(v, 1.0f)).xyz();
      Vec3 spec_pos1 = pose.specialize_pos(v);
      oc_assert(distance(spec_pos0, spec_pos1) < 0.0001f, spec_pos0, spec_pos1);

      Vec3 spec_dir0 = (spec_mat * Vec4(v, 0.0f)).xyz();
      Vec3 spec_dir1 = pose.specialize_dir(v);
      oc_assert(distance(spec_dir0, spec_dir1) < 0.0001f, spec_dir0, spec_dir1);

      Vec3 gene_pos0 = (gene_mat * Vec4(v, 1.0f)).xyz();
      Vec3 gene_pos1 = pose.generalize_pos(v);
      oc_assert(distance(gene_pos0, gene_pos1) < 0.0001f, gene_pos0, gene_pos1);

      Vec3 gene_dir0 = (gene_mat * Vec4(v, 0.0f)).xyz();
      Vec3 gene_dir1 = pose.generalize_dir(v);
      oc_assert(distance(gene_dir0, gene_dir1) < 0.0001f, gene_dir0, gene_dir1);

      // Test that specialize and generalize are each others inverse
      Vec3 result0 = (gene_mat * spec_mat * Vec4(v, 1.0f)).xyz();
      Vec3 result1 = (spec_mat * gene_mat * Vec4(v, 1.0f)).xyz();
      oc_assert(distance(result0, v) < 0.0001f, result0, v);
      oc_assert(distance(result1, v) < 0.0001f, result1, v);

      Vec3 result2 = (gene_mat * spec_mat * Vec4(v, 0.0f)).xyz();
      Vec3 result3 = (spec_mat * gene_mat * Vec4(v, 0.0f)).xyz();
      oc_assert(distance(result2, v) < 0.0001f, result2, v);
      oc_assert(distance(result3, v) < 0.0001f, result3, v);

      Vec3 result4 = pose.generalize_pos(pose.specialize_pos(v));
      Vec3 result5 = pose.specialize_pos(pose.generalize_pos(v));
      oc_assert(distance(result4, v) < 0.0001f, result4, v);
      oc_assert(distance(result5, v) < 0.0001f, result5, v);

      Vec3 result6 = pose.generalize_dir(pose.specialize_dir(v));
      Vec3 result7 = pose.specialize_dir(pose.generalize_dir(v));
      oc_assert(distance(result6, v) < 0.0001f, result6, v);
      oc_assert(distance(result7, v) < 0.0001f, result7, v);
    }
  }

  return 0;
}
