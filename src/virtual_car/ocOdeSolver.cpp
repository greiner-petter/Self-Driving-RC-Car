#include "ocOdeSolver.h"

#include "../common/ocArray.h"

ocSolverReturnCode init_runge_kutta_3_8th(ocRungeKuttaSolver *solver)
{
  if (!solver) return ocSolverReturnCode_NoSolver;
  memset(solver, 0, sizeof(ocRungeKuttaSolver));
  solver->count  = 4;
  solver->as[ 5] =  1.0f / 3.0f;
  solver->as[ 9] = -1.0f / 3.0f;
  solver->as[10] =  1.0f;
  solver->as[13] = -1.0f;
  solver->as[14] =  1.0f;
  solver->bs[0]  = 1.0f / 8.0f;
  solver->bs[1]  = 3.0f / 8.0f;
  solver->bs[2]  = 3.0f / 8.0f;
  solver->bs[3]  = 1.0f / 8.0f;
  solver->cs[1]  = 1.0f / 3.0f;
  solver->cs[2]  = 2.0f / 3.0f;
  solver->cs[3]  = 1.0f;
  return ocSolverReturnCode_Ok;
}

ocSolverReturnCode run_runge_kutta_solver(
  const ocRungeKuttaSolver *solver,
        size_t              y_count,
        Dydx               *dydx,
        float               x0,
  const float              *y0,
        float               x1,
        float              *y1)
{
  if (!solver)  return ocSolverReturnCode_NoSolver;
  if (!dydx)    return ocSolverReturnCode_NoDydx;
  if (!y0)      return ocSolverReturnCode_NoY0;
  if (!y1)      return ocSolverReturnCode_NoY1;
  if (x1 <= x0) return ocSolverReturnCode_BadDx;
  float dx = x1 - x0;
  ocArray<float> ks(solver->count * y_count);
  ocArray<float> ny(y_count);
  for (size_t i = 0; i < y_count; ++i) y1[i] = y0[i];
  for (size_t i = 0; i < solver->count; ++i)
  {
    for (size_t j = 0; j < y_count; ++j) ny[j] = y0[j];
    for (size_t j = 0; j < i; ++j)
    {
      float a = solver->as[i * solver->count + j];
      for (size_t k = 0; k < y_count; ++k)
      {
        ny[k] += ks[j * y_count + k] * a * dx;
      }
    }
    (*dydx)(y_count, x0 + dx * solver->cs[i], ny.all(), ks.get_space(i * y_count, y_count));
    for (size_t j = 0; j < y_count; ++j)
    {
      y1[j] += ks[i * y_count + j] * dx * solver->bs[i];
    }
  }
  return ocSolverReturnCode_Ok;
}
