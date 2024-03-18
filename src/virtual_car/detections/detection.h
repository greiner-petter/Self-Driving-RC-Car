#pragma once

#include "../../common/ocCommon.h"
#include "../../common/ocPacket.h"
#include "../ocOverviewMap.h"

class simDetection
{
public:
    virtual ~simDetection() = default;

    // quickly handle packets like ocMessageId_EnableObjectDetection etc. Nothing that takes time.
    virtual void handle_packet(const ocPacket& packet) = 0;

    // run the detection and store the found objects for later, to be queried by next_object.
    virtual void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) = 0;

    // This method will be called to get the objects that were found during run_detection.
    virtual bool next_object(ocPacket& packet) = 0;

    // Override this call if the detection has something to display. But it might not get
    // called because the user has disabled the ui.
    virtual void draw_ui(
        oc::Window&              target,
        const DrawContext&       context,
        const ocSimulationWorld& world,
        const ocCarState&        car) = 0;
};
