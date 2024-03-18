#pragma once

#include "detection.h"
#include "../../common/ocSdfRenderer.h"

class simLaneDetection: public simDetection
{
private:
    bool       _lane_present = false;
    ocLaneData _lane_data    = {};
    ocTime     _frame_time   = {};
    uint32_t   _frame_number = 0;

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
                _lane_present = _lane_data.is_valid();
            } break;
            default: break;
        }

        // If the last detection of the lane is too far in the past, reset it.
        if (_lane_present)
        {
            if (ocTime::seconds(1) < _frame_time - _lane_data.frame_time)
            {
                _lane_present = false;
                _lane_data    = {};
            }
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        Vec2 look_at = car.pose.generalize_pos(Vec3(50.0f, 0.0f, 0.0f)).xy(); // look 50cm ahead (~25cm in front of car)
        auto lane = world.get_lane_at(look_at);
        Vec2 rel_circle = car.pose.specialize_pos(Vec3(lane.center, 0.0f)).xy();

        if (0.0f < lane.radius)
        {
            float offset_x = random_float(-2.0f, 2.0f);
            float offset_y = random_float(-2.0f, 2.0f);
            float offset_r = random_float(-1.0f, 1.0f) + (rel_circle.y < 0 ? -offset_y : offset_y);

            _lane_data.curve_center_x = rel_circle.x + offset_x;
            _lane_data.curve_center_y = rel_circle.y + offset_y;
            _lane_data.curve_radius = lane.radius + offset_r;
            _lane_data.frame_number = _frame_number;
            _lane_data.frame_time   = _frame_time;

            _lane_present = true;
        }
        else
        {
            _lane_present = false;
        }
    }

    bool next_object(ocPacket& packet) override
    {
        if (_lane_present)
        {
            _lane_present = false;
            packet.set_sender(ocMemberId::Lane_Detection);
            packet.set_message_id(ocMessageId::Lane_Found);
            packet.clear_and_edit().write(_lane_data);
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
        if (_lane_present)
        {
            Vec2 curve_center = (car_to_screen * Vec4(_lane_data.curve_center_x, _lane_data.curve_center_y, 0.0f, 1.0f)).xy();
            float curve_radius = _lane_data.curve_radius * context.scale;
            float min_radius = 20.0f * context.scale;

            // need to check if the circle is visible, due to a opencv performance problem.
            float dist =
                std::max(
                    std::max(-curve_center.x - 20.0f, curve_center.x - (float)target.get_width()  - 20.0f),
                    std::max(-curve_center.y - 20.0f, curve_center.y - (float)target.get_height() - 20.0f));
            if (dist < curve_radius && curve_radius < dist + (float)target.get_width())
            {
                if (min_radius < curve_radius)
                {
                    oc::render(target, oc::outline(oc::circle(curve_center, curve_radius - min_radius), 1.0f), {1.0f, 0.0f, 0.0f});
                }
                oc::render(target, oc::outline(oc::circle(curve_center, curve_radius + min_radius), 1.0f), {1.0f, 0.0f, 0.0f});
            }
        }

    }
};
