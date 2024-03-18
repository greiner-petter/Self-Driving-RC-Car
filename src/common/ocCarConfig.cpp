#include "ocCarConfig.h"
#include "ocConfigFileReader.h"

bool read_config_file(const char *file, ocCarProperties& target, ocLogger& logger)
{
    ocConfigFileReader config;
    ocConfigReadReport result = config.read_file(file);
    if (ocConfigReadReport::Success != result)
    {
        if (ocConfigReadReport::File_Not_Accessible == result)
        {
            logger.log("No config file found at %s", file);
        }
        else
        {
            logger.error("Error while reading config file: %s (0x%x)", to_string(result), (int32_t)result);
            return false;
        }
    }

    bool success = true;

    auto read_f32 = [&](std::string_view key, float &target){
        auto result = config.get_float32(key, &target);
        if (ocConfigReadReport::Success != result)
        {
            logger.warn("Error at property '%s': %s (0x%x)", key.data(), to_string(result), (int)result);
            success = false;
        }
    };

    auto read_u32 = [&](std::string_view key, uint32_t &target){
        auto result = config.get_uint32(key, &target);
        if (ocConfigReadReport::Success != result)
        {
            logger.warn("Error at property '%s': %s (0x%x)", key.data(), to_string(result), (int)result);
            success = false;
        }
    };

    read_f32("wheel_base",               target.wheel_base);
    read_f32("axle_width",               target.axle_width);
    read_f32("wheel.width",              target.wheel.width);
    read_f32("wheel.diameter",           target.wheel.diameter);
    read_f32("wheel.circumference",      target.wheel.circumference);
    read_f32("wheel.offset",             target.wheel.offset);
    read_f32("cam.pose.pos.x",           target.cam.pose.pos.x);
    read_f32("cam.pose.pos.y",           target.cam.pose.pos.y);
    read_f32("cam.pose.pos.z",           target.cam.pose.pos.z);
    read_f32("cam.pose.yaw",             target.cam.pose.yaw);
    read_f32("cam.pose.pitch",           target.cam.pose.pitch);
    read_f32("cam.pose.roll",            target.cam.pose.roll);
    read_f32("cam.fov",                  target.cam.fov);
    read_f32("cam.sensor_offset_x",      target.cam.sensor_offset_x);
    read_f32("cam.sensor_offset_y",      target.cam.sensor_offset_y);
    read_f32("cam.distortion",           target.cam.distortion);
    read_u32("cam.image_width",          target.cam.image_width);
    read_u32("cam.image_height",         target.cam.image_height);

    const char *pixel_format_names[] = {
        "gray",
        "bgr",
        "bgra"
    };
    ocPixelFormat pixel_formats[] = {
        ocPixelFormat::Gray_U8,
        ocPixelFormat::Bgr_U8,
        ocPixelFormat::Bgra_U8
    };
    size_t format_count = sizeof(pixel_format_names) / sizeof(pixel_format_names[0]);

    size_t pfi;
    result = config.get_index("cam.pixel_format", pixel_format_names, format_count, &pfi);
    if (ocConfigReadReport::Success != result)
    {
        logger.warn("Error at property 'cam.pixel_format': %s (0x%x)", to_string(result), (int)result);
        success = false;
    }
    else
    {
        target.cam.pixel_format = pixel_formats[pfi];
    }

    read_f32("mass",                     target.mass);
    read_f32("moment_of_inertia",        target.moment_of_inertia);
    read_f32("cornering_stiffness",      target.cornering_stiffness);
    read_f32("drag_coefficient",         target.drag_coefficient);
    read_f32("rolling_resistance",       target.rolling_resistance);
    read_f32("center_of_mass_x",         target.center_of_mass_x);
    read_f32("center_of_mass_y",         target.center_of_mass_y);
    read_f32("center_of_mass_z",         target.center_of_mass_z);
    read_f32("min_steering_angle_front", target.min_steering_angle_front);
    read_f32("max_steering_angle_front", target.max_steering_angle_front);
    read_f32("min_steering_angle_rear",  target.min_steering_angle_rear);
    read_f32("max_steering_angle_rear",  target.max_steering_angle_rear);
    read_f32("steering_offset_front",    target.steering_offset_front);
    read_f32("steering_offset_rear",     target.steering_offset_rear);
    read_f32("steering_speed",           target.steering_speed);
    read_f32("max_acceleration",         target.max_acceleration);
    read_f32("max_deceleration",         target.max_deceleration);
    read_f32("max_forward_speed",        target.max_forward_speed);
    read_f32("max_backward_speed",       target.max_backward_speed);
    read_u32("odo_ticks_number",         target.odo_ticks_number);
    read_f32("odo_gear_ratio",           target.odo_gear_ratio);
    read_f32("motor_gear_ratio",         target.motor_gear_ratio);

    return success;
}