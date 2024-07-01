#pragma once
#include "SignDetector.h"

class HaarSignDetector : public SignDetector
{
public:
    virtual void Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger, bool supportGUI) override;
    virtual void Tick() override;


    static std::filesystem::path GetStopSignXML();
    static std::filesystem::path GetLeftSignXML();
    static std::filesystem::path GetRightSignXML();
    static std::filesystem::path GetPrioritySignXML();
    static std::filesystem::path GetParkSignXML();

};
