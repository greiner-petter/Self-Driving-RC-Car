#define f2(x, y) ((float2)(x, y))
#define f3(x, y, z) ((float3)(x, y, z))
#define f4(x, y, z, w) ((float4)(x, y, z, w))
#define i2(x, y) ((int2)(x, y))
#define left(v) (v.yx * f2(1, -1))
#define right(v) (v.yx * f2(-1, 1))

#define EPSILON 0.00001f

typedef   signed  char   int8_t;
typedef unsigned  char  uint8_t;
typedef   signed short  int16_t;
typedef unsigned short uint16_t;
typedef   signed   int  int32_t;
typedef unsigned   int uint32_t;

typedef enum
{
  Color_White  = 0, // RAL 9016
  Color_Black  = 1, // RAL 9017
  Color_Yellow = 2, // RAL 1003
  Color_Red    = 3, // RAL 3020
  Color_Blue   = 4, // RAL 5017
  Color_Error  = 5,
  Color_Obstacle = 6
} Color;

__constant float3 color_table[6] = {
  f3(0.885240f, 0.884938f, 0.848794f), // white
  f3(0.053546f, 0.051445f, 0.050623f), // black
  f3(1.000000f, 0.462233f, 0.008481f), // yellow
  f3(0.610911f, 0.031382f, 0.004836f), // red
  f3(0.000017f, 0.104505f, 0.260678f), // blue
  f3(1.0f, 0.0f, 1.0f), // error
  f3(0.729f, 0.09f, 0.122f)
};

typedef struct
{
  int type;
  float3 pos;
  float3 facing;
  float3 right;
  float3 up;
  float3 size;
} Object;

typedef struct
{
  float3 origin;
  float3 normal;
} Ray;

typedef struct
{
  float3 pos;
  float3 dir_forward;
  float3 dir_right;
  float3 dir_up;
} Camera;

typedef struct
{
  uint3 u;
} random_state;
uint3 pcg3d(uint3 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v ^= v >> 16u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  return v;
}

float3 rand_float3(random_state *rng) {
  // can't do more than 24 bit due to float precision.
  // Divide by 0xFFFFFF + 1 to make sure the result is never 1.0
  float3 result;
  rng->u = pcg3d(rng->u);
  result.x = (float)(rng->u.x & 0xFFFFFF) / 16777216.0f;
  result.y = (float)(rng->u.y & 0xFFFFFF) / 16777216.0f;
  result.z = (float)(rng->u.z & 0xFFFFFF) / 16777216.0f;
  return result;
}

bool intersect_ray_object(
    const Ray *r,
    const Object *obj,
    float *result_d,
    float3 *result_n,
    float3 *result_uvw)
{
  // inspired by https://www.shadertoy.com/view/ld23DV
  float3 origin = f3(
      dot(r->origin - obj->pos, obj->facing),
      dot(r->origin - obj->pos, obj->right),
      dot(r->origin - obj->pos, obj->up));
  float3 normal = f3(
      dot(r->normal, obj->facing),
      dot(r->normal, obj->right),
      dot(r->normal, obj->up));
  float3 half_size = f3(obj->size.x * 0.5, obj->size.y * 0.5, obj->size.z * 0.5);

  float3 tmin = (-copysign(half_size, normal) - origin) / normal;
  float3 tmax = ( copysign(half_size, normal) - origin) / normal;
  float tnear = max(tmin.x, max(tmin.y, tmin.z));
  float tfar  = min(tmax.x, min(tmax.y, tmax.z));

  if (tnear < tfar && EPSILON < tnear && tnear < *result_d) // condition will be false in case of any NaN
  {
      *result_d = tnear;
      float3 n = -sign(normal) * step(tmin.yzx, tmin.xyz) * step(tmin.zxy, tmin.xyz);
      *result_n = obj->facing * n.x + obj->right * n.y + obj->up * n.z;
      float3 p = origin + normal * tnear;
      *result_uvw = f3(p.x, p.y, p.z) / half_size;
      return true;
  }
  return false;
}

float road_straight_outer(float2 p)
{
  float apy = fabs(p.y);
  return (41.0f < apy && apy <= 43.0f) ? 1.0f : 0.0f;
}

float road_straight_dashed_gaps(float2 p)
{
  return (fmod(fabs(p.x) + 10.0f, 40.0f) < 20.0f) ? 1.0f : 0.0f;
}

float road_straight_center(float2 p)
{
  return (fabs(p.y) <= 1.0f) ? 1.0f : 0.0f;
}

float road_straight_double_center(float2 p)
{
  float apy = fabs(p.y);
  return (1.0f < apy && apy <= 3.0f) ? 1.0f : 0.0f;
}

float road_straight_double_dashed_center(float2 p)
{
  float solid  = ( 1.0f < p.y && p.y <=  3.0f) ? 1.0f : 0.0f;
  float dashed = (-3.0f < p.y && p.y <= -1.0f) ? 1.0f : 0.0f;
  return solid + dashed * road_straight_dashed_gaps(p);
}

float road_straight(float2 p)
{
  return road_straight_outer(p) + road_straight_center(p) * road_straight_dashed_gaps(p);
}

float road_no_overtake_straight(float2 p)
{
  return road_straight_outer(p) + road_straight_double_center(p);
}

float road_no_overtake_left_straight(float2 p)
{
  return road_straight_outer(p) + road_straight_double_dashed_center(p);
}

float road_curve_outer(float2 p)
{
  float2 dp = p - f2(-100.0f, 100.0f);
  float d_sq = dot(dp, dp);
  float inner = ( 57.0f *  57.0f < d_sq && d_sq <  59.0f *  59.0f) ? 1.0f : 0.0f;
  float outer = (141.0f * 141.0f < d_sq && d_sq < 143.0f * 143.0f) ? 1.0f : 0.0f;
  return inner + outer;
}

float road_curve_dashed_gaps(float2 p)
{
  float2 dp  = p - f2(-100.0f, 100.0f);
  float gap0 = (fabs(dp.x + dp.y * 0.19891925f) <  9.993576672f) ? 1.0f : 0.0f;
  float gap1 = (fabs(dp.x + dp.y * 0.66819002f) < 11.787973542f) ? 1.0f : 0.0f;
  float gap2 = (fabs(dp.x + dp.y * 1.49658027f) < 17.641648668f) ? 1.0f : 0.0f;
  float gap3 = (fabs(dp.x + dp.y * 5.02716556f) < 50.239364428f) ? 1.0f : 0.0f;
  return gap0 + gap1 + gap2 + gap3;
}

float road_curve_center(float2 p)
{
  float2 dp  = p - f2(-100.0f, 100.0f);
  float d_sq = dot(dp, dp);
  return (99.0f * 99.0f < d_sq && d_sq <= 101.0f * 101.0f) ? 1.0f : 0.0f;
}

float road_curve_double_center(float2 p)
{
  float2 dp  = p - f2(-100.0f, 100.0f);
  float d_sq = dot(dp, dp);
  float line0 = ( 97.0f *  97.0f < d_sq && d_sq <=  99.0f *  99.0f) ? 1.0f : 0.0f;
  float line1 = (101.0f * 101.0f < d_sq && d_sq <= 103.0f * 103.0f) ? 1.0f : 0.0f;
  return line0 + line1;
}

float road_curve(float2 p)
{
  return road_curve_outer(p) + road_curve_center(p) * road_curve_dashed_gaps(p);
}

float road_no_overtake_curve(float2 p)
{
  return road_curve_outer(p) + road_curve_double_center(p);
}

float road_no_overtake_left_curve(float2 p)
{
  float2 dp  = p - f2(-100.0f, 100.0f);
  float d_sq = dot(dp, dp);
  float line0 = ( 97.0f *  97.0f < d_sq && d_sq <=  99.0f *  99.0f) ? 1.0f : 0.0f;
  float line1 = (101.0f * 101.0f < d_sq && d_sq <= 103.0f * 103.0f) ? 1.0f : 0.0f;
  return road_curve_outer(p) + line0 + line1 * road_curve_dashed_gaps(p);
}

float road_no_overtake_right_curve(float2 p)
{
  float2 dp  = p - f2(-100.0f, 100.0f);
  float d_sq = dot(dp, dp);
  float line0 = ( 97.0f *  97.0f < d_sq && d_sq <=  99.0f *  99.0f) ? 1.0f : 0.0f;
  float line1 = (101.0f * 101.0f < d_sq && d_sq <= 103.0f * 103.0f) ? 1.0f : 0.0f;
  return road_curve_outer(p) + line0 * road_curve_dashed_gaps(p) + line1;
}

float road_intersection(float2 p)
{
  float2 ap = fabs(p);
  ap = (ap.x < ap.y) ? ap.yx : ap;
  float outer  = (41.0f < ap.y && ap.y <= 43.0f) ? 1.0f : 0.0f;
  float center = (ap.y < 1.0f) ? 1.0f : 0.0f;
  float gap0   = (41.0f < ap.x && ap.x <= 50.0f) ? 1.0f : 0.0f;
  float gap1   = (70.0f < ap.x && ap.x <= 90.0f) ? 1.0f : 0.0f;
  return outer + center * (gap0 + gap1);
}

float road_intersection_turn(float2 p)
{
  if (-43 < p.x && p.x <= 43 && -43 < p.y && p.y <= 43)
  {
    float2 dp = p - f2(-41, 41);
    float d_sq = dot(dp, dp);
    if (40 * 40 < d_sq && d_sq <= 42 * 42)
    {
      if (fabs(dp.x       ) < 16.0f / 1.0f    * 0.5f) return 1.0f;
      if (fabs(dp.x + dp.y) < 16.0f / 0.7071f * 0.5f) return 1.0f;
      if (fabs(       dp.y) < 16.0f / 1.0f    * 0.5f) return 1.0f;
      return 0.0f;
    }
    if (82 * 82 < d_sq && d_sq <= 84 * 84)
    {
      if (fabs(dp.x                       ) < 20.0f / 1.0f   * 0.5f) return 1.0f;
      if (fabs(dp.x + dp.y * 0.5f / 0.866f) < 20.0f / 0.866f * 0.5f) return 1.0f;
      if (fabs(dp.x + dp.y * 0.866f / 0.5f) < 20.0f / 0.5f   * 0.5f) return 1.0f;
      if (fabs(       dp.y                ) < 20.0f / 1.0f   * 0.5f) return 1.0f;
      return 0.0f;
    }
  }
  return 0.0f;
}

float road_parking(float2 p)
{
  float2 ap = fabs(p);
  if (p.y < -43)
  {
    if (-93 < p.y)
    {
      float f = (p.y + 41) * (4.0f / 7.0f) + 100 - ap.x; // ~60°
      if (0 < f && f <= 2.3f) return 1.0f;
      if (p.y <= -91)
      {
        if (ap.x <= 69) return 1.0f;
      }
      else
      {
        if (ap.x <= 1) return 1.0f;
        if (34 < ap.x && ap.x <= 36) return 1.0f;
        if (69 < ap.x && ap.x <= 71) return 1.0f;
        float f2 = (p.y + 41.5f) * (4.0f / 7.0f) - 1 - p.x; // ~60°
        if (0 < f2 && f2 <= 2.3f) return 1.0f;
        float f3 = (p.y + 41.5f) * (4.0f / 7.0f) + 34 + p.x; // ~60°
        if (0 < f3 && f3 <= 2.3f) return 1.0f;
      }
    }
  }
  if (43 < p.y)
  {
    if (p.y <= 73)
    {
      float f = (41 - p.y) * (4.0f / 7.0f) + 100 - ap.x; // ~60°
      if (0 < f && f <= 2.3f) return 1.0f;
      if (71 < p.y)
      {
        if (ap.x <= 81) return 1.0f;
      }
      else
      {
        if (20 < ap.x && ap.x <= 22) return 1.0f;
        if (80 < ap.x && ap.x <= 82) return 1.0f;
        float f2 = (p.y - 41.5f) * (7.0f / 4.0f) - 76 - p.x; // ~30°
        if (0 < f2 && f2 <= 4.0f) return 1.0f;
        float f3 = (p.y - 41.5f) * (7.0f / 4.0f) + 26 + p.x; // ~30°
        if (0 < f3 && f3 <= 4.0f) return 1.0f;
      }
    }
  }
  return 0.0f;
}

float road_start_line(float2 p)
{
  if (-41.0f < p.y && p.y < 41.0f && 94.0f < p.x)
  {
    int ix = (int)round(p.x / 2.0f + 0.5f);
    int iy = (int)round(p.y / 2.0f + 0.5f);
    return ((ix ^ iy) & 1) ? 1.0f : 0.0f;
  }
  return 0.0f;
}

float road_start_box(float2 p)
{
  if (-43.0f < p.y && p.y < -41.0f) return 1.0f;
  if ( -1.0f < p.y && p.y <   1.0f &&  0.0f < p.x) return 1.0f;
  if (-43.0f < p.y && p.y <   1.0f && 98.0f < p.x) return 1.0f;
  return 0.0f;
}

float3 get_road(__global uint8_t *map, int2 map_size, float2 p)
{
  int2 pi = convert_int2_rte(p / 200.0f);
  if (pi.x < 0 || map_size.x <= pi.x || pi.y < 0 || map_size.y <= pi.y) return f3(0.0f, 0.0f, 0.0f);
  int i = pi.x + pi.y * map_size.x;
  float2 rp = fmod(p + 100.0f, f2(200.0f, 200.0f)) - 100.0f;
  float road = 0.25f;
  switch (map[i] & 0x03)
  {
    case 0: break;
    case 1: rp =  left(rp); break;
    case 2: rp =      -rp;  break;
    case 3: rp = right(rp); break;
  }
  switch (map[i] >> 2)
  {
    case  0: road = 0.0f; break;
    case  1: road = road_straight(rp); break;
    case  2: road = road_curve(rp); break;
    case  3: road = road_intersection(rp); break;
    case  4: road = road_straight(rp); break;
    case  5: road = road_straight(rp); break;
    case  6: road = road_no_overtake_straight(rp); break;
    case  7: road = road_no_overtake_curve(rp); break;
    case  8: road = road_no_overtake_left_straight(rp); break;
    case  9: road = road_no_overtake_left_curve(rp); break;
    case 10: road = road_no_overtake_right_curve(rp); break;
    case 11: road = road_intersection(rp); break;
    case 12: road = road_intersection(rp); break;
    case 13: road = road_intersection(rp) + road_intersection_turn(rp); break;
    case 14: road = road_straight(rp) + road_parking(rp); break;
    case 15: road = road_straight(rp) + road_start_line(rp); break;
    case 16: road = road_curve(rp) + road_start_line(-rp) + road_start_box(rp); break;
    // TODO: pedestrian island
  }
  // TODO: floor noise
  return f3(road, road, road);
}

bool big_square_sign_mask(float3 uvw)
{
  float2 q = fabs(uvw.xy) - f2(0.8f, 0.8f);
  float d = min(max(q.x, q.y), 0.0f) + length(max(q, f2(0.0f, 0.0f))) - 0.2f;
  return d < 0.0f;
}
bool small_square_sign_mask(float3 uvw)
{
  float2 q = fabs(uvw.xy) - f2(0.8f, 0.8f);
  float d = min(max(q.x, q.y), 0.0f) + length(max(q, f2(0.0f, 0.0f))) - 0.2f;
  return d < 0.0f;
}
bool round_sign_mask(float3 uvw)
{
  return dot(uvw.xy, uvw.xy) < 1.0f;
}
bool stop_sign_mask(float3 uvw)
{
  const float k1 = sqrt(2.0f);
  const float k2 = 1.0f / sqrt(2.0f);
  float d = max(max(fabs(uvw.x), fabs(uvw.y)) - 1.0f, (fabs(uvw.x) + fabs(uvw.y) - k1) * k2);
  return d < 0.0f;
}
bool priority_sign_mask(float3 uvw)
{
  const float k = 1.0f / sqrt(2.0f);
  float d = (fabs(uvw.x) + fabs(uvw.y) - 1.0f) * k;
  return d < 0.0f;
}
bool triangle_sign_mask(float3 uvw)
{
  const float k = 1.0f / sqrt(3.0f);
  float d = max(fabs(uvw.x) - 1.0f + ((1.0f - uvw.y) * k), uvw.y - 1.0f);
  return d < 0.0f;
}

bool marking_start_line(float3 uvw)
{
  return false; // TODO
}
bool marking_stop_line(float3 uvw)
{
  return true;
}
bool marking_yield_line(float3 uvw)
{
  if ((uvw.y <= -0.65f) ||
      (-0.35f < uvw.y && uvw.y <= 0.05f) ||
      ( 0.35f < uvw.y && uvw.y <= 0.75f))
  {
    return true;
  }
  return false;
}
bool marking_speed_limit_start(float3 uvw)
{
  return false; // TODO
}
bool marking_speed_limit_end(float3 uvw)
{
  return false; // TODO
}
bool marking_barred_area(float3 uvw)
{
  float f = (uvw.y * 15.0f - 41) * (4.0f / 7.0f) + 100.0f - fabs(uvw.x * 100.0f); // ~60°
  if (0.0f < f)
  {
    if ((uvw.y < -0.87f) ||
       (f <= 2.3f) ||
       (fmod(uvw.y * 15.0f * (25.5f / 13.0f) + uvw.x * 100.0f + 1000.0f, 23.8f) < 8.8f)) // ~27°
    {
      return true;
    }
  }
  return false;
}
bool marking_crosswalk(float3 uvw)
{
  return (fmod(fabs(uvw.y) * 41.0f + 2.0f, 8.0f) < 4.0f);
}
bool marking_turn_right(float3 uvw)
{
  uvw *= f3(25.0f, 3.5f, 1.0f);
  if (-10.5f < uvw.x && 1.5f < uvw.y)
  { // arrow stem
    return true;
  }
  if (-14.5f < uvw.x && uvw.x <= -10.5f && -0.5f < uvw.y)
  { // outer curve
    float2 d = uvw.xy - f2(-10.5f, -0.5f);
    return (dot(d, d) <= 4.0f * 4.0f);
  }
  if (-10.5f < uvw.x && uvw.x <= -8.5f && -0.5f < uvw.y && uvw.y <= 1.5f)
  { // inner curve
    float2 d = uvw.xy - f2(-8.5f, -0.5f);
    return (2.0f * 2.0f < dot(d, d));
  }
  if (uvw.y <= -0.5f)
  { // arrow head
    if (0.0f < (uvw.x + 12.5f) + (12.5f / 3.0f) * (uvw.y + 3.5f) &&
        (uvw.x + 12.5f) + (12.5f / 3.0f) * -(uvw.y + 3.5f) <= 0.0f)
    {
      return true;
    }
  }
  return false;
}
bool marking_turn_left(float3 uvw)
{
  uvw.y = -uvw.y;
  return marking_turn_right(uvw);
}

bool check_object_mask(int object_type, float3 uvw)
{
  switch(object_type)
  {
    case 0x0011: // Sign_Speed_Limit_Start
    case 0x0012: // Sign_Speed_Limit_End
    case 0x0013: // Sign_Crosswalk
    case 0x0014: // Sign_Parking_Zone
    case 0x0015: // Sign_Expressway_Start
    case 0x0016: // Sign_Expressway_End
      return big_square_sign_mask(uvw.yzx);

    case 0x0017: // Sign_Sharp_Turn_Left
    case 0x0018: // Sign_Sharp_Turn_Right
      return small_square_sign_mask(uvw.yzx);

    case 0x0019: // Sign_Barred_Area
    case 0x001A: // Sign_Pedestrian_Island
    case 0x001E: // Sign_Left
    case 0x001F: // Sign_Right
    case 0x0020: // Sign_No_Passing_Start
    case 0x0021: // Sign_No_Passing_End
      return round_sign_mask(uvw.yzx);

    case 0x001B: // Sign_Stop
      return stop_sign_mask(uvw.yzx);

    case 0x001C: // Sign_Priority
      return priority_sign_mask(uvw.yzx);

    case 0x001D: // Sign_Yield
      return triangle_sign_mask(uvw.yzx);

    case 0x0022: // Sign_Uphill
    case 0x0023: // Sign_Downhill
      return triangle_sign_mask(-uvw.yzx);

    case 0x0041: return marking_start_line(uvw.xyz);
    case 0x0042: return marking_stop_line(uvw.xyz);
    case 0x0043: return marking_yield_line(uvw.xyz);
    case 0x0044: return marking_speed_limit_start(uvw.xyz);
    case 0x0045: return marking_speed_limit_end(uvw.xyz);
    case 0x0046: return marking_barred_area(uvw.xyz);
    case 0x0047: return marking_crosswalk(uvw.xyz);
    case 0x0049: return marking_turn_left(uvw.xyz);
    case 0x004A: return marking_turn_right(uvw.xyz);

    case 0x0081: // pedestrian
    case 0x0085: // obstacle
      return true;
  }
  return true;
}

Color sign_parking_zone(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  float2 q = fabs(uvw.xy) - f2(0.8f, 0.8f);
  float d = min(max(q.x, q.y), 0.0f) + length(max(q, f2(0.0f, 0.0f))) - 0.2f;
  if (d < -0.048f)
  {
    if (0.2f < uvw.x)
    {
      if (uvw.x < 0.42f && -0.69f < uvw.y && uvw.y < 0.69f)
      {
        return Color_White;
      }
    }
    else if (-0.13f < uvw.x)
    {
      if ( 0.47f < uvw.y && uvw.y < 0.69f) return Color_White;
      if (-0.15f < uvw.y && uvw.y < 0.07f) return Color_White;
    }
    else
    {
      float2 d1 = uvw.xy - f2(-0.13f, 0.27f);
      float dist_sq = dot(d1, d1);
      if (0.2f * 0.2f < dist_sq && dist_sq < 0.42f * 0.42f)
      {
        return Color_White;
      }
    }
    return Color_Blue;
  }
  return Color_White;
}

Color sign_sharp_turn_left(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  float f = (uvw.x - floor(uvw.x * 0.5f) * 2.0f) + fabs(uvw.y);
  if (f - floor(f * 0.5f) * 2.0f < 1.0f) return Color_Red;
  return Color_White;
}

Color sign_barred_area(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  float d_sq = dot(uvw.xy, uvw.xy);
  // inner white circle
  if (d_sq < 0.7f * 0.7f)
  {
    Color arrow_color = Color_Black;
    if (uvw.x < 0.0f)
    {
      arrow_color = Color_Red;
      uvw *= f3(-1.0f, -1.0f, 1.0f);
    }
    // arrow stems
    if (-0.39f < uvw.y && uvw.y < 0.39f &&
        0.37f - 0.15f < uvw.x && uvw.x < 0.37f)
    {
      return arrow_color;
    }
    // arrow tip
    else if (0.29f - 0.26f < uvw.x && uvw.x < 0.29f + 0.26f)
    {
      float d = -uvw.y + fabs(uvw.x - 0.29f);
      if (0.47f - 0.15f * sqrt(2.0f) < d && d < 0.47f)
      {
        return arrow_color;
      }
    }
    return Color_White;
  }
  // outer red ring
  if (d_sq < 0.98f * 0.98f)
  {
    return Color_Red;
  }
  // tiny outer white ring
  return Color_White;
}

Color sign_pedestrian_island(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  const float inv_sqrt2 = 1.0f / sqrt(2.0f);
  float d_sq = dot(uvw.xy, uvw.xy);

  if (d_sq < (1.0f - 0.07f) * (1.0f - 0.07f))
  {
    if (-0.54f < uvw.x && -0.54f < uvw.y)
    {
      float2 ruv = f2((uvw.y - uvw.x) * inv_sqrt2, -(uvw.x + uvw.y) * inv_sqrt2);
      if (uvw.x < -0.54f + 0.2f || uvw.y < -0.54f + 0.2f)
      {
        if (-0.45f < ruv.x && ruv.x < 0.45f)
        {
          return Color_White;
        }
      }
      else if (-0.1f < ruv.x && ruv.x < 0.1f && -0.74f < ruv.y)
      {
        return Color_White;
      }
    }
    return Color_Blue;
  }
  return Color_White;
}

Color sign_stop(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  const float k1 = sqrt(2.0f);
  const float k2 = 1.0f / sqrt(2.0f);
  float d = max(max(fabs(uvw.x) - 1.0f, fabs(uvw.y) - 1.0f), (fabs(uvw.x) + fabs(uvw.y) - k1) * k2);

  if (d < -0.09f)
  {
    if (-0.35f < uvw.y && uvw.y < 0.35f)
    {
      // S
      if (0.38f < uvw.x && uvw.x < 0.77f)
      {
        if (0.35f - 0.17f < uvw.y)
        {
          float2 d1 = uvw.xy - f2(0.38f + 0.17f, 0.35f - 0.17f);
          float dist_sq = dot(d1, d1);
          if ((0.17f - 0.1f) * (0.17f - 0.1f) < dist_sq && dist_sq < 0.17f * 0.17f)
          {
            return Color_White;
          }
        }
        else if (uvw.y < -0.35f + 0.17f)
        {
          float2 d1 = uvw.xy - f2(0.38f + 0.17f, -0.35f + 0.17f);
          float dist_sq = dot(d1, d1);
          if ((0.17f - 0.1f) * (0.17f - 0.1f) < dist_sq && dist_sq < 0.17f * 0.17f)
          {
            return Color_White;
          }
        }
        else if (0.38f + 0.17f < uvw.x)
        {
          float2 d1 = uvw.xy - f2(0.38f + 0.35f - 0.05f - 0.18f, 0.35f - 0.17f);
          float dist1_sq = dot(d1, d1);
          if ((0.18f - 0.05f) * (0.18f - 0.05f) < dist1_sq && dist1_sq < (0.18f + 0.05f) * (0.18f + 0.05f))
          {
            return Color_White;
          }
        }
        else
        {
          float2 d1 = uvw.xy - f2(0.38f + 0.05f + 0.18f, -0.35f + 0.17f);
          float dist1_sq = dot(d1, d1);
          if ((0.18f - 0.05f) * (0.18f - 0.05f) < dist1_sq && dist1_sq < (0.18f + 0.05f) * (0.18f + 0.05f))
          {
            return Color_White;
          }
        }
      }
      // T
      else if (0.0f < uvw.x && uvw.x < 0.34f)
      {
        if (0.35f - 0.1f < uvw.y) return Color_White;
        if (0.17f - 0.05f < uvw.x && uvw.x < 0.17f + 0.05f) return Color_White;
      }
      // O
      else if (-0.04f - 0.34f < uvw.x && uvw.x < -0.04f)
      {
        if (0.35f - 0.17f < uvw.y)
        {
          float2 d1 = uvw.xy - f2(-0.04f - 0.17f, 0.35f - 0.17f);
          float dist_sq = dot(d1, d1);
          if ((0.17f - 0.1f) * (0.17f - 0.1f) < dist_sq && dist_sq < 0.17f * 0.17f)
          {
            return Color_White;
          }
        }
        else if (uvw.y < -0.35f + 0.17f)
        {
          float2 d1 = uvw.xy - f2(-0.04f - 0.17f, -0.35f + 0.17f);
          float dist_sq = dot(d1, d1);
          if ((0.17f - 0.1f) * (0.17f - 0.1f) < dist_sq && dist_sq < 0.17f * 0.17f)
          {
            return Color_White;
          }
        }
        else
        {
          if (-uvw.x < 0.04f + 0.1f || 0.04f + 0.34f - 0.1f < -uvw.x)
          {
            return Color_White;
          }
        }
      }
      // P
      else if (-0.04f - 0.34f - 0.34f - 0.09f < uvw.x && uvw.x < -0.04f - 0.34f - 0.09f)
      {
        if (-uvw.x < 0.04f + 0.34f + 0.09f + 0.1f)
        {
          return Color_White;
        }
        else
        {
          float2 d1 = uvw.xy - f2(-0.04f - 0.34f - 0.09f - 0.1f, 0.35f - 0.21f);
          float dist_sq = dot(d1, d1);
          if ((0.21f - 0.1f) * (0.21f - 0.1f) < dist_sq && dist_sq < 0.21f * 0.21f)
          {
            return Color_White;
          }
        }
      }
    }
    return Color_Red;
  }
  return Color_White;
}

Color sign_priority(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  const float k = 1.0f / sqrt(2.0f);
  float d = (fabs(uvw.x) + fabs(uvw.y) - 1.0f) * k;
  if (d < -0.3f)  return Color_Yellow;
  if (d < -0.28f) return Color_Black;
  if (d < -0.04f) return Color_White;
  if (d < -0.02f) return Color_Black;
                  return Color_White;
}

Color sign_yield(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;
  const float k = 1.0f / sqrt(3.0f);
  float d = max(fabs(uvw.x) - 1.0f + ((1.0f - uvw.y) * k), uvw.y - 1.0f);
  if (d < -0.2f)  return Color_White;
  if (d < -0.03f) return Color_Red;
                  return Color_White;
}

Color sign_left(float3 uvw)
{
  if (uvw.z < 0.0f) return Color_Black;

  float d_sq = dot(uvw.xy, uvw.xy);
  if (d_sq < (1.0f - 0.07f) * (1.0f - 0.07f))
  {
    if (-0.38f < uvw.x && -0.67f < uvw.y)
    {
      // first part if the line
      if (uvw.x < -0.38f + 0.18f && uvw.y < 0.0f) return Color_White;
      // turn
      if (uvw.x < 0.0f && 0.0f < uvw.y && (0.38f - 0.18f) * (0.38f - 0.18f) < d_sq && d_sq < 0.38f * 0.38f) return Color_White;
      // second line
      if (0.0f < uvw.x && uvw.x < 0.38f && 0.38f - 0.18f < uvw.y && uvw.y < 0.38f) return Color_White;
      // tip
      if (0.29f - 0.35f < uvw.y && uvw.y < 0.29f + 0.35f)
      {
        float d2 = -uvw.x - fabs(uvw.y - 0.29f);
        if (-0.64f < d2 && d2 < -0.64f + 0.18f * sqrt(2.0f))
        {
          return Color_White;
        }
      }
    }
    return Color_Blue;
  }
  return Color_White;
}

Color get_pedestrian(float3 uvw)
{
  const float sqrt_half = sqrt(0.5f);
  const float r = 3.75f / 2.0f;
  uvw *= f3(5.0f, 7.5f, 1.0f);
  float2 d = uvw.xy - f2(0.0f, 6.0f - r);
  if (dot(d, d) < r * r)
  {
    return Color_Black;
  }

  uvw.x = fabs(uvw.x);
  if (uvw.x < 0.75f && -6.0f + r < uvw.y && uvw.y < 6.0f - r)
  {
    return Color_Black;
  }

  float rx = sqrt_half * uvw.x + sqrt_half * uvw.y;
  float ry = sqrt_half * uvw.x - sqrt_half * uvw.y;
  if ((ry < 7.0f && -3.0f < rx && rx < -1.5f) ||
      (rx < 5.0f && -1.0f < ry && ry <  0.5f))
  {
    return Color_Black;
  }
  return Color_White;
}

Color get_object_color(int object_type, float3 uvw)
{
  switch(object_type)
  {
    // case 0x0011: return sign_speed_limit_start(uvw.yzx);
    // case 0x0012: return sign_speed_limit_end(uvw.yzx);
    // case 0x0013: return sign_crosswalk(uvw.yzx);
    case 0x0014: return sign_parking_zone(uvw.yzx);
    // case 0x0015: return sign_expressway_start(uvw.yzx);
    // case 0x0016: return sign_expressway_end(uvw.yzx);
    case 0x0017: return sign_sharp_turn_left(uvw.yzx);
    case 0x0018: return sign_sharp_turn_left(-uvw.yzx);// sign_sharp_turn_right
    case 0x0019: return sign_barred_area(uvw.yzx);
    case 0x001A: return sign_pedestrian_island(uvw.yzx);
    case 0x001B: return sign_stop(uvw.yzx);
    case 0x001C: return sign_priority(uvw.yzx);
    case 0x001D: return sign_yield(uvw.yzx);
    case 0x001E: return sign_left(uvw.yzx);
    case 0x001F: return sign_left(uvw.yzx * f3(-1.0f, 1.0f, 1.0f)); // sign_right
    // case 0x0020: return sign_no_passing_start(uvw.yzx);
    // case 0x0021: return sign_no_passing_end(uvw.yzx);
    // case 0x0022: return sign_uphill(uvw.yzx);
    // case 0x0023: return sign_downhill(uvw.yzx);

    case 0x0041: // Road_Start_Line
    case 0x0042: // Road_Stop_Line
    case 0x0043: // Road_Yield_Line
    case 0x0044: // Road_Speed_Limit_Start
    case 0x0045: // Road_Speed_Limit_End
    case 0x0046: // Road_Barred_Area
    case 0x0047: // Road_Crosswalk
    case 0x0049: // Road_Turn_Left
    case 0x004A: // Road_Turn_Right
      return Color_White;

    case 0x0081: return get_pedestrian(uvw.yzx);
    case 0x0085: // Obstacle
      return Color_Obstacle;
  }
  return Color_Error;
}

Ray create_ray(const Camera *cam, const float3 offset, const float3 dir)
{
  float3 normal =
    dir.x * cam->dir_forward +
    dir.y * cam->dir_right +
    dir.z * cam->dir_up;
  float3 pos = cam->pos +
    offset.x * cam->dir_forward +
    offset.y * cam->dir_right +
    offset.z * cam->dir_up;
  return (Ray){ pos, normal };
}

__kernel void MAIN(
    __write_only image2d_t result,
    __global float3 *cam_normals,
    __global uint8_t *map,
    const int2   map_size,
    __global Object *objects,
    const int num_objects,
    const float3 cam_pos,
    const float3 cam_dir_forward,
    const float3 cam_dir_right,
    const float3 cam_dir_up,
    const int    cam_ortho,
    const int    cam_linear,
    const uint   image_num,
    const float3 sun_dir,
    const float  noise_strength,
    const float  brightness)
{
  const int id = get_global_id(0);
  const int2 image_size = get_image_dim(result);
  const int ix = id % image_size.x;
  const int iy = id / image_size.x;
  random_state rng = {{ix, iy, image_num}};
  const Camera cam = {
    .pos = cam_pos,
    .dir_forward = cam_dir_forward,
    .dir_right = cam_dir_right,
    .dir_up = cam_dir_up
  };

  float depth = 10000000.0f;
  Ray r;
  if (cam_ortho)
  {
    r = create_ray(&cam, cam_normals[ix + iy * image_size.x], f3(1, 0, 0));
  }
  else
  {
    r = create_ray(&cam, f3(0.0f, 0.0f, 0.0f), cam_normals[ix + iy * image_size.x]);
  }
  float3 normal;
  float3 uvw;
  float3 color = f3(0.5f, 0.5f, 0.5f);
  int object_type = 0;
  for (int i = 0; i < num_objects; ++i)
  {
    Object obj = objects[i];
    float new_depth = depth;
    float3 new_normal;
    float3 new_uvw;
    if(intersect_ray_object(&r, &obj, &new_depth, &new_normal, &new_uvw))
    {
      if (check_object_mask(obj.type, new_uvw))
      {
        depth  = new_depth;
        normal = new_normal;
        uvw    = new_uvw;
        object_type = obj.type;
      }
    }
  }

  if (0 != object_type)
  {
    float3 point  = r.origin + r.normal * depth;
    if (0x0040 == object_type) // Road_Markings -> road
    {
      color = get_road(map, map_size, point.xy);
    }
    else
    {
      color = color_table[get_object_color(object_type, uvw)];
    }
    float ambient = 0.5f;
    float diffuse = max(0.0f, dot(normal, sun_dir)) * 0.5f;
    float3 half_vec = normalize(-r.normal + sun_dir);
    float specular = pow(max(0.0f, dot(normal, half_vec)), 20.0f);

    float r0 = pow((1.0f - 1.2f) / (1.0f + 1.2f), 2.0f);
    float refl = r0 + (1.0f - r0) * pow(1 + dot(r.normal, normal), 5.0f);
    color = color * (ambient + diffuse) + 0.2f * refl + 0.4f * specular;
  }

  color.xyz *= brightness;

  if (cam_linear)
  {
    color.xyz *= pow(dot(r.normal, cam_dir_forward), 4.0f);
  }

  color.xyz *= rand_float3(&rng) * noise_strength + (1.0f - noise_strength * 0.5f);
  color.xyz += rand_float3(&rng) * noise_strength - (noise_strength * 0.5f);

  write_imagef(result, i2(ix, iy), f4(color.x, color.y, color.z, 1.0f));
}
