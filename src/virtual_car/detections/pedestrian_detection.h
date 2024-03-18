#pragma once

#include "detection.h"

class simPedestrianDetection : public simDetection
{
private:

    bool _left_found;
    bool _center_found;
    bool _right_found;
    ocDetectedObject _left_obj;
    ocDetectedObject _center_obj;
    ocDetectedObject _right_obj;

    uint32_t _frame_number;
    ocTime   _frame_time;

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
            case ocMessageId::Object_Found:
            {
                ocDetectedObject obj = reader.read<ocDetectedObject>();
                if (ocObjectType::Pedestrian_Left   == obj.object_type)
                {
                    _left_obj   = obj;
                    _left_found = 0 < obj.length;
                }
                if (ocObjectType::Pedestrian_Center == obj.object_type)
                {
                    _center_obj   = obj;
                    _center_found = 0 < obj.length;
                }
                if (ocObjectType::Pedestrian_Right  == obj.object_type)
                {
                    _right_obj   = obj;
                    _right_found = 0 < obj.length;
                }
            } break;
            default: break;
        }

        // If the last detection of an object is too far in the past, they get reset.
        if (_left_found)
        {
            if (ocTime::seconds(1) < _frame_time - _left_obj.frame_time)
            {
                _left_found = false;
                _left_obj   = {};
            }
        }
        if (_center_found)
        {
            if (ocTime::seconds(1) < _frame_time - _center_obj.frame_time)
            {
                _center_found = false;
                _center_obj   = {};
            }
        }
        if (_right_found)
        {
            if (ocTime::seconds(1) < _frame_time - _right_obj.frame_time)
            {
                _right_found = false;
                _right_obj   = {};
            }
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        _left_found = false;
        _center_found = false;
        _right_found = false;

        auto obstacles = world.get_objects_visible_from(car, ocObjectType::Pedestrian);
        for (auto &obj : obstacles)
        {
            Vec2 rel_ped_pos = car.pose.specialize_pos(Vec3(obj.pose.pos.xy(), 0.0f)).xy();
            if (20.0f < rel_ped_pos.x && rel_ped_pos.x < 120.0f) {
                if (rel_ped_pos.y < -82.0f)
                {
                    // too far left
                }
                else if (rel_ped_pos.y < -60.0f)
                {
                    _left_found = true;
                    _left_obj   = {
                        .object_type    = ocObjectType::Pedestrian_Left,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = rel_ped_pos.x,
                        .length         = 5.0f
                    };
                }
                else if (rel_ped_pos.y < 20.0f)
                {
                    _center_found = true;
                    _center_obj   = {
                        .object_type    = ocObjectType::Pedestrian_Center,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = rel_ped_pos.x,
                        .length         = 5.0f
                    };
                }
                else if (rel_ped_pos.y < 42.0f)
                {
                    _right_found = true;
                    _right_obj   = {
                        .object_type    = ocObjectType::Pedestrian_Right,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = rel_ped_pos.x,
                        .length         = 5.0f
                    };
                }
            }
        }
    }

    bool next_object(ocPacket& packet) override
    {
        if (_left_found)
        {
            _left_found = false;
            packet.set_sender(ocMemberId::Sign_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_left_obj);
            return true;
        }
        if (_center_found)
        {
            _center_found = false;
            packet.set_sender(ocMemberId::Sign_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_center_obj);
            return true;
        }
        if (_right_found)
        {
            _right_found = false;
            packet.set_sender(ocMemberId::Sign_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_right_obj);
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
