#pragma once

#include <cstdint>

enum class TrafficSignType : uint16_t
{
    None = 0,
    SpeedLimit_7,
    SpeedLimit_10,
    SpeedLimit_20,
    SpeedLimit_30,
    SpeedLimit_50,
    SpeedLimit_60,
    SpeedLimit_70,
    SpeedLimit_80,
    SpeedLimit_90,
    SpeedLimit_100,
    SpeedLimit_120,
    GiveWay,                // Vorfahrt beachten
    COUNT
};

class TrafficSign
{
    TrafficSignType type = TrafficSignType::None;
    uint64_t distanceCM = 0;
};