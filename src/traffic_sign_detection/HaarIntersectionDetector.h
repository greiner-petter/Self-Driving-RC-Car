#pragma once
#include "SignDetector.h"

class HaarIntersectionDetector : public SignDetector
{
public:
    virtual void Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger, bool supportGUI) override;
    virtual void Tick() override;


    static std::filesystem::path GetCrossingLeftXML();

};
