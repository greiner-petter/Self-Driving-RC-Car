#pragma once

#include "../common/ocConfigFileReader.h"
#include "../common/ocLogger.h"
#include "../common/ocVec.h"

#include <cstdint> // _t int types

#define SIM_CONFIG_FILE "../simulation.conf"

struct ocSimulationSettings
{
  float brightness;
  float noise_strength;
  Vec3  initial_pos;
  float initial_yaw;
  float initial_pitch;
  float initial_roll;
};


inline bool read_config_file(const char *file_path, ocSimulationSettings *settings, ocLogger& logger)
{
    ocConfigFileReader config;
    ocConfigReadReport result = config.read_file(file_path);
    if (ocConfigReadReport::Success != result)
    {
        if (ocConfigReadReport::File_Not_Accessible == result)
        {
            logger.log("No config file found at %s", file_path);
        }
        else
        {
            logger.error("Error while reading config file: %s (0x%x)", to_string(result), (int32_t)result);
            return false;
        }
    }

    result = config.get_float32("brightness", &settings->brightness);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'brightness': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("noise_strength", &settings->noise_strength);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'noise_strength': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("initial_pos_x", &settings->initial_pos.x);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'initial_pos_x': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("initial_pos_y", &settings->initial_pos.y);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'initial_pos_y': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("initial_pos_z", &settings->initial_pos.z);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'initial_pos_z': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("initial_yaw", &settings->initial_yaw);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'initial_yaw': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("initial_pitch", &settings->initial_pitch);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'initial_pitch': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    result = config.get_float32("initial_roll", &settings->initial_roll);
    if (ocConfigReadReport::Success != result && ocConfigReadReport::Key_Not_Found != result)
    {
        logger.warn("Error at property 'initial_roll': %s (0x%x)", to_string(result), (int)result);
        return false;
    }

    return true;
}
