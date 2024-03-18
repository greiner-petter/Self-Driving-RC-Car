#pragma once

#include "detection.h"
#include "../../common/ocSdfRenderer.h"

class simParkingSpaceDetection : public simDetection
{
private:
    uint32_t _frame_number              = 0;
    ocTime   _frame_time                = {};

    bool _parallel_found                = false;
    bool _perpendicular_found           = false;
    ocDetectedObject _parallel_obj      = {};
    ocDetectedObject _perpendicular_obj = {};

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
                if (ocObjectType::Parallel_Parking_Space == obj.object_type)
                {
                    _parallel_obj = obj;
                    _parallel_found = 0 < obj.length;
                }
                if (ocObjectType::Perpendicular_Parking_Space == obj.object_type)
                {
                    _perpendicular_obj = obj;
                    _perpendicular_found = 0 < obj.length;
                }
            } break;
            default: break;
        }

        // If the last detection of an object is too far in the past, they get reset.
        if (_parallel_found)
        {
            if (ocTime::seconds(1) < _frame_time - _parallel_obj.frame_time)
            {
                _parallel_found = false;
                _parallel_obj   = {};
            }
        }
        if (_perpendicular_found)
        {
            if (ocTime::seconds(1) < _frame_time - _perpendicular_obj.frame_time)
            {
                _perpendicular_found = false;
                _perpendicular_obj   = {};
            }
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        _parallel_found      = false;
        _perpendicular_found = false;
        _parallel_obj      = {};
        _perpendicular_obj = {};

        Vec2 look_at = car.pose.generalize_pos(Vec3(30.0f, 0.0f, 0.0f)).xy();
        auto tile = world.get_tile_at(look_at);
        if (tile)
        {
            auto objects = tile->get_objects();
            for (auto& obj : objects)
            {
                if (ocObjectType::Parallel_Parking_Space == obj.type)
                {
                    auto from_car = car.pose.specialize_pos(obj.pose.pos);
                    if (20.0f < from_car.x)
                    {
                        _parallel_found = true;
                        _parallel_obj = {
                            .object_type    = obj.type,
                            .frame_number   = _frame_number,
                            .frame_time     = _frame_time,
                            .distance_ahead = from_car.x - obj.size.x * 0.5f,
                            .length         = obj.size.x
                        };
                    }
                }
                if (ocObjectType::Perpendicular_Parking_Space == obj.type)
                {
                    auto from_car = car.pose.specialize_pos(obj.pose.pos);
                    if (20.0f < from_car.x)
                    {
                        _perpendicular_found = true;
                        _perpendicular_obj = {
                            .object_type    = obj.type,
                            .frame_number   = _frame_number,
                            .frame_time     = _frame_time,
                            .distance_ahead = from_car.x - obj.size.x * 0.5f,
                            .length         = obj.size.x
                        };
                    }
                }
            }
        }
    }

    bool next_object(ocPacket& packet) override
    {
        if (_parallel_found)
        {
            _parallel_found = false;
            packet.set_sender(ocMemberId::Lane_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_parallel_obj);
            return true;
        }
        if (_perpendicular_found)
        {
            _perpendicular_found = false;
            packet.set_sender(ocMemberId::Lane_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_perpendicular_obj);
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
        if (0 < _parallel_obj.length)
        {
            float start = _parallel_obj.distance_ahead;
            float end   = _parallel_obj.distance_ahead + _parallel_obj.length;
            Vec2 p1 = (car_to_screen * Vec4(start, -20.0f, 0.0f, 1.0f)).xy();
            Vec2 p2 = (car_to_screen * Vec4(start,  50.0f, 0.0f, 1.0f)).xy();
            Vec2 p3 = (car_to_screen * Vec4(end,   -20.0f, 0.0f, 1.0f)).xy();
            Vec2 p4 = (car_to_screen * Vec4(end,    50.0f, 0.0f, 1.0f)).xy();
            oc::render(target, oc::line(p1, p2, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p3, p4, 1.0f), {1.0f, 0.0f, 0.0f});
        }
        if (0 < _perpendicular_obj.length)
        {
            float start = _perpendicular_obj.distance_ahead;
            float end   = _perpendicular_obj.distance_ahead + _perpendicular_obj.length;
            Vec2 p1 = (car_to_screen * Vec4(start, -110.0f, 0.0f, 1.0f)).xy();
            Vec2 p2 = (car_to_screen * Vec4(start,  -20.0f, 0.0f, 1.0f)).xy();
            Vec2 p3 = (car_to_screen * Vec4(end,   -110.0f, 0.0f, 1.0f)).xy();
            Vec2 p4 = (car_to_screen * Vec4(end,    -20.0f, 0.0f, 1.0f)).xy();
            oc::render(target, oc::line(p1, p2, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p3, p4, 1.0f), {1.0f, 0.0f, 0.0f});
        }
    }
};

