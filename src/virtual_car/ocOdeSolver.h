#pragma once

#include <cstddef> // size_t

enum ocSolverReturnCode
{
  ocSolverReturnCode_Ok = 0,
  ocSolverReturnCode_UnknownType = -1,
  ocSolverReturnCode_NoSolver    = -2,
  ocSolverReturnCode_NoDydx      = -3,
  ocSolverReturnCode_NoY0        = -3,
  ocSolverReturnCode_NoY1        = -4,
  ocSolverReturnCode_BadDx       = -5
};

struct Dydx
{
  virtual ~Dydx() = default;
  virtual void operator()(size_t y_count, float x, const float* y, float* dy) = 0;
};

struct ocRungeKuttaSolver
{
  size_t count;
  float  as[4 * 4];
  float  bs[4];
  float  cs[4];
};

ocSolverReturnCode init_runge_kutta_3_8th(ocRungeKuttaSolver *solver);


ocSolverReturnCode run_runge_kutta_solver(
  const ocRungeKuttaSolver *solver,
        size_t              y_count,
        Dydx               *dydx,
        float               x0,
  const float              *y0,
        float               x1,
        float              *y1);
