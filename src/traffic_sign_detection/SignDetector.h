#pragma once
#include "../common/ocMember.h"

#include <filesystem>

class SignDetector
{
public:
    static void Init(ocIpcSocket* socket, ocLogger* logger);
    static void Run();


    static std::filesystem::path GetStopSignXML();
};
