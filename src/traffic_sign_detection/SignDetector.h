#pragma once
#include "../common/ocMember.h"
#include "TrafficSign.h"

#include <filesystem>
#include <memory>

#include <opencv2/core/types.hpp>

class SignDetector
{
public:
    virtual void Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger, bool supportGUI);
    virtual void Run() = 0;

    static float ConvertRectSizeToEstimatedDistance(float rectSize, double sizeFactor);
    static uint32_t ConvertRectToDistanceInCM(const cv::Rect& rect, const int cam_width, const int cam_height, double sizeFactor);

    static void SendPacket(TrafficSign sign);
    // This function remaps a given value in space from in1 to in2, into the space from out1 to out2
    template<typename T>
    static T Remap(T value, const T in1, const T in2, const T out1, const T out2)
    {
        return out1 + (value - in1) * (out2 - out1) / (in2 - in1);
    }
};
