#pragma once

#include "ocCar.h"
#include "ocLogger.h"

bool read_config_file(const char *file, ocCarProperties& target, ocLogger& logger);
