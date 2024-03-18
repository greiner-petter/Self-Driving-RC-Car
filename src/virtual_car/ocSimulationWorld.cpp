#include "ocSimulationWorld.h"

ocArray<ocLaneModel> get_models_for_road(ocRoadTileType road)
{
  switch(road)
  {
    case ocRoadTileType::Straight_We:
    case ocRoadTileType::Crosswalk_We:
    case ocRoadTileType::Barred_Area_We:
    case ocRoadTileType::Straight_We_No_Passing:
    case ocRoadTileType::Straight_We_No_Passing_N:
    case ocRoadTileType::Parking_We:
      return {
        { .center = Vec2(0.0f, 1000000.0f - 21.0f), .radius = 1000000.0f, .width = 20.0f },
        { .center = Vec2(0.0f, 1000000.0f + 21.0f), .radius = 1000000.0f, .width = 20.0f },
      };
    case ocRoadTileType::Curve_Sw:
    case ocRoadTileType::Curve_Sw_No_Passing:
    case ocRoadTileType::Curve_Sw_No_Passing_I:
    case ocRoadTileType::Curve_Sw_No_Passing_O:
      return {
        { .center = Vec2(-100.0f, 100.0f),  .radius = 100 - 21.0f, .width = 20.0f },
        { .center = Vec2(-100.0f, 100.0f),  .radius = 100 + 21.0f, .width = 20.0f },
      };
    case ocRoadTileType::Intersection_Stop_We:
    case ocRoadTileType::Intersection_Yield_We:
    case ocRoadTileType::Intersection_Yield_All:
      return {
        { .center = Vec2(0.0f, 1000000.0f - 21.0f), .radius = 1000000.0f, .width = 20.0f },
        { .center = Vec2(0.0f, 1000000.0f + 21.0f), .radius = 1000000.0f, .width = 20.0f },
        { .center = Vec2(1000000.0f - 21.0f, 0.0f), .radius = 1000000.0f, .width = 20.0f },
        { .center = Vec2(1000000.0f + 21.0f, 0.0f), .radius = 1000000.0f, .width = 20.0f },
      };
    case ocRoadTileType::Intersection_Turn_Sw:
      return {
        { .center = Vec2(-40.0f, 40.0f),    .radius = 40.0f - 21.0f, .width = 20.0f },
        { .center = Vec2(-40.0f, 40.0f),    .radius = 40.0f + 21.0f, .width = 20.0f },
      };
    default: break;
  }
  return {};
}

ocArray<ocVirtualObject> get_objects_for_road(ocRoadTileType road)
{
  const float Pi = 3.141592654f;
  const float North = -Pi * 0.5f;
  const float East  = 0.0f;
  const float South = Pi * 0.5f;
  const float West  = Pi;
  switch (road)
  {
    case ocRoadTileType::Intersection_Stop_We:
      return {
        {
          .type = ocObjectType::Sign_Stop,
          .pose = ocPose({-85.0f, 52.0f, 22.5f}, West, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Stop,
          .pose = ocPose({85.0f, -52.0f, 22.5f}, East, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Priority,
          .pose = ocPose({-52.0f, -85.0f, 20.0f}, North, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Sign_Priority,
          .pose = ocPose({52.0f, 85.0f, 20.0f}, South, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Road_Stop_Line,
          .pose = ocPose({-43.0f, 21.0f, 0.0f}, West, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Stop_Line,
          .pose = ocPose({43.0f, -21.0f, 0.0f}, East, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        }
      };
    case ocRoadTileType::Crosswalk_We:
      return {
        {
          .type = ocObjectType::Sign_Crosswalk,
          .pose = ocPose({-60.0f, 52.0f, 22.5f}, West, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Crosswalk,
          .pose = ocPose({60.0f, -52.0f, 22.5f}, East, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Road_Crosswalk,
          .pose = ocPose{{}, East, 0.0f, 0.0f},
          .size = {40.0f, 82.0f, 0.1f}
        }
      };
    case ocRoadTileType::Barred_Area_We:
      return {
        {
          .type = ocObjectType::Sign_Barred_Area,
          .pose = ocPose({-140.0f, 52.0f, 22.5f}, West, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Road_Barred_Area,
          .pose = ocPose({0.0f, 26.0f, 0.0f}, East, 0.0f, 0.0f),
          .size = {200.0f, 30.0f, 0.1f}
        }
      };
    case ocRoadTileType::Intersection_Yield_We:
      return {
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({-85.0f, 52.0f, 22.5f}, West, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({85.0f, -52.0f, 22.5f}, East, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Priority,
          .pose = ocPose({-52.0f, -85.0f, 20.0f}, North, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Sign_Priority,
          .pose = ocPose({52.0f, 85.0f, 20.0f}, South, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({-43.0f, 21.0f, 0.0f}, West, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({43.0f, -21.0f, 0.0f}, East, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        }
      };
    case ocRoadTileType::Intersection_Yield_All:
      return {
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({-85.0f, 52.0f, 22.5f}, West, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({85.0f, -52.0f, 22.5f}, East, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({-52.0f, -85.0f, 20.0f}, North, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({52.0f, 85.0f, 20.0f}, South, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({-43.0f, 21.0f, 0.0f}, West, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({-21.0f, -43.0f, 0.0f}, North, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({43.0f, -21.0f, 0.0f}, East, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({21.0f, 43.0f, 0.0f}, South, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        }
      };
    case ocRoadTileType::Intersection_Turn_Sw:
      return {
        {
          .type = ocObjectType::Sign_Priority,
          .pose = ocPose({-85.0f, 52.0f, 20.0f}, West, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Sign_Proceed_Right,
          .pose = ocPose({-65.0f, 52.0f, 20.0f}, West, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Sign_Priority,
          .pose = ocPose({52.0f, 85.0f, 20.0f}, South, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Sign_Proceed_Left,
          .pose = ocPose({52.0f, 65.0f, 20.0f}, South, 0.0f, 0.0f),
          .size = {0.1f, 10.0f, 10.0f}
        },
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({85.0f, -52.0f, 22.5f}, East, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Sign_Yield,
          .pose = ocPose({-52.0f, -85.0f, 22.5f}, North, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Road_Proceed_Right,
          .pose = ocPose({-66.0f, 21.0f, 0.0f}, West, 0.0f, 0.0f),
          .size = {50.0f, 7.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Proceed_Left,
          .pose = ocPose({21.0f, 66.0f, 0.0f}, South, 0.0f, 0.0f),
          .size = {50.0f, 7.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({-21.0f, -43.0f, 0.0f}, North, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
        {
          .type = ocObjectType::Road_Yield_Line,
          .pose = ocPose({43.0f, -21.0f, 0.0f}, East, 0.0f, 0.0f),
          .size = {4.0f, 40.0f, 0.1f}
        },
      };
    case ocRoadTileType::Parking_We:
      return {
        {
          .type = ocObjectType::Sign_Parking_Zone,
          .pose = ocPose({-120.0f, 52.0f, 22.5f}, West, 0.0f, 0.0f),
          .size = {0.1f, 15.0f, 15.0f}
        },
        {
          .type = ocObjectType::Obstacle,
          .pose = ocPose({-55.0f, -70.0f, 8.5f}, East, 0.0f, 0.0f),
          .size = {10.0f, 20.0f, 17.0f}
        },
        {
          .type = ocObjectType::Obstacle,
          .pose = ocPose({50.0f, -65.0f, 10.0f}, East, 0.0f, 0.0f),
          .size = {20.0f, 30.0f, 20.0f}
        },
        {
          .type = ocObjectType::Obstacle,
          .pose = ocPose({0.0f, 60.0f, 7.5f}, East, 0.0f, 0.0f),
          .size = {30.0f, 20.0f, 15.0f}
        },
        {
          .type = ocObjectType::Parallel_Parking_Space,
          .pose = ocPose({45.0f, 58.0f, 0.0f}, West, 0.0f, 0.0f),
          .size = {50.0f, 30.0f, 0.0f}
        },
        {
          .type = ocObjectType::Perpendicular_Parking_Space,
          .pose = ocPose({-2.5f, -68.0f, 0.0f}, West, 0.0f, 0.0f),
          .size = {35.0f, 50.0f, 0.0f}
        },
      };
    default:
    {
      // nothing
    } break;
  }
  return {};
}

ocArray<ocArea> get_areas_for_road(ocRoadTileType road)
{
  switch (road)
  {
    case ocRoadTileType::None:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100, -100),
            Vec2( 100,  100),
            Vec2(-100,  100),
            Vec2(-100, -100)
          }
        }
      };
    case ocRoadTileType::Straight_We:
    case ocRoadTileType::Crosswalk_We:
    case ocRoadTileType::Straight_We_No_Passing:
    case ocRoadTileType::Straight_We_No_Passing_N:
    case ocRoadTileType::Start_Straight_We:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100, -100),
            Vec2( 100,  -43),
            Vec2(-100,  -43),
            Vec2(-100, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100,  43),
            Vec2( 100, 100),
            Vec2(-100, 100),
            Vec2(-100,  43)
          }
        }
      };
    case ocRoadTileType::Curve_Sw:
    case ocRoadTileType::Curve_Sw_No_Passing:
    case ocRoadTileType::Curve_Sw_No_Passing_I:
    case ocRoadTileType::Curve_Sw_No_Passing_O:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 3,
          .vertices     = {
            Vec2(-100, 100),
            Vec2(-100,  43),
            Vec2( -43, 100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 8,
          .vertices     = {
            Vec2( 100, -100),
            Vec2( 100,  100),
            Vec2(  43,  100),

            Vec2(  35,   44),
            Vec2(   3,   -3),
            Vec2( -44,  -35),

            Vec2(-100,  -43),
            Vec2(-100, -100)
          }
        }
      };
    case ocRoadTileType::Intersection_Stop_We:
    case ocRoadTileType::Intersection_Yield_We:
      return {
        {
          .no_driving   = false,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = true,
          .trigger_off  = false,
          .trigger_time = ocTime::seconds(3),
          .vertex_count = 4,
          .vertices     = {
            Vec2( -94, -43),
            Vec2( -94,  43),
            Vec2(-100,  43),
            Vec2(-100, -43)
          }
        },
        {
          .no_driving   = false,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = true,
          .trigger_off  = false,
          .trigger_time = ocTime::seconds(3),
          .vertex_count = 4,
          .vertices     = {
            Vec2( 94, -43),
            Vec2( 94,  43),
            Vec2(100,  43),
            Vec2(100, -43)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = true,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-43, -43),
            Vec2(-43,  43),
            Vec2( 43,  43),
            Vec2( 43, -43)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(100, -100),
            Vec2(100,  -43),
            Vec2( 43,  -43),
            Vec2( 43, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(100,  43),
            Vec2(100, 100),
            Vec2( 43, 100),
            Vec2( 43,  43)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-100, 100),
            Vec2(-100,  43),
            Vec2( -43,  43),
            Vec2( -43, 100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-100, -100),
            Vec2( -43, -100),
            Vec2( -43,  -43),
            Vec2(-100,  -43)
          }
        }
      };
    case ocRoadTileType::Intersection_Yield_All:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(100, -100),
            Vec2(100,  -43),
            Vec2( 43,  -43),
            Vec2( 43, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(100,  43),
            Vec2(100, 100),
            Vec2( 43, 100),
            Vec2( 43,  43)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-100, 100),
            Vec2(-100,  43),
            Vec2( -43,  43),
            Vec2( -43, 100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-100, -100),
            Vec2( -43, -100),
            Vec2( -43,  -43),
            Vec2(-100,  -43)
          }
        }
      };
    case ocRoadTileType::Intersection_Turn_Sw:
      return {
        {
          .no_driving   = false,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = true,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( -94, -43),
            Vec2( -94,  43),
            Vec2(-100,  43),
            Vec2(-100, -43)
          }
        },
        {
          .no_driving   = false,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = true,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-43,  94),
            Vec2( 43,  94),
            Vec2( 43, 100),
            Vec2(-43, 100)
          }
        },
        {
          .no_driving   = false,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = true,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 94,  43),
            Vec2( 94, -43),
            Vec2(100, -43),
            Vec2(100,  43)
          }
        },
        {
          .no_driving   = false,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = true,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 43,  -94),
            Vec2(-43,  -94),
            Vec2(-43, -100),
            Vec2( 43, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = true,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 8,
          .vertices     = {
            Vec2( 43,  43),
            Vec2( 50,  43),
            Vec2( 50, -50),
            Vec2(-43, -50),
            Vec2(-43, -43),
            Vec2(-20, -43),
            Vec2( 20, -20),
            Vec2( 43,  20)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(100, -100),
            Vec2(100,  -43),
            Vec2( 43,  -43),
            Vec2( 43, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(100,  43),
            Vec2(100, 100),
            Vec2( 43, 100),
            Vec2( 43,  43)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-100, 100),
            Vec2(-100,  43),
            Vec2( -43,  43),
            Vec2( -43, 100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2(-100, -100),
            Vec2( -43, -100),
            Vec2( -43,  -43),
            Vec2(-100,  -43)
          }
        }
      };
    case ocRoadTileType::Barred_Area_We:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100, -100),
            Vec2( 100,  -43),
            Vec2(-100,  -43),
            Vec2(-100, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100,  43),
            Vec2( 100, 100),
            Vec2(-100, 100),
            Vec2(-100,  43)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 65, 10),
            Vec2( 85, 43),
            Vec2(-85, 43),
            Vec2(-65, 10)
          }
        }
      };
    case ocRoadTileType::Parking_We:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100, -100),
            Vec2( 100,  -43),
            Vec2(-100,  -43),
            Vec2(-100, -100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100,  43),
            Vec2( 100, 100),
            Vec2(-100, 100),
            Vec2(-100,  43)
          }
        }
      };
      case ocRoadTileType::Start_Curve_Sw:
      return {
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 3,
          .vertices     = {
            Vec2(-100, 100),
            Vec2(-100,  43),
            Vec2( -43, 100)
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 4,
          .vertices     = {
            Vec2( 100, -100),
            Vec2( 100,  -43),
            Vec2(-100,  -43),
            Vec2(-100, -100),
          }
        },
        {
          .no_driving   = true,
          .parking_ok   = false,
          .triggerable  = false,
          .trigger_on   = false,
          .trigger_off  = false,
          .trigger_time = {},
          .vertex_count = 5,
          .vertices     = {
            Vec2( 100,    1),
            Vec2( 100,  100),
            Vec2(  43,  100),

            Vec2(  35,   44),
            Vec2(   3,    1)
          }
        }
      };
    default:
    {
      // nothing
    } break;
  }
  return {};
}
