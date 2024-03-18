#include "../ocAssert.h"
#include "../ocVec.h"

int main()
{
  oc_assert(Vec2(5, 6).x == 5.0f);
  oc_assert(Vec2(5, 6).y == 6.0f);

  oc_assert(Vec2(5, 6)(0) == 5.0f);
  oc_assert(Vec2(5, 6)(1) == 6.0f);

  oc_assert(Vec2(5, 6).elements[0] == 5.0f);
  oc_assert(Vec2(5, 6).elements[1] == 6.0f);

  oc_assert(Vec2(1, 2) == Vec2(1, 2));
  oc_assert(!(Vec2(1, 2) != Vec2(1, 2)));
  oc_assert(Vec2(1, 2) != Vec2(1, 1));
  oc_assert(!(Vec2(1, 2) == Vec2(1, 1)));
  oc_assert(Vec2(1, 2) != Vec2(2, 2));
  oc_assert(!(Vec2(1, 2) == Vec2(2, 2)));
  oc_assert(Vec2(1, 2) != Vec2(4, 4));
  oc_assert(!(Vec2(1, 2) == Vec2(4, 4)));

  oc_assert(Vec2(1, 2) + Vec2(3, 4) == Vec2(4, 6));
  oc_assert(Vec2(4, 4) - Vec2(1, 2) == Vec2(3, 2));
  oc_assert(Vec2(3, 2) * Vec2(4, 2) == Vec2(12, 4));
  oc_assert(Vec2(3, 2) * 2.0f == Vec2(6, 4));
  oc_assert(Vec2(12, 14) / Vec2(3, 7) == Vec2(4, 2));
  oc_assert(Vec2(12, 14) / 2.0f == Vec2(6, 7));

  oc_assert(left(Vec2(1, 2)) == Vec2(2, -1));
  oc_assert(right(Vec2(1, 2)) == Vec2(-2, 1));

  oc_assert(Vec3(Vec2(1, 2), 3) == Vec3(1, 2, 3));
  oc_assert(Vec3(1, Vec2(2, 3)) == Vec3(1, 2, 3));


  oc_assert(Vec4(Vec2(1, 2), 3, 4) == Vec4(1, 2, 3, 4));
  oc_assert(Vec4(1, Vec2(2, 3), 4) == Vec4(1, 2, 3, 4));
  oc_assert(Vec4(1, 2, Vec2(3, 4)) == Vec4(1, 2, 3, 4));
  oc_assert(Vec4(Vec2(1, 2), Vec2(3, 4)) == Vec4(1, 2, 3, 4));
  oc_assert(Vec4(Vec3(1, 2, 3), 4) == Vec4(1, 2, 3, 4));
  oc_assert(Vec4(1, Vec3(2, 3, 4)) == Vec4(1, 2, 3, 4));

  return 0;
}
