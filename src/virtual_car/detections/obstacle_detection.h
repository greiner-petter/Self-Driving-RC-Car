#pragma once

#include "detection.h"
#include "../../common/ocSdfRenderer.h"

class simObstacleDetection : public simDetection
{
private:
    uint32_t _frame_number;
    ocTime   _frame_time;

    bool _near_obst_found = false;
    bool _far_obst_found  = false;

    ocDetectedObject _near_obst = {};
    ocDetectedObject _far_obst  = {};

    ocLaneData _lane_data = {};

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
            case ocMessageId::Lane_Found:
            {
                _lane_data = reader.read<ocLaneData>();
            } break;
            case ocMessageId::Object_Found:
            {
                ocDetectedObject obj = reader.read<ocDetectedObject>();
                if (ocObjectType::Obstacle_Near == obj.object_type)
                {
                    _near_obst = obj;
                    _near_obst_found = 0 < obj.length;
                }
                if (ocObjectType::Obstacle_Far  == obj.object_type)
                {
                    _far_obst  = obj;
                    _far_obst_found = 0 < obj.length;
                }
            } break;
            default: break;
        }

        // If the last detection of an object is too far in the past, they get reset.
        if (_near_obst_found)
        {
            if (ocTime::seconds(1) < _frame_time - _near_obst.frame_time)
            {
                _near_obst_found = false;
                _near_obst       = {};
            }
        }
        if (_far_obst_found)
        {
            if (ocTime::seconds(1) < _frame_time - _far_obst.frame_time)
            {
                _far_obst_found = false;
                _far_obst       = {};
            }
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        _near_obst_found = false;
        _far_obst_found  = false;
        _near_obst = {};
        _far_obst  = {};

        auto obstacles = world.get_objects_visible_from(car, ocObjectType::Obstacle);
        for (auto &obj : obstacles)
        {
            Vec2 obst_rel_pos = car.pose.specialize_pos(Vec3(obj.pose.pos.xy(), 0.0f)).xy();
            float obst_road_dist = length(obst_rel_pos - Vec2(_lane_data.curve_center_x, _lane_data.curve_center_y));
            if (std::abs(obst_road_dist - _lane_data.curve_radius) < 20.0f && 20.0f < obst_rel_pos.x) // is it on the road and in front of the car
            {
                float obst_car_dist = length(obst_rel_pos);
                Vec3 obst_size = obj.size;
                float obstacle_length = std::max(obst_size.x, obst_size.y);
                float obstacle_dist = obst_rel_pos.x;
                if (obst_car_dist < 70.0f)
                {
                    _near_obst_found = true;
                    _near_obst = {
                        .object_type    = ocObjectType::Obstacle_Near,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = obstacle_dist,
                        .length         = obstacle_length
                    };
                }
                else if (obst_car_dist < 120.0f)
                {
                    _far_obst_found = true;
                    _far_obst = {
                        .object_type    = ocObjectType::Obstacle_Far,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = obstacle_dist,
                        .length         = obstacle_length
                    };
                }
            }
        }
    }

    bool next_object(ocPacket& packet) override
    {
        if (_near_obst_found)
        {
            _near_obst_found = false;
            packet.set_sender(ocMemberId::Obstacle_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_near_obst);
            return true;
        }
        if (_far_obst_found)
        {
            _far_obst_found = false;
            packet.set_sender(ocMemberId::Obstacle_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_far_obst);
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
        if (_far_obst_found)
        {
            float start = _far_obst.distance_ahead;
            float end   = _far_obst.distance_ahead + _far_obst.length;
            Vec2 p1 = (car_to_screen * Vec4(start, -10.0f, 0.0f, 1.0f)).xy();
            Vec2 p2 = (car_to_screen * Vec4(start,  10.0f, 0.0f, 1.0f)).xy();
            Vec2 p3 = (car_to_screen * Vec4(end,   -10.0f, 0.0f, 1.0f)).xy();
            Vec2 p4 = (car_to_screen * Vec4(end,    10.0f, 0.0f, 1.0f)).xy();
            oc::render(target, oc::line(p1, p2, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p3, p4, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p1, p3, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p2, p4, 1.0f), {1.0f, 0.0f, 0.0f});
        }
        if (_near_obst_found)
        {
            float start = _near_obst.distance_ahead;
            float end   = _near_obst.distance_ahead + _near_obst.length;
            Vec2 p1 = (car_to_screen * Vec4(start, -10.0f, 0.0f, 1.0f)).xy();
            Vec2 p2 = (car_to_screen * Vec4(start,  10.0f, 0.0f, 1.0f)).xy();
            Vec2 p3 = (car_to_screen * Vec4(end,   -10.0f, 0.0f, 1.0f)).xy();
            Vec2 p4 = (car_to_screen * Vec4(end,    10.0f, 0.0f, 1.0f)).xy();
            oc::render(target, oc::line(p1, p2, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p3, p4, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p1, p3, 1.0f), {1.0f, 0.0f, 0.0f});
            oc::render(target, oc::line(p2, p4, 1.0f), {1.0f, 0.0f, 0.0f});
        }
    }
};

