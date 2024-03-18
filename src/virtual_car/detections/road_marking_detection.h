#pragma once

#include "detection.h"

class simRoadMarkingDetection : public simDetection
{
private:

    bool             _object_found = false;
    ocDetectedObject _object       = {};
    uint32_t         _frame_number = 0;
    ocTime           _frame_time   = {};

public:
    void handle_packet(const ocPacket& packet) override
    {
        auto reader = packet.read_from_start();
        switch (packet.get_message_id())
        {
            case ocMessageId::Camera_Image_Available:
            {
                _frame_time   = reader.read<ocTime>();
                _frame_number = reader.read<uint32_t>();
            } break;
            // TODO: handle object detection messages
            default: break;
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        _object_found = false;

        auto objects = world.get_objects_visible_from(car);
        for (auto& obj : objects)
        {
            if (ocObjectType::Road_Proceed_Right == obj.type ||
                ocObjectType::Road_Proceed_Left  == obj.type)
            {
                auto from_car = car.pose.specialize_pos(obj.pose.pos);
                if (20.0f < from_car.x &&  // not too close of behind the car
                    from_car.x < 100.0f && // not too far away
                    std::abs(from_car.y) < 20.0f && // within the lane
                    0.5f < dot(car.pose.x_axis(), obj.pose.x_axis())) // facing towards the car
                {
                    _object_found = true;
                    _object = {
                        .object_type    = obj.type,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = from_car.x,
                        .length         = obj.size.x
                    };
                }
            }
        }
    }

    bool next_object(ocPacket& packet) override
    {
        if (_object_found)
        {
            _object_found = false;
            packet.set_sender(ocMemberId::Lane_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_object);
            return true;
        }
        return false;
    }

    void draw_ui(
        oc::Window&,
        const DrawContext&,
        const ocSimulationWorld&,
        const ocCarState&) override
    {

    }
};
