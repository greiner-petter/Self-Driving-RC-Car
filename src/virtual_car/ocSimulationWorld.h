#pragma once

#include "../common/ocArray.h"
#include "../common/ocCar.h"
#include "../common/ocGeometry.h"
#include "../common/ocMat.h"
#include "../common/ocRect.h"
#include "../common/ocTime.h"
#include "../common/ocTypes.h"
#include "../common/ocVec.h"

#include <cmath>
#include <cstdint>

enum class ocRoadTileType : uint8_t
{
    None                     =  0,
    None1                    =  1,
    None2                    =  2,
    None3                    =  3,

    Straight_We              =  4,
    Straight_Ns              =  5,
    Straight_EW              =  6,
    Straight_Sn              =  7,

    Curve_Sw                 =  8,
    Curve_Nw                 =  9,
    Curve_Ne                 = 10,
    Curve_Se                 = 11,

    Intersection_Stop_We     = 12,
    Intersection_Stop_Ns     = 13,
    Intersection_Stop_Ew     = 14,
    Intersection_Stop_Sn     = 15,

    Crosswalk_We             = 16,
    Crosswalk_Ns             = 17,
    Crosswalk_Ew             = 18,
    Crosswalk_Sn             = 19,

    Barred_Area_We           = 20,
    Barred_Area_Ns           = 21,
    Barred_Area_Ew           = 22,
    Barred_Area_Sn           = 23,

    Straight_We_No_Passing   = 24,
    Straight_Ns_No_Passing   = 25,
    Straight_Ew_No_Passing   = 26,
    Straight_Sn_No_Passing   = 27,

    Curve_Sw_No_Passing      = 28,
    Curve_Nw_No_Passing      = 29,
    Curve_Ne_No_Passing      = 30,
    Curve_Se_No_Passing      = 31,

    Straight_We_No_Passing_N = 32,
    Straight_Ns_No_Passing_W = 33,
    Straight_Ew_No_Passing_S = 34,
    Straight_Sn_No_Passing_E = 35,

    Curve_Sw_No_Passing_I    = 36,
    Curve_Nw_No_Passing_I    = 37,
    Curve_Ne_No_Passing_I    = 38,
    Curve_Se_No_Passing_I    = 39,

    Curve_Sw_No_Passing_O    = 40,
    Curve_Nw_No_Passing_O    = 41,
    Curve_Ne_No_Passing_O    = 42,
    Curve_Se_No_Passing_O    = 43,

    Intersection_Yield_We    = 44,
    Intersection_Yield_Ns    = 45,
    Intersection_Yield_Ew    = 46,
    Intersection_Yield_Sn    = 47,

    Intersection_Yield_All   = 48,
    Intersection_Yield_All1  = 49,
    Intersection_Yield_All2  = 50,
    Intersection_Yield_All3  = 51,

    Intersection_Turn_Sw     = 52,
    Intersection_Turn_Nw     = 53,
    Intersection_Turn_Ne     = 54,
    Intersection_Turn_Se     = 55,

    Parking_We               = 56,
    Parking_Ns               = 57,
    Parking_Ew               = 58,
    Parking_Sn               = 59,

    Start_Straight_We        = 60,
    Start_Straight_Ns        = 61,
    Start_Straight_Ew        = 62,
    Start_Straight_Sn        = 63,

    Start_Curve_Sw           = 64,
    Start_Curve_Nw           = 65,
    Start_Curve_Ne           = 66,
    Start_Curve_Se           = 67,

    Count                    = 68
};

struct ocLaneModel
{
    Vec2 center;
    float radius;
    float width;
};

struct Aab3
{
  Vec3 min;
  Vec3 max;

  bool contains(Vec3 point) const
  {
    return min.x <= point.x && point.x <= max.x
        && min.y <= point.y && point.y <= max.y
        && min.z <= point.z && point.z <= max.z;
  }

  Rect xy() const { return {min.xy(), max.xy()}; }
  Rect xz() const { return {min.xz(), max.xz()}; }
  Rect yz() const { return {min.yz(), max.yz()}; }
};

struct ocVirtualObject
{
  ocObjectType type;
  ocPose       pose;
  Vec3         size;

  bool is_box() const {
    return ocObjectType::Obstacle == type;
  }
  bool is_sign() const {
    return 0 < (int)type && (int)type < 0x40;
  }
  bool is_pedestrian() const {
    return ocObjectType::Pedestrian == type;
  }

  void move(float dx, float dy)
  {
    pose.pos.x += dx;
    pose.pos.y += dy;
  }

  Aab3 bounds() const
  {
    Vec3 half_size = size * 0.5f;

    Vec3 corners[8] = {
      Vec3{-half_size.x, -half_size.y, -half_size.z},
      Vec3{-half_size.x, -half_size.y,  half_size.z},
      Vec3{-half_size.x,  half_size.y, -half_size.z},
      Vec3{-half_size.x,  half_size.y,  half_size.z},
      Vec3{ half_size.x, -half_size.y, -half_size.z},
      Vec3{ half_size.x, -half_size.y,  half_size.z},
      Vec3{ half_size.x,  half_size.y, -half_size.z},
      Vec3{ half_size.x,  half_size.y,  half_size.z},
    };

    Aab3 result = {pose.pos, pose.pos};
    for (int i = 0; i < 8; ++i)
    {
      Vec3 world_pos = pose.generalize_pos(corners[i]);
      result.min.x = std::min(result.min.x, world_pos.x);
      result.min.y = std::min(result.min.y, world_pos.y);
      result.min.z = std::min(result.min.z, world_pos.z);
      result.max.x = std::max(result.max.x, world_pos.x);
      result.max.y = std::max(result.max.y, world_pos.y);
      result.max.z = std::max(result.max.z, world_pos.z);
    }
    return result;
  }

  ocVirtualObject facing_up() const
  {
    const float half_pi = 1.570796327f;
    ocVirtualObject result = *this;
    if (is_pedestrian() || is_sign())
    {
      result.pose.pitch = -half_pi;
    }
    return result;
  }
};

struct ocArea
{
  bool no_driving;
  bool parking_ok;
  bool triggerable;
  bool trigger_on;
  bool trigger_off;
  ocTime trigger_time;

  uint32_t vertex_count;
  Vec2 vertices[10];

  bool contains(Vec2 v)
  {
    Vec2 v2 = v + Vec2(10000000.0f, 0.0f);
    int intersections = line_line_intersection(v, v2, vertices[vertex_count - 1], vertices[0], nullptr);
    for (size_t i = 1; i < vertex_count; ++i)
    {
      intersections += line_line_intersection(v, v2, vertices[i-1], vertices[i], nullptr);
    }
    return intersections & 1;
  }
};

ocArray<ocLaneModel> get_models_for_road(ocRoadTileType road);
ocArray<ocVirtualObject> get_objects_for_road(ocRoadTileType road);
ocArray<ocArea> get_areas_for_road(ocRoadTileType road);

struct ocRoadTile
{
  ocRoadTileType type;
  int index_x;
  int index_y;

  Vec2 center() const
  {
    return Vec2((float)index_x, (float)index_y) * 200.0f;
  }

  Rect bounds() const
  { 
    auto c = center();
    return { c.x - 100.0f, c.y - 100.0f, c.x + 100.0f, c.y + 100.0f };
  }

  ocPose pose() const
  {
    const float Pi = 3.141592654f;
    float angle = (float)((int)type & 0x03) * Pi * 0.5f;
    return ocPose(Vec3(center(), 0.0f), angle, 0.0f, 0.0f);
  }

  ocArray<ocLaneModel> get_lanes() const
  {
    auto lanes = get_models_for_road((ocRoadTileType)((int)type & ~0x03));
    auto p = pose();
    for (auto& lane : lanes)
    {
      lane.center = p.generalize_pos(Vec3(lane.center, 0.0f)).xy();
    }
    return lanes;
  }

  ocArray<ocVirtualObject> get_objects() const
  {
    auto objects = get_objects_for_road((ocRoadTileType)((int)type & ~0x03));
    auto p = pose();
    for (auto& object : objects)
    {
      object.pose = ocPose::compose(p, object.pose);
    }
    return objects;
  }

  ocArray<ocArea> get_areas() const
  {
    auto areas = get_areas_for_road((ocRoadTileType)((int)type & ~0x03));
    auto p = pose();
    for (auto& area : areas)
    {
      for (uint32_t i = 0; i < area.vertex_count; ++i)
      {
        area.vertices[i] = p.generalize_pos(Vec3(area.vertices[i], 0.0f)).xy();
      }
    }
    return areas;
  }
};

struct ocSimulationWorld
{
  int world_width;
  int world_height;
  ocRoadTile *world;
  ocArray<ocVirtualObject> world_objects;
  ocArray<ocVirtualObject> user_objects;
  ocArray<ocArea>          triggers;
  bool triggers_active;

  const ocRoadTile *get_tile(int col, int row) const
  {
    if (0 <= col && col < world_width && 0 <= row && row < world_height)
    {
      return &world[col + row * world_width];
    }
    return nullptr;
  }

  ocRoadTile *get_tile(int col, int row)
  {
    if (0 <= col && col < world_width && 0 <= row && row < world_height)
    {
      return &world[col + row * world_width];
    }
    return nullptr;
  }

  const ocRoadTile *get_tile_at(Vec2 point) const
  {
    int ix = (int)((point.x + 100.0f) / 200.0f);
    int iy = (int)((point.y + 100.0f) / 200.0f);
    if (0 <= ix && ix < world_width && 0 <= iy && iy < world_height)
    {
      return &world[ix + iy * world_width];
    }
    return nullptr;
  }

  ocRoadTile *get_tile_at(Vec2 point)
  {
    int ix = (int)((point.x + 100.0f) / 200.0f);
    int iy = (int)((point.y + 100.0f) / 200.0f);
    if (0 <= ix && ix < world_width && 0 <= iy && iy < world_height)
    {
      return &world[ix + iy * world_width];
    }
    return nullptr;
  }

  ocLaneModel get_lane_at(Vec2 point) const
  {
    auto tile = get_tile_at(point);
    if (tile)
    {
      auto lanes = tile->get_lanes();
      for (auto lane : lanes)
      {
        if (std::abs(distance(point, lane.center) - lane.radius) <= lane.width * 0.5f)
        {
          return lane;
        }
      }
    }
    return {};
  }

  void remove_objects_at(Vec2 point, ocObjectType type = ocObjectType::None)
  {
    for (size_t i = 0; i < world_objects.get_length(); ++i)
    {
      auto &obj = world_objects[i];
      if (ocObjectType::None == type || obj.type == type)
      {
        if (obj.bounds().xy().contains(point)) world_objects.remove_at(i);
      }
    }
    for (size_t i = 0; i < user_objects.get_length(); ++i)
    {
      auto &obj = user_objects[i];
      if (ocObjectType::None == type || obj.type == type)
      {
        if (obj.bounds().xy().contains(point)) user_objects.remove_at(i);
      }
    }
  }

  bool remove_object(const ocVirtualObject *object)
  {
    // TODO: this loop is dumb. just calculate if the pointer is in the array or not!
    for (size_t i = 0; i < user_objects.get_length(); ++i)
    {
      if (object == &user_objects[i])
      {
        user_objects.remove_at(i);
        return true;
      }
    }
    return false;
  }

  void add_object(const ocVirtualObject& object)
  {
    user_objects.append(object);
  }

  ocVirtualObject *get_object_at(Vec2 point, ocObjectType type = ocObjectType::None)
  {
    for (auto &obj : user_objects)
    {
      if (ocObjectType::None == type || obj.type == type)
      {
        if (obj.facing_up().bounds().xy().contains(point))  return &obj;
      }
    }
    for (auto &obj : world_objects)
    {
      if (ocObjectType::None == type || obj.type == type)
      {
        if (obj.facing_up().bounds().xy().contains(point))  return &obj;
      }
    }
    return nullptr;
  }

  ocArray<ocVirtualObject> get_objects_visible_from(
    const ocCarState& car,
    ocObjectType type = ocObjectType::None) const
  {
    auto projector = car.make_projector();

    ocArray<ocVirtualObject> result;
    for (auto &obj : world_objects)
    {
      if (ocObjectType::None == type || obj.type == type)
      {
        auto bounds = obj.bounds();
        Vec3 corners[8] = {
          Vec3{bounds.min.x, bounds.min.y, bounds.min.z},
          Vec3{bounds.min.x, bounds.min.y, bounds.max.z},
          Vec3{bounds.min.x, bounds.max.y, bounds.min.z},
          Vec3{bounds.min.x, bounds.max.y, bounds.max.z},
          Vec3{bounds.max.x, bounds.min.y, bounds.min.z},
          Vec3{bounds.max.x, bounds.min.y, bounds.max.z},
          Vec3{bounds.max.x, bounds.max.y, bounds.min.z},
          Vec3{bounds.max.x, bounds.max.y, bounds.max.z}
        };
        for (int i = 0; i < 8; ++i)
        {
          if (projector.can_see(corners[i]))
          {
            result.append(obj);
            break;
          }
        }
      }
    }
    for (auto &obj : user_objects)
    {
      if (ocObjectType::None == type || obj.type == type)
      {
        auto bounds = obj.bounds();
        Vec3 corners[8] = {
          Vec3{bounds.min.x, bounds.min.y, bounds.min.z},
          Vec3{bounds.min.x, bounds.min.y, bounds.max.z},
          Vec3{bounds.min.x, bounds.max.y, bounds.min.z},
          Vec3{bounds.min.x, bounds.max.y, bounds.max.z},
          Vec3{bounds.max.x, bounds.min.y, bounds.min.z},
          Vec3{bounds.max.x, bounds.min.y, bounds.max.z},
          Vec3{bounds.max.x, bounds.max.y, bounds.min.z},
          Vec3{bounds.max.x, bounds.max.y, bounds.max.z}
        };
        for (int i = 0; i < 8; ++i)
        {
          if (projector.can_see(corners[i]))
          {
            result.append(obj);
            break;
          }
        }
      }
    }
    return result;
  }

  ocArray<ocVirtualObject> get_all_objects_facing_up() const
  {
    ocArray<ocVirtualObject> result;
    for (auto obj : world_objects) result.append(obj.facing_up());
    for (auto obj :  user_objects) result.append(obj.facing_up());
    return result;
  }

  void regenerate_objects()
  {
    world_objects.clear();
    for (int y = 0; y < world_height; ++y)
    for (int x = 0; x < world_width; ++x)
    {
      world_objects.append(get_tile(x, y)->get_objects());
    }
  }

  void regenerate_triggers()
  {
    triggers.clear();
    for (int y = 0; y < world_height; ++y)
    for (int x = 0; x < world_width; ++x)
    {
      triggers.append(get_tile(x, y)->get_areas());
    }
  }
};
