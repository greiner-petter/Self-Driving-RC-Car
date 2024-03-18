#pragma once

#include "../common/ocCar.h"
#include "../common/ocRect.h"
#include "../common/ocSdfRenderer.h"
#include "../common/ocVec.h"
#include "../common/ocWindow.h"
#include "ocSimulationWorld.h"

struct DrawContext
{
    float scale;
    Vec2  offset;
    int   width;
    int   height;

    Vec3 screen_to_world(Vec2 screen) const
    {
        return Vec3((screen - offset) / scale, 0.0f);
    }
    Vec3 screen_to_world(float screen_x, float screen_y) const
    {
        return screen_to_world(Vec2(screen_x, screen_y));
    }
    Vec2 world_to_screen(Vec3 world) const
    {
        return world.xy() * scale + offset;
    }
    Vec2 world_to_screen(Vec2 world) const
    {
        return world * scale + offset;
    }
    Vec2 world_to_screen(float world_x, float world_y, float world_z = 0.0f) const
    {
        return world_to_screen(Vec3(world_x, world_y, world_z));
    }

    Mat4 world_to_screen_mat() const
    {
        return Mat4::translate(offset, 0.0f)
             * Mat4::scale(scale, scale, 1.0f, 1.0f);
    }

    Mat4 screen_to_world_mat() const
    {
        return Mat4::scale(1.0f / scale, 1.0f / scale, 1.0f, 1.0f)
             * Mat4::translate(-offset, 0.0f);
    }

    void center_at(Vec2 world)
    {
        offset.x = (float)width  * 0.5f - world.x * scale;
        offset.y = (float)height * 0.5f - world.y * scale;
    }

    void center_at(Rect world)
    {
        scale = std::min((float)width / world.width(), (float)height / world.height());
        offset.x = (float)width  * 0.5f - world.center().x * scale;
        offset.y = (float)height * 0.5f - world.center().y * scale;
    }

    Rect get_visible_world_rect() const
    {
        return {
            -offset.x / scale,
            -offset.y / scale,
            ((float)width  - offset.x) / scale,
            ((float)height - offset.y) / scale
        };
    }
};

void draw_rotated_rectangle(
    oc::Window& window,
    Vec2 center,
    Vec2 dims,
    float angle,
    const oc::Color &color)
{
    float nx = std::cos(angle) * 0.5f;
    float ny = std::sin(angle) * 0.5f;
    oc::render(
        window,
        oc::polygon(
            center.x + dims.x * nx + dims.y * ny,
            center.y + dims.x * ny - dims.y * nx,
            center.x + dims.x * nx - dims.y * ny,
            center.y + dims.x * ny + dims.y * nx,
            center.x - dims.x * nx - dims.y * ny,
            center.y - dims.x * ny + dims.y * nx,
            center.x - dims.x * nx + dims.y * ny,
            center.y - dims.x * ny - dims.y * nx),
        color);
}

bool is_point_in_rotated_rectangle(
    Vec2 center,
    Vec2 dims,
    float angle,
    Vec2 point)
{
    float nx = std::cos(angle) * 0.5f;
    float ny = std::sin(angle) * 0.5f;
    auto sdf = oc::polygon(
        center.x + dims.x * nx + dims.y * ny,
        center.y + dims.x * ny - dims.y * nx,
        center.x + dims.x * nx - dims.y * ny,
        center.y + dims.x * ny + dims.y * nx,
        center.x - dims.x * nx - dims.y * ny,
        center.y - dims.x * ny + dims.y * nx,
        center.x - dims.x * nx + dims.y * ny,
        center.y - dims.x * ny - dims.y * nx);
    return sdf(point.x, point.y) <= 0.0f;
}

void draw_map(oc::Window& window, int overview_width, int overview_height, const float *overview_base)
{
    for (int y = 0; y < overview_height; ++y)
    for (int x = 0; x < overview_width;  ++x)
    {
        oc::Color color {
            overview_base[(y * overview_width + x) * 4 + 0],
            overview_base[(y * overview_width + x) * 4 + 1],
            overview_base[(y * overview_width + x) * 4 + 2]
        };
        window.draw_pixel(x, y, color);
    }
}

void draw_areas(oc::Window& window, const ocSimulationWorld& world, DrawContext& draw_context)
{
    for (auto &trigger : world.triggers)
    {
        oc::Color color {0.5f, 0.5f, 0.5f};
        if (trigger.trigger_off) color = {1.0f, 0.0f, 0.0f};
        if (trigger.trigger_on)  color = {0.0f, 1.0f, 0.0f};
        if (trigger.no_driving && !trigger.triggerable) color = {0.25f, 0.25f, 1.0f};
        if (trigger.no_driving &&  trigger.triggerable && !world.triggers_active) color = {0.5f, 0.0f, 0.5f};
        if (trigger.no_driving &&  trigger.triggerable &&  world.triggers_active) color = {1.0f, 0.25f, 1.0f};
        for (size_t v = 1; v < trigger.vertex_count; ++v)
        {
            Vec2 a = draw_context.world_to_screen(trigger.vertices[v - 1]);
            Vec2 b = draw_context.world_to_screen(trigger.vertices[v]);
            oc::render(window, oc::line(a, b, 2.0f), color);
        }
        Vec2 a = draw_context.world_to_screen(trigger.vertices[trigger.vertex_count - 1]);
        Vec2 b = draw_context.world_to_screen(trigger.vertices[0]);
        oc::render(window, oc::line(a, b, 2.0f), color);
    }
}

void draw_grid(oc::Window& window, DrawContext& draw_context)
{
    oc::Color color {0.75f, 0.75f, 0.75f};
    Vec3 top_left     = draw_context.screen_to_world(0.0f, 0.0f);
    Vec3 bottom_right = draw_context.screen_to_world((float)window.get_width(), (float)window.get_height());
    top_left = floor(top_left / 200.0f - Vec3(0.5f, 0.5f, 0.0f)) * 200.0f + Vec3(100.0f, 100.0f, 0.0f);
    for (float y = top_left.y; y <= bottom_right.y; y += 200.0f)
    {
        Vec2 a = draw_context.world_to_screen(Vec2(top_left.x,     y));
        Vec2 b = draw_context.world_to_screen(Vec2(bottom_right.x, y));
        oc::render(window, oc::line(a, b, 1.0f), color);
    }

    for (float x = top_left.x; x <= bottom_right.x; x += 200.0f)
    {
        Vec2 a = draw_context.world_to_screen(Vec2(x, top_left.y    ));
        Vec2 b = draw_context.world_to_screen(Vec2(x, bottom_right.y));
        oc::render(window, oc::line(a, b, 1.0f), color);
    }
}

bool is_point_in_car(const ocCarState& car, Vec2 world_point)
{
    Vec2 car_dims     = Vec2(car.properties->wheel_base, car.properties->axle_width);
    Vec2 wheel_dims   = Vec2(car.properties->wheel.diameter, car.properties->wheel.width);
    Vec2 car_center   = car.pose.generalize_pos(car.properties->car_center()).xy();
    float car_heading = car.pose.heading;
    Vec2 car_points[] = {
        car.pose.generalize_pos(car.properties->wheel_center_fl()).xy(),
        car.pose.generalize_pos(car.properties->wheel_center_fr()).xy(),
        car.pose.generalize_pos(car.properties->wheel_center_rl()).xy(),
        car.pose.generalize_pos(car.properties->wheel_center_rr()).xy()
    };

    return is_point_in_rotated_rectangle(car_center, car_dims, car_heading, world_point)
        || is_point_in_rotated_rectangle(car_points[0], wheel_dims, car_heading + car.steering_front, world_point)
        || is_point_in_rotated_rectangle(car_points[1], wheel_dims, car_heading + car.steering_front, world_point)
        || is_point_in_rotated_rectangle(car_points[2], wheel_dims, car_heading + car.steering_rear, world_point)
        || is_point_in_rotated_rectangle(car_points[3], wheel_dims, car_heading + car.steering_rear, world_point);
}

enum class ManipulatorState
{
    Default,
    X_Hover,
    Y_Hover,
    R_Hover,
    X_Active,
    Y_Active,
    R_Active
};

ManipulatorState get_manipulator_state(Vec2 man_pos, float r_angle, Vec2 mouse_pos, bool lmb_down)
{
    const float arm_start =  20.0f;
    const float arm_end   = 100.0f;
    const float arm_width =  10.0f;
    const float tip_size  =  20.0f;
    Vec2 d = mouse_pos - man_pos;
    Vec2 rotator = angle_to_vector(r_angle) * arm_end;
    if (distance(d, rotator) < tip_size * 0.5f)
    {
        return (lmb_down) ? ManipulatorState::R_Active : ManipulatorState::R_Hover;
    }
    else if (d.y < d.x)
    {
        if (arm_start < d.x && d.x < arm_end && std::abs(d.y) < arm_width)
        {
            return (lmb_down) ? ManipulatorState::X_Active : ManipulatorState::X_Hover;
        }
    }
    else
    {
        if (arm_start < d.y && d.y < arm_end && std::abs(d.x) < arm_width)
        {
            return (lmb_down) ? ManipulatorState::Y_Active : ManipulatorState::Y_Hover;
        }
    }
    return ManipulatorState::Default;
}

void draw_manipulator(oc::Window& window, Vec2 pos, float angle, ManipulatorState state)
{
    const float arm_start =  20.0f;
    const float arm_end   = 100.0f;
    const float tip_size  =  20.0f;
    const float default_arrow_width = 5.0f;
    const float hover_arrow_width   = 6.0f;
    const float active_arrow_width  = 7.0f;
    const float default_r_radius    = 10.0f;
    const float hover_r_radius      = 11.0f;
    const float active_r_radius     = 12.0f;
    const oc::Color default_red   = {0.7f, 0.1f, 0.1f};
    const oc::Color hover_red     = {0.9f, 0.2f, 0.2f};
    const oc::Color active_red    = {1.0f, 0.3f, 0.3f};
    const oc::Color default_blue  = {0.1f, 0.1f, 0.7f};
    const oc::Color hover_blue    = {0.2f, 0.2f, 0.9f};
    const oc::Color active_blue   = {0.3f, 0.3f, 1.0f};
    const oc::Color default_green = {0.1f, 0.7f, 0.1f};
    const oc::Color hover_green   = {0.2f, 0.9f, 0.2f};
    const oc::Color active_green  = {0.3f, 1.0f, 0.3f};

    Vec2 base_x = pos + Vec2(arm_start, 0.0f);
    Vec2 tip_x  = pos + Vec2(arm_end, 0.0f);
    Vec2 arm_x0 = pos + Vec2(arm_end - tip_size,  tip_size);
    Vec2 arm_x1 = pos + Vec2(arm_end - tip_size, -tip_size);
    Vec2 base_y = pos + Vec2(0.0f, arm_start);
    Vec2 tip_y  = pos + Vec2(0.0f, arm_end);
    Vec2 arm_y0 = pos + Vec2( tip_size, arm_end - tip_size);
    Vec2 arm_y1 = pos + Vec2(-tip_size, arm_end - tip_size);
    Vec2 base_r = pos + angle_to_vector(angle) * arm_start;
    Vec2 tip_r  = pos + angle_to_vector(angle) * arm_end;

    float x_width = default_arrow_width;
    float y_width = default_arrow_width;
    float r_width = default_r_radius;
    oc::Color x_color = default_red;
    oc::Color y_color = default_blue;
    oc::Color r_color = default_green;
    switch (state)
    {
        case ManipulatorState::X_Hover:  x_width = hover_arrow_width;  x_color = hover_red;    break;
        case ManipulatorState::Y_Hover:  y_width = hover_arrow_width;  y_color = hover_blue;   break;
        case ManipulatorState::R_Hover:  r_width = hover_r_radius;     r_color = hover_green;  break;
        case ManipulatorState::X_Active: x_width = active_arrow_width; x_color = active_red;   break;
        case ManipulatorState::Y_Active: y_width = active_arrow_width; y_color = active_blue;  break;
        case ManipulatorState::R_Active: r_width = active_r_radius;    r_color = active_green; break;
        default: break;
    }

    oc::render(window, oc::line(base_x, tip_x, x_width), x_color); 
    oc::render(window, oc::line(arm_x0, tip_x, x_width), x_color);
    oc::render(window, oc::line(arm_x1, tip_x, x_width), x_color);
    oc::render(window, oc::line(base_y, tip_y, y_width), y_color);
    oc::render(window, oc::line(arm_y0, tip_y, y_width), y_color);
    oc::render(window, oc::line(arm_y1, tip_y, y_width), y_color);
    oc::render(window, oc::line(base_r, tip_r, 1.0f), r_color);
    oc::render(window, oc::circle(tip_r, r_width), r_color);
}

void draw_car(oc::Window& window, const ocCarState& car, DrawContext& draw_context)
{
    auto car_to_screen = [&](Vec3 v)
    {
        return draw_context.world_to_screen(car.pose.generalize_pos(v));
    };

    Vec2 car_dims = Vec2(car.properties->wheel_base, car.properties->axle_width);

    Vec2 car_center = car_to_screen(car.properties->car_center());
    Vec2 wheel_dims = Vec2(car.properties->wheel.diameter, car.properties->wheel.width) * draw_context.scale;
    float car_heading = car.pose.heading;
    Vec2 car_norm = angle_to_vector(car_heading);
    Vec2 car_points[] = {
        car_to_screen(car.properties->wheel_center_fl()),
        car_to_screen(car.properties->wheel_center_fr()),
        car_to_screen(car.properties->wheel_center_rl()),
        car_to_screen(car.properties->wheel_center_rr())
    };

    draw_rotated_rectangle(window, car_center, car_dims * draw_context.scale, car_heading, {1.0f, 0.0f, 0.0f});
    draw_rotated_rectangle(window, car_points[0], wheel_dims, car_heading + car.steering_front, {0.0f, 1.0f, 0.0f});
    draw_rotated_rectangle(window, car_points[1], wheel_dims, car_heading + car.steering_front, {0.0f, 1.0f, 0.0f});
    draw_rotated_rectangle(window, car_points[2], wheel_dims, car_heading + car.steering_rear, {0.0f, 1.0f, 0.0f});
    draw_rotated_rectangle(window, car_points[3], wheel_dims, car_heading + car.steering_rear, {0.0f, 1.0f, 0.0f});
    if (car.lights.indicator_left)
    {
        draw_rotated_rectangle(window, car_points[0] + car_norm * 2, Vec2(4, 4), car_heading, {1.0f, 0.8f, 0.05f});
        draw_rotated_rectangle(window, car_points[2] - car_norm * 2, Vec2(4, 4), car_heading, {1.0f, 0.8f, 0.05f});
    }
    if (car.lights.indicator_right)
    {
        draw_rotated_rectangle(window, car_points[1] + car_norm * 2, Vec2(4, 4), car_heading, {1.0f, 0.8f, 0.05f});
        draw_rotated_rectangle(window, car_points[3] - car_norm * 2, Vec2(4, 4), car_heading, {1.0f, 0.8f, 0.05f});
    }
    if (car.lights.headlights)
    {
        draw_rotated_rectangle(window, car_points[0] + car_norm * 2, Vec2(4, 4), car_heading, {1.0f, 1.0f, 1.0f});
        draw_rotated_rectangle(window, car_points[1] + car_norm * 2, Vec2(4, 4), car_heading, {1.0f, 1.0f, 1.0f});
    }
    // TODO: reverse and brake lights
}

void draw_action(oc::Window& window, const ocCarState& car, const ocCarAction& action, DrawContext& draw_context)
{
    Vec2 car_pos = draw_context.world_to_screen(car.pose.pos);
    float car_heading = car.pose.heading;
    Vec2 car_norm = angle_to_vector(car_heading);

    if ((action.speed < 0.0f && action.distance < car.milage()) || (0.0f < action.speed && car.milage() < action.distance))
    {
        float distance = action.distance - car.milage();
        if (action.steering_front == action.steering_rear)
        {
            Vec2 n = angle_to_vector(car_heading + action.steering_front);
            Vec2 line_end = draw_context.world_to_screen(car.pose.pos.xy() + n * distance);
            oc::render(window, oc::line(car_pos, line_end, 1.0f), {0.0f, 1.0f, 1.0f});
        }
        else
        {
            Vec2 pivot;
            car.steering_to_pivot(action.steering_front, action.steering_rear, &pivot.x, &pivot.y);
            float radius = car.pivot_to_radius(pivot.x, pivot.y) * draw_context.scale;
            pivot = draw_context.world_to_screen(pivot);
  
            auto action_circle = oc::outline(oc::circle(pivot, radius), 1.0f);
            auto clip_circle = oc::circle(car_pos, distance * draw_context.scale);
            auto half = (action.speed < 0.0f) ? oc::half(car_norm.x, car_norm.y, car_pos.x, car_pos.y) : oc::half(-car_norm.x, -car_norm.y, car_pos.x, car_pos.y);
            oc::render(
                window,
                (int)std::floor(std::max(action_circle.bounds().min_x, clip_circle.bounds().min_x)),
                (int)std::floor(std::max(action_circle.bounds().min_y, clip_circle.bounds().min_y)),
                (int)std::ceil(std::min(action_circle.bounds().max_x, clip_circle.bounds().max_x)),
                (int)std::ceil(std::min(action_circle.bounds().max_y, clip_circle.bounds().max_y)),
                oc::intersect(half, oc::intersect(action_circle, clip_circle)),
                {0.0f, 1.0f, 1.0f},
                1.0f);
        }
    }
}

void draw_fov(oc::Window& window, const ocCarState& car, DrawContext& draw_context)
{
    // Draw a grid to visualize the car's field of view on the ground
    Plane floor = {.facing_direction = {0.0f, 0.0f, 1.0f}, .offset = 0.0f};
    auto cam_pos = car.pose.generalize_pos(car.properties->cam.pose.pos);
    auto projector = car.make_projector();
    const int h_lines = 9;
    const int v_lines = 12;
    Vec2 points[h_lines * v_lines] = {};

    // Initialize the points
    for (int i = 0; i < h_lines; ++i)
    {
        for (int j = 0; j < v_lines; ++j)
        {
            Vec2 image_point = {
                (float)((int)car.properties->cam.image_width  * j / v_lines),
                (float)((int)car.properties->cam.image_height * i / h_lines)
            };
            Ray ray = {.origin = cam_pos, .direction = projector.ego_to_world(image_point)};
            float distance = intersect_ray_plane(ray, floor);
            // If the ray doesn't intersect the plane, the distance will be infinite and the projection will be NaN
            // which is ok, we're just not going to attempt to draw NaN points in the code below.
            points[i * v_lines + j] = draw_context.world_to_screen(ray.origin + ray.direction * distance);
        }
    }
    // draw "horizontal" lines through points
    for (int i = 0; i < h_lines; ++i)
    {
        for (int j = 1; j < v_lines; ++j)
        {
            auto& p0 = points[i * v_lines + j - 1];
            auto& p1 = points[i * v_lines + j];
            if (!std::isnan(p0.x) && !std::isnan(p0.y) && !std::isnan(p1.x) && !std::isnan(p1.y))
            {
                oc::render(window, oc::line(p0, p1, 1.0f), {1.0f, 1.0f, 0.0f});
            }
        }
    }

    // draw "vertical" lines through points
    for (int j = 0; j < v_lines; ++j)
    {
        for (int i = 1; i < h_lines; ++i)
        {
            auto& p0 = points[(i - 1) * v_lines + j];
            auto& p1 = points[i * v_lines + j];
            if (!std::isnan(p0.x) && !std::isnan(p0.y) && !std::isnan(p1.x) && !std::isnan(p1.y))
            {
                oc::render(window, oc::line(p0, p1, 1.0f), {1.0f, 1.0f, 0.0f});
            }
        }
    }
}

void draw_help_text(oc::Window& /*window*/)
{
/*
TODO: add text rendering back
    if (0 <= fps && fps <= 999)
    {
        char fps_text[16];
        sprintf(fps_text, "fps: %3i", fps);

        cv::putText(overview, fps_text, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
    }

    int text_x = (int)overview_width - 260;
    if (show_controls)
    {
        const ocVirtualObject *object = sim_data.get_object_at(mouse_in_world);
        bool hover_obst = nullptr != object && object->type == ocObjectType::Obstacle;
        bool hover_ped  = nullptr != object && object->type == ocObjectType::Pedestrian;

        int text_y = 10;
        cv::putText(overview, "[C] hide controls", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, (show_barred) ? "[B] hide barred areas" : "[B] show barred areas", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, (restart_on_error) ? "[E] disable reset on error" : "[E] enable reset on error", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[1] blue button (free drive)", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[2] red button (obst. drive)", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, (rc_is_on) ? "[3] disable rc" : "[3] enable rc", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[R] reset", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, (hover_obst) ? "[O] remove obstacle" : "[O] add obstacle", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, (hover_ped) ? "[P] remove pedestrian" : "[P] add pedestrian", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[+] zoom in", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[-] zoom out", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[W] move view up", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[A] move view left", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[S] move view down", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[D] move view right", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[N] increase noise", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
        cv::putText(overview, "[M] decrease noise", cv::Point(text_x, text_y += 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
    }
    else
    {
        cv::putText(overview, "press [C] to show controls", cv::Point(text_x, 30), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 2);
    }
*/
}
