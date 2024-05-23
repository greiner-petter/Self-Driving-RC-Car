#pragma once
#include "../common/ocMember.h"
#include "TrafficSign.h"

#include <filesystem>
#include <memory>

class SignDetector
{
public:
    static void Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger);
    static void Run();


    static std::filesystem::path GetStopSignXML();
    static std::filesystem::path GetLeftSignXML();
    static std::filesystem::path GetRightSignXML();

    // This function remaps a given value in space from in1 to in2, into the space from out1 to out2
    template<typename T>
    static T Remap(T value, const T in1, const T in2, const T out1, const T out2)
    {
        return out1 + (value - in1) * (out2 - out1) / (in2 - in1);
    }
};
