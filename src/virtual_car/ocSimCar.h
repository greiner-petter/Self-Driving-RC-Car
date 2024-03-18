#pragma once

#include "../common/ocCar.h"
#include "../common/ocMat.h"
#include "../common/ocPose.h"
#include "../common/ocVec.h"

#include "ocOdeSolver.h"

#include <cstdint> // uint32_t, ...

struct ocSimCarDydx : Dydx
{
  ocCarAction action;
  ocCarProperties *properties;
  void operator()(size_t y_count, float x, const float* y, float* dy);
};

ocCarState simulate_car_dyn(const ocCarState& state, const ocCarAction& action, float duration, float step_size);
