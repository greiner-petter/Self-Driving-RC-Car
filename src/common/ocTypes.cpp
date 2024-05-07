#include "ocTypes.h"

// C++ is a language with many features. But automatic enum->string conversion
// isn't one of them. That would be too useful.

// The way these functions are written (no default case, return at the end) will
// cause compiler warnings if a case was forgotten. At least that's nice.

const char *to_string(ocMemberId member_id)
{
  switch (member_id)
  {
  case ocMemberId::None:               return "ocMemberId::None";
  case ocMemberId::Camera:             return "ocMemberId::Camera";
  case ocMemberId::Image_Processing:   return "ocMemberId::Image_Processing";
  case ocMemberId::Lane_Detection:     return "ocMemberId::Lane_Detection";
  case ocMemberId::Ai:                 return "ocMemberId::Ai";
  case ocMemberId::Sign_Detection:     return "ocMemberId::Sign_Detection";
  case ocMemberId::Ipc_Hub:            return "ocMemberId::Ipc_Hub";
  case ocMemberId::Eth_Gateway:        return "ocMemberId::Eth_Gateway";
  case ocMemberId::Can_Gateway:        return "ocMemberId::Can_Gateway";
  case ocMemberId::Video_Viewer:       return "ocMemberId::Video_Viewer";
  case ocMemberId::Video_Input:        return "ocMemberId::Video_Input";
  case ocMemberId::Video_Recorder:     return "ocMemberId::Video_Recorder";
  case ocMemberId::Command_Line:       return "ocMemberId::Command_Line";
  case ocMemberId::Qr_Detection:       return "ocMemberId::Qr_Detection";
  case ocMemberId::Obstacle_Detection: return "ocMemberId::Obstacle_Detection";
  case ocMemberId::Virtual_Car:        return "ocMemberId::Virtual_Car";
  case ocMemberId::Tachometer:         return "ocMemberId::Tachometer";
  case ocMemberId::Apriltag_Detection: return "ocMemberId::Apriltag_Detection";
  }
  return "<unknown>";
}

const char *to_string(ocMessageId message_id)
{
  switch (message_id)
  {
  case ocMessageId::None:                     return "ocMessageId::None";
  case ocMessageId::Auth_Request:             return "ocMessageId::Auth_Request";
  case ocMessageId::Auth_Response:            return "ocMessageId::Auth_Response";
  case ocMessageId::Member_List:              return "ocMessageId::Member_List";
  case ocMessageId::Subscribe_To_Messages:    return "ocMessageId::Subscribe_To_Messages";
  case ocMessageId::Deafen_Member:            return "ocMessageId::Deafen_Member";
  case ocMessageId::Mute_Member:              return "ocMessageId::Mute_Member";
  case ocMessageId::Ipc_Stats:                return "ocMessageId::Ipc_Stats";
  case ocMessageId::Disconnect_Me:            return "ocMessageId::Disconnect_Me";
  case ocMessageId::Camera_Image_Available:   return "ocMessageId::Camera_Image_Available";
  case ocMessageId::Binary_Image_Available:   return "ocMessageId::Binary_Image_Available";
  case ocMessageId::Birdseye_Image_Available: return "ocMessageId::Birdseye_Image_Available";
  case ocMessageId::Lane_Found:               return "ocMessageId::Lane_Found";
  case ocMessageId::Lines_Available:          return "ocMessageId::Lines_Available";
  case ocMessageId::Set_Lights:               return "ocMessageId::Set_Lights";
  case ocMessageId::Start_Driving_Task:       return "ocMessageId::Start_Driving_Task";
  case ocMessageId::Ai_Switched_State:        return "ocMessageId::Ai_Switched_State";
  case ocMessageId::Object_Found:             return "ocMessageId::Object_Found";
  case ocMessageId::Request_Timing_Sites:     return "ocMessageId::Request_Timing_Sites";
  case ocMessageId::Timing_Sites:             return "ocMessageId::Timing_Sites";
  case ocMessageId::Timing_Events:            return "ocMessageId::Timing_Events";
  case ocMessageId::Shapes:                   return "ocMessageId::Shapes";
  case ocMessageId::Approach_Point:           return "ocMessageId::Approach_Point";
  case ocMessageId::Imu_Rotation_Euler:       return "ocMessageId::Imu_Rotation_Euler";
  case ocMessageId::Imu_Rotation_Gyro:        return "ocMessageId::Imu_Rotation_Gyro";
  case ocMessageId::Imu_Linear_Acceleration:  return "ocMessageId::Imu_Linear_Acceleration";
  case ocMessageId::Imu_Rotation_Quaternion:  return "ocMessageId::Imu_Rotation_Quaternion";
  case ocMessageId::Send_Can_Frame:           return "ocMessageId::Send_Can_Frame";
  case ocMessageId::Can_Frame_Transmitted:    return "ocMessageId::Can_Frame_Transmitted";
  case ocMessageId::Can_Frame_Received:       return "ocMessageId::Can_Frame_Received";
  case ocMessageId::Received_Odo_Steps:       return "ocMessageId::Received_Odo_Steps";
  case ocMessageId::Received_Current_Speed:   return "ocMessageId::Received_Current_Speed";
  case ocMessageId::Received_Button_Press:    return "ocMessageId::Received_Button_Press";
  case ocMessageId::Driving_Task_Finished:    return "ocMessageId::Driving_Task_Finished";
  case ocMessageId::Rc_State_Changed:         return "ocMessageId::Rc_State_Changed";
  case ocMessageId::Set_Camera_Parameter:     return "ocMessageId::Set_Camera_Parameter";
  }
  return "<unknown>";
}

const char *to_string(ocObjectType object_type)
{
  switch (object_type)
  {
    case ocObjectType::None:                        return "ocObjectType::None";
    case ocObjectType::Signs:                       return "ocObjectType::Signs";
    case ocObjectType::Sign_Speed_Limit_Start:      return "ocObjectType::Sign_Speed_Limit_Start";
    case ocObjectType::Sign_Speed_Limit_End:        return "ocObjectType::Sign_Speed_Limit_End";
    case ocObjectType::Sign_Crosswalk:              return "ocObjectType::Sign_Crosswalk";
    case ocObjectType::Sign_Parking_Zone:           return "ocObjectType::Sign_Parking_Zone";
    case ocObjectType::Sign_Expressway_Start:       return "ocObjectType::Sign_Expressway_Start";
    case ocObjectType::Sign_Expressway_End:         return "ocObjectType::Sign_Expressway_End";
    case ocObjectType::Sign_Sharp_Turn_Left:        return "ocObjectType::Sign_Sharp_Turn_Left";
    case ocObjectType::Sign_Sharp_Turn_Right:       return "ocObjectType::Sign_Sharp_Turn_Right";
    case ocObjectType::Sign_Barred_Area:            return "ocObjectType::Sign_Barred_Area";
    case ocObjectType::Sign_Pedestrian_Island:      return "ocObjectType::Sign_Pedestrian_Island";
    case ocObjectType::Sign_Stop:                   return "ocObjectType::Sign_Stop";
    case ocObjectType::Sign_Priority:               return "ocObjectType::Sign_Priority";
    case ocObjectType::Sign_Yield:                  return "ocObjectType::Sign_Yield";
    case ocObjectType::Sign_Proceed_Left:           return "ocObjectType::Sign_Proceed_Left";
    case ocObjectType::Sign_Proceed_Right:          return "ocObjectType::Sign_Proceed_Right";
    case ocObjectType::Sign_Proceed_Straight:       return "ocObjectType::Sign_Proceed_Straight";
    case ocObjectType::Sign_No_Passing_Start:       return "ocObjectType::Sign_No_Passing_Start";
    case ocObjectType::Sign_No_Passing_End:         return "ocObjectType::Sign_No_Passing_End";
    case ocObjectType::Sign_Uphill:                 return "ocObjectType::Sign_Uphill";
    case ocObjectType::Sign_Downhill:               return "ocObjectType::Sign_Downhill";
    case ocObjectType::Sign_No_Entry:               return "ocObjectType::Sign_No_Entry";
    case ocObjectType::Sign_Dead_End:               return "ocObjectType::Sign_Dead_End";
    case ocObjectType::Road_Markings:               return "ocObjectType::Road_Markings";
    case ocObjectType::Road_Start_Line:             return "ocObjectType::Road_Start_Line";
    case ocObjectType::Road_Stop_Line:              return "ocObjectType::Road_Stop_Line";
    case ocObjectType::Road_Yield_Line:             return "ocObjectType::Road_Yield_Line";
    case ocObjectType::Road_Speed_Limit_Start:      return "ocObjectType::Road_Speed_Limit_Start";
    case ocObjectType::Road_Speed_Limit_End:        return "ocObjectType::Road_Speed_Limit_End";
    case ocObjectType::Road_Barred_Area:            return "ocObjectType::Road_Barred_Area";
    case ocObjectType::Road_Crosswalk:              return "ocObjectType::Road_Crosswalk";
    case ocObjectType::Road_Pedestrian_Island:      return "ocObjectType::Road_Pedestrian_Island";
    case ocObjectType::Road_Crossing_4Way:          return "ocObjectType::Road_Crossing_4Way";
    case ocObjectType::Road_Crossing_3WayLeft:      return "ocObjectType::Road_Crossing_3WayLeft";
    case ocObjectType::Road_Crossing_3WayRight:     return "ocObjectType::Road_Crossing_3WayRight";
    case ocObjectType::Road_Crossing_3WayT:         return "ocObjectType::Road_Crossing_3WayT";
    case ocObjectType::Road_Proceed_Left:           return "ocObjectType::Road_Proceed_Left";
    case ocObjectType::Road_Proceed_Right:          return "ocObjectType::Road_Proceed_Right";
    case ocObjectType::Road_Proceed_Straight:       return "ocObjectType::Road_Proceed_Straight";
    case ocObjectType::Road_Crossing_2WayLeft:      return "ocObjectType::Road_Crossing_2WayLeft";
    case ocObjectType::Road_Crossing_2WayRight:     return "ocObjectType::Road_Crossing_2WayRight";
    case ocObjectType::Parking_Space:               return "ocObjectType::Parking_Space";
    case ocObjectType::Parallel_Parking_Space:      return "ocObjectType::Parallel_Parking_Space";
    case ocObjectType::Perpendicular_Parking_Space: return "ocObjectType::Perpendicular_Parking_Space";
    case ocObjectType::Pedestrian:                  return "ocObjectType::Pedestrian";
    case ocObjectType::Pedestrian_Left:             return "ocObjectType::Pedestrian_Left";
    case ocObjectType::Pedestrian_Center:           return "ocObjectType::Pedestrian_Center";
    case ocObjectType::Pedestrian_Right:            return "ocObjectType::Pedestrian_Right";
    case ocObjectType::Obstacle:                    return "ocObjectType::Obstacle";
    case ocObjectType::Obstacle_Far:                return "ocObjectType::Obstacle_Far";
    case ocObjectType::Obstacle_Near:               return "ocObjectType::Obstacle_Near";
    case ocObjectType::Obstacle_Left:               return "ocObjectType::Obstacle_Left";
    case ocObjectType::Obstacle_Right:              return "ocObjectType::Obstacle_Right";
    case ocObjectType::Qr_Code:                     return "ocObjectType::Qr_Code";
  }
  return "<unknown>";
}

const char *to_string(ocImageType image_type)
{
  switch (image_type)
  {
  case ocImageType::None: return "ocImageType::None";
  case ocImageType::Cam:  return "ocImageType::Cam";
  case ocImageType::Bin:  return "ocImageType::Bin";
  case ocImageType::Bev:  return "ocImageType::Bev";
  }
  return "<unknown>";
}

const char *to_string(ocShapeType shape_type)
{
  switch (shape_type)
  {
  case ocShapeType::Shape:     return "ocShapeType::Shape";
  case ocShapeType::Point:     return "ocShapeType::Point";
  case ocShapeType::Circle:    return "ocShapeType::Circle";
  case ocShapeType::Line:      return "ocShapeType::Line";
  case ocShapeType::Rectangle: return "ocShapeType::Rectangle";
  }
  return "<unknown>";
}

uint8_t ocBevData::read_at(int32_t x, int32_t y) const
{
  size_t ix = (size_t)(x - min_map_x);
  size_t iy = (size_t)(y - min_map_y);
  size_t range_x = (size_t)(max_map_x - min_map_x);
  return img_buffer[ix + iy * range_x];
}
