#pragma once

#include "detection.h"
#include "../../common/ocArray.h"
#include "../../common/ocSdfRenderer.h"

class simSignDetection : public simDetection
{
private:

    ocArray<ocDetectedObject> _objects;
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
            // TODO: handle sign detection packets
            default: break;
        }
    }

    void run_detection(
        const ocSimulationWorld& world,
        const ocCarState&        car) override
    {
        _objects.clear();


        const float Pi = 3.14159265358979f;
        auto world_to_cam = ocPose::compose(car.pose, car.properties->cam.pose);

        auto obstacles = world.get_objects_visible_from(car);

        for (auto &obj : obstacles)
        {
            if (obj.is_sign())
            {
                Vec3 sign_facing = world_to_cam.specialize_dir(obj.pose.x_axis());
                Vec3 to_sign     = world_to_cam.specialize_pos(obj.pose.pos);
                float distance   = length(to_sign);
                float cos_sign_to_car = dot(sign_facing, -to_sign / distance);
                if (distance < 150.0f && // is the sign close enough
                    std::cos(30.0f / 180.0f * Pi) < cos_sign_to_car) // is the sign facing the camera (<30°)
                {
                    _objects.append({
                        .object_type    = obj.type,
                        .frame_number   = _frame_number,
                        .frame_time     = _frame_time,
                        .distance_ahead = to_sign.x,
                        .length         = 0.0f
                    });
                }
            }
        }
    }

    bool next_object(ocPacket& packet) override
    {
        if (!_objects.is_empty())
        {
            packet.set_sender(ocMemberId::Sign_Detection);
            packet.set_message_id(ocMessageId::Object_Found);
            packet.clear_and_edit().write(_objects.last());
            _objects.remove_last();
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

        for (auto &obj : _objects)
        {
            Vec2 left  = (car_to_screen * Vec4(obj.distance_ahead, -20.0f, 0.0f, 1.0f)).xy();
            Vec2 right = (car_to_screen * Vec4(obj.distance_ahead,  20.0f, 0.0f, 1.0f)).xy();
            oc::render(target, oc::line(left, right, 1.0f), {1.0f, 0.0f, 0.0f});
        }
    }
};
