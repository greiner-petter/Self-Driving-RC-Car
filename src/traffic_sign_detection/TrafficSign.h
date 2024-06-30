#pragma once

#include <cstdint>
#include <string>

enum class TrafficSignType : uint16_t
{
    None = 0,
    Stop,
    PriorityRoad,                // "Vorfahrtstrasse"
    Park,
    Left,
    Right,
    COUNT
};

static std::string TrafficSignTypeToString(const TrafficSignType& type)
{
    switch (type)
    {
        case TrafficSignType::None: return "None";
        case TrafficSignType::Stop: return "Stop";
        case TrafficSignType::PriorityRoad: return "PriorityRoad";
        case TrafficSignType::Park: return "Park";
        case TrafficSignType::Left: return "Left";
        case TrafficSignType::Right: return "Right";
        default: return "TrafficSignTypeToString(): MISSING IMPLEMENTATION.";
    }
}

struct TrafficSign
{
    TrafficSignType type = TrafficSignType::None;
    uint64_t distanceCM = 0;
};
