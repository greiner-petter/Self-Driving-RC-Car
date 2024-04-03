#pragma once

#include <cstdint>

enum class TrafficSignType : uint16_t
{
    None = 0,
    SpeedLimit_50,
    COUNT
};

class TrafficSign
{
    TrafficSignType type = TrafficSignType::None;
    uint64_t distanceCM = 0;
};