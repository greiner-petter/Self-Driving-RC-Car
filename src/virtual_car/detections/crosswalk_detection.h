#pragma once

#include "detection.h"
#include "../../common/ocSdfRenderer.h"

class simCrosswalkDetection : public simDetection
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
            case ocMessageId::Object_Found:
            {
                ocDetectedObject obj = reader.read<ocDetectedObject>();
                if (ocObjectType::Road_Crosswalk == obj.object_type)
                {
                    _object = obj;
                    _object_found = 0 < obj.length;
                }
            } break;
            default: break;
        }

        // If the last detection of an object is too far in the past, they get reset.
        if (_object_found)
        {
            if (ocTime::seconds(1) < _frame_time - _object.frame_time)
            {
                _object_found = false;
                _object       = {};
            }
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        _object_found = false;
        _object = {};

        auto objects = world.get_objects_visible_from(car, ocObjectType::Road_Crosswalk);
        for (auto& obj : objects)
        {
            if (ocObjectType::Road_Crosswalk == obj.type)
            {
                auto from_car = car.pose.specialize_pos(obj.pose.pos);
                if (20.0f < from_car.x &&  // not too close of behind the car
                    from_car.x < 100.0f && // not too far away
                    -43.0f < from_car.y &&
                    from_car.y < 21.0f &&
                    0.5f < std::abs(dot(car.pose.x_axis(), obj.pose.x_axis()))) // facing towards the car
                {
                    _object_found = true;
                    _object = {
                        .object_type    = obj.type,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = from_car.x - 20.0f,
                        .length         = 40.0f
                    };
                }
            }
        }
    }

    bool next_object(ocPacket& packet)override
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
        oc::Window&              target,
        const DrawContext&       context,
        const ocSimulationWorld& /*world*/,
        const ocCarState&        car) override
    {
        Mat4 car_to_screen = context.world_to_screen_mat()
                           * car.pose.get_generalize_mat();
        if (_object_found)
        {
            float start = _object.distance_ahead;
            float end   = _object.distance_ahead + _object.length;
            Vec2 p1 = (car_to_screen * Vec4(start, -62.0f, 0.0f, 1.0f)).xy();
            Vec2 p2 = (car_to_screen * Vec4(start,  20.0f, 0.0f, 1.0f)).xy();
            Vec2 p3 = (car_to_screen * Vec4(end,   -62.0f, 0.0f, 1.0f)).xy();
            Vec2 p4 = (car_to_screen * Vec4(end,    20.0f, 0.0f, 1.0f)).xy();
            oc::render(target, oc::line(p1, p2, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p3, p4, 1.0f), {1.0f, 0.0f, 0.0f});
        }
    }
};
