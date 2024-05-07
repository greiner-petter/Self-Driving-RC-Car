#pragma once

#include <cstdint>

#include "ocConst.h"
#include "ocImageOps.h"
#include "ocTime.h"

#include <type_traits> // std::is_trivial_v

/**
 * This file contains all types that are transmitted via ipc messages or the
 * shared memory.
 */

enum class ocMemberId: uint16_t
{
    None               = 0,
    Camera             = 1,
    Image_Processing   = 2,
    Lane_Detection     = 3,
    Ai                 = 4,
    Sign_Detection     = 5,
    Ipc_Hub            = 6,
    Eth_Gateway        = 7,
    Can_Gateway        = 8,
    Video_Viewer       = 9,
    Video_Input        = 10,
    Video_Recorder     = 11,
    Command_Line       = 12,
    Qr_Detection       = 13,
    Obstacle_Detection = 14,
    Virtual_Car        = 15,
    Tachometer         = 16,
    Apriltag_Detection = 17,
};

const char *to_string(ocMemberId member_id);

enum class ocMessageId: uint16_t
{
    None                     = 0x00,

    Auth_Request             = 0x01,
    Auth_Response            = 0x02,
    Member_List              = 0x04,
    Subscribe_To_Messages    = 0x06,
    Deafen_Member            = 0x07,
    Mute_Member              = 0x08,
    Ipc_Stats                = 0x09,
    Disconnect_Me            = 0x0A,

    Camera_Image_Available   = 0x11,
    Binary_Image_Available   = 0x12,
    Birdseye_Image_Available = 0x13,

    Lane_Found               = 0x22,
    Lines_Available          = 0x23,

    Set_Lights               = 0x48,
    Start_Driving_Task       = 0x49,
    Ai_Switched_State        = 0x4C,

    Object_Found             = 0x52,

    Request_Timing_Sites     = 0xA1,
    Timing_Sites             = 0xA2,
    Timing_Events            = 0xA3,
    Shapes                   = 0xA4,
    Approach_Point           = 0xA5,

    Imu_Rotation_Euler       = 0xB0,
    Imu_Rotation_Gyro        = 0xB1,
    Imu_Linear_Acceleration  = 0xB2,
    Imu_Rotation_Quaternion  = 0xB3,

    Send_Can_Frame           = 0xC1,
    Can_Frame_Transmitted    = 0xC2,
    Can_Frame_Received       = 0xC3,
    Received_Odo_Steps       = 0xC4,
    Received_Current_Speed   = 0xC5,
    Received_Button_Press    = 0xC9,
    Driving_Task_Finished    = 0xCA,
    Rc_State_Changed         = 0xCB,

    Set_Camera_Parameter     = 0xD3,
};

const char *to_string(ocMessageId message_id);

enum class ocObjectType : uint32_t
{
    None                        = 0x0000,

    Signs                       = 0x0010,
    Sign_Speed_Limit_Start      = 0x0011,
    Sign_Speed_Limit_End        = 0x0012,
    Sign_Crosswalk              = 0x0013,
    Sign_Parking_Zone           = 0x0014,
    Sign_Expressway_Start       = 0x0015,
    Sign_Expressway_End         = 0x0016,
    Sign_Sharp_Turn_Left        = 0x0017,
    Sign_Sharp_Turn_Right       = 0x0018,
    Sign_Barred_Area            = 0x0019,
    Sign_Pedestrian_Island      = 0x001A,
    Sign_Stop                   = 0x001B,
    Sign_Priority               = 0x001C,
    Sign_Yield                  = 0x001D,
    Sign_Proceed_Left           = 0x001E,
    Sign_Proceed_Right          = 0x001F,
    Sign_Proceed_Straight       = 0x0020,
    Sign_No_Passing_Start       = 0x0021,
    Sign_No_Passing_End         = 0x0022,
    Sign_Uphill                 = 0x0023,
    Sign_Downhill               = 0x0024,
    Sign_No_Entry               = 0x0025,
    Sign_Dead_End               = 0x0026,

    Road_Markings               = 0x0040,
    Road_Start_Line             = 0x0041,
    Road_Stop_Line              = 0x0042,
    Road_Yield_Line             = 0x0043,
    Road_Speed_Limit_Start      = 0x0044,
    Road_Speed_Limit_End        = 0x0045,
    Road_Barred_Area            = 0x0046,
    Road_Crosswalk              = 0x0047,
    Road_Pedestrian_Island      = 0x0048,
    Road_Crossing_4Way          = 0x004B,
    Road_Crossing_3WayLeft      = 0x004C,
    Road_Crossing_3WayRight     = 0x004D,
    Road_Crossing_3WayT         = 0x004E,
    Road_Proceed_Left           = 0x004F,
    Road_Proceed_Right          = 0x0050,
    Road_Proceed_Straight       = 0x0051,
    Road_Crossing_2WayLeft      = 0x0052,
    Road_Crossing_2WayRight     = 0x0053,

    Parking_Space               = 0x0060,
    Parallel_Parking_Space      = 0x0061,
    Perpendicular_Parking_Space = 0x0062,

    Pedestrian                  = 0x0081,
    Pedestrian_Left             = 0x0082,
    Pedestrian_Center           = 0x0083,
    Pedestrian_Right            = 0x0084,
    Obstacle                    = 0x0085,
    Obstacle_Far                = 0x0086,
    Obstacle_Near               = 0x0087,
    Obstacle_Left               = 0x0088,
    Obstacle_Right              = 0x0089,
    Qr_Code                     = 0x008A,
};

const char *to_string(ocObjectType object_type);

struct ocDetectedObject final
{
    // Type of the detected object. See enum above for all options.
    ocObjectType object_type;

    // Number of the camera frame in which the object was detected.
    uint32_t frame_number;

    // Time when the frame was taken
    ocTime frame_time;

    // How far has the car to drive (from the time the camera image was taken)
    // until the object becomes relevant. What that means depends on the type
    // of object. Measured in cm. Some examples:
    // Stop line: distance until the car needs to stand still
    // Speed limit: distance after which the car needs to drive at limited speed
    // Obstacle: distance from the cars position to the beginning of the obstacle
    float distance_ahead;

    // Optional length of the object. Only relevant for obstacles right now.
    // Should be 0 if unused.
    float length;
};

struct ocAiStateChange final
{
    uint8_t state_name_len;
    char state_name[64];
    uint8_t state_change_param_len;
    char state_change_params[32];
    bool is_main;
};

enum class ocImageType
{
    None = 0,
    Cam  = 1, // camera image
    Bin  = 2, // binarized camera image
    Bev  = 3, // birds eye view
};

const char *to_string(ocImageType image_type);

enum class ocShapeType : uint16_t
{
    Shape     = 0,
    Point     = 1,
    Circle    = 2,
    Line      = 3,
    Rectangle = 4,
};

const char *to_string(ocShapeType shape_type);

struct ocPoint final
{
    int16_t x, y;
};
struct ocCircle final
{
    int16_t x, y, radius;
};
struct ocLine final
{
    int16_t x1, y1, x2, y2;
};
struct ocRectangle final
{
    int16_t left, top, right, bottom;
};

struct ocShape final
{
    ocShapeType type;
    union
    {
        ocPoint     point;
        ocCircle    circle;
        ocLine      line;
        ocRectangle rectangle;
    };
};

struct ocLaneData final
{
    ocTime   frame_time;
    uint32_t frame_number; // the camera frame this data was computed with
    float    curve_center_x; // positive = left, negative = right
    float    curve_center_y; // positive = forward, negative = backwards
    float    curve_radius;

    bool is_valid() const { return 0.0f != curve_radius; }
};

struct ocCamData final
{
    ocTime   frame_time;
    uint32_t frame_number;
    uint32_t width;
    uint32_t height;
    ocPixelFormat pixel_format;
    uint8_t  img_buffer[OC_CAM_BUFFER_SIZE];
};

struct ocBinData final
{
    ocTime   frame_time;
    uint32_t frame_number;
    uint32_t width;
    uint32_t height;
    uint8_t  img_buffer[OC_BIN_BUFFER_SIZE];
};

struct ocBevData final
{
    ocTime   frame_time;
    uint32_t frame_number;
    int32_t  min_map_x, max_map_x;
    int32_t  min_map_y, max_map_y;
    uint8_t  img_buffer[OC_BEV_BUFFER_SIZE];

    /**
     * Reads and returns the value of the given position from the map.
     * x and y are in car-relative coordinates at a 1cm scale.
     */
    uint8_t read_at(int32_t x, int32_t y) const;
};

// Shared Memory
struct ocSharedMemory final
{
    uint64_t _canary0;

    ocCamData cam_data[OC_NUM_CAM_BUFFERS];

    uint64_t _canary1;

    uint32_t last_written_cam_data_index;

    uint64_t _canary2;

    ocBinData bin_data[OC_NUM_BIN_BUFFERS];

    uint64_t _canary3;

    uint32_t last_written_bin_data_index;

    uint64_t _canary4;

    ocBevData bev_data[OC_NUM_BEV_BUFFERS];

    uint64_t _canary5;

    uint32_t last_written_bev_data_index;

    uint64_t _canary6;

    // bit mask of the processes that are currently running
    uint16_t online_members;

    uint64_t _canary7;
};

// Lines detected in the BEV
struct ocBevLines {
    int contour_num;
    int poly_num[NUMBER_OF_CONTOURS];
    int lines[NUMBER_OF_CONTOURS][NUMBER_OF_POLYGONS_PER_CONTOUR][2];
};

static_assert(std::is_trivial_v<ocSharedMemory>);
