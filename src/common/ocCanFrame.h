#pragma once

#include "ocAssert.h"
#include "ocTime.h"

#include <cstdint> // _t int
#include <cstring> // memcpy
#include <type_traits> // std::is_trivial_v

enum class ocCanId : uint32_t
{
    Set_Speed          = 0x11,
    Set_Steering       = 0x12,
    Set_Task           = 0x13,
    Task_Report        = 0x14,

    Digital_Input      = 0x21,
    Analog_Input       = 0x22,

    Odo_Front          = 0x31,
    Odo_Rear           = 0x32,

    Imu_Rotation_Euler = 0x40,
    Imu_Rotation       = 0x41,
    Imu_Acceleration   = 0x42,
    Imu_Reset          = 0x43,
    Imu_Calibrate      = 0x44,
    Imu_Quaternion     = 0x45,

    Heartbeat_EDI      = 0x61,
    Heartbeat_SAM1     = 0x62,
    Heartbeat_SAM2     = 0x63,
    Heartbeat_SAM3     = 0x64,
    Heartbeat_SAM4     = 0x65,

    Boot_Complete      = 0x61,
    Reset_Controller   = 0x62,

    Set_Parameter      = 0x71,
    Get_Parameter      = 0x72,

    Com_Error          = 0x81,
    Imu_Not_Found      = 0x82,

    Light_Absolute     = 0x91,
    Light_Relative     = 0x92
};

enum class ocCanInputId : uint8_t
{
    Aux      = 1,
    Throttle = 2,
    Steering = 3,
    Button1  = 4,
    Button2  = 5
};

enum class ocCanLightId : uint8_t
{
    Headlights      = 1,
    Brakelights     = 2,
    Indicator_Left  = 3,
    Indicator_Right = 4,
    Rc_Indicator    = 5
};

struct ocCanFrame
{
    ocTime timestamp;
    uint32_t index;

    ocCanId  id;
    uint32_t length;
    std::byte data[8];

    template <typename T>
    T read()
    {
        static_assert(std::is_trivial_v<T>);
        oc_assert(index + sizeof(T) <= length, index, sizeof(T), length);
        T result;
        memcpy(&result, &data[index], sizeof(T));
        index += (uint32_t)sizeof(T);
        return result;
    }

    template <typename T>
    void write(T value)
    {
        static_assert(std::is_trivial_v<T>);
        oc_assert(index + sizeof(T) <= 8, index, sizeof(T));
        memcpy(&data[index], &value, sizeof(T));
        index += (uint32_t)sizeof(T);
        if (length < index) length = index;
    }

    void write(const void *src_data, uint32_t src_length)
    {
        oc_assert(index + src_length <= 8, index, src_length);
        memcpy(&data[index], src_data, src_length);
        index += src_length;
        if (length < index) length = index;
    }

    void reset_pos()
    {
        index = 0;
    }

    void clear()
    {
        length = 0;
        index = 0;
    }
};
