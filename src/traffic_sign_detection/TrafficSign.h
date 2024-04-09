#pragma once

#include <cstdint>

enum class TrafficSignType : uint16_t
{
    None = 0,
    Stop,
    GiveWay,                // Vorfahrt beachten
    Park,
    Left,
    Right,
    COUNT
};

struct TrafficSign
{
    TrafficSignType type = TrafficSignType::None;
    uint64_t distanceCM = 0;
};