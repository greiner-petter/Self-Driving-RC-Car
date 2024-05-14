#include "ocSimCar.h"

#include "../common/ocAssert.h"

#include "ocOdeSolver.h"

#include <algorithm> // std::clamp
#include <cmath> // std::isnan

#define PROPERTY_COUNT 12

static void to_array(const ocCarState& state, float *array)
{
  size_t i = 0;
  array[i++] = state.pose.pos.x;
  array[i++] = state.pose.pos.y;
  array[i++] = state.pose.pos.z;
  array[i++] = state.pose.rot_x;
  array[i++] = state.pose.rot_y;
  array[i++] = state.pose.rot_z;
  array[i++] = state.velocity.x;
  array[i++] = state.velocity.y;
  array[i++] = state.angular_velocity;
  array[i++] = state.steering_front;
  array[i++] = state.steering_rear;
  array[i++] = state.wheel_revolutions;
  oc_assert(i == PROPERTY_COUNT, i);
}

static void from_array(ocCarState& state, const float *array)
{
  size_t i = 0;
  state.pose.pos.x = array[i++];
  state.pose.pos.y = array[i++];
  state.pose.pos.z = array[i++];
  state.pose.rot_x = array[i++];
  state.pose.rot_y = array[i++];
  state.pose.rot_z = array[i++];
  state.velocity.x = array[i++];
  state.velocity.y = array[i++];
  state.angular_velocity = array[i++];
  state.steering_front = array[i++];
  state.steering_rear = array[i++];
  state.wheel_revolutions = array[i++];
  oc_assert(i == PROPERTY_COUNT, i);
}

ocCarState simulate_car_dyn(const ocCarState& state, const ocCarAction& action, float duration, float step_size)
{
  oc_assert(0.0f < duration, duration);
  oc_assert(0.0f < step_size, step_size);

  ocCarState result = state;

  ocRungeKuttaSolver solver;
  init_runge_kutta_3_8th(&solver);

  float x = 0.0f;
  float y0[PROPERTY_COUNT];
  float y1[PROPERTY_COUNT];

  ocSimCarDydx dydx;
  dydx.action = action;
  dydx.properties = state.properties;

  while (x < duration)
  {
    to_array(result, y0);

    run_runge_kutta_solver(
      &solver,
      PROPERTY_COUNT,
      &dydx,
      x,
      y0,
      x + step_size,
      y1);

    from_array(result, y1);
    x += step_size;
  }

  if (state.lights.indicator_left)
  {
    result.indicator_left_timer = state.indicator_left_timer + duration;
  }
  else
  {
    result.indicator_left_timer = 0;
  }
  if (state.lights.indicator_right)
  {
    result.indicator_right_timer = state.indicator_right_timer + duration;
  }
  else
  {
    result.indicator_right_timer = 0;
  }

  return result;
}

void ocSimCarDydx::operator()(size_t /*y_count*/, float /*x*/, const float* y, float* dy)
{
  ocCarState car = {};
  from_array(car, y);
  car.properties = properties;

  float dsf = 0.0f, dsr = 0.0f;
  if (car.steering_front < action.steering_front) dsf =  properties->steering_speed;
  if (action.steering_front < car.steering_front) dsf = -properties->steering_speed;
  if (car.steering_rear  < action.steering_rear ) dsr =  properties->steering_speed;
  if (action.steering_rear < car.steering_rear  ) dsr = -properties->steering_speed;

  Vec2 cx = angle_to_vector(car.pose.rot_z);
  Vec2 cy = right(cx);
  Vec2 fx = angle_to_vector(car.pose.rot_z + car.steering_front);
  Vec2 fy = right(fx);
  Vec2 rx = angle_to_vector(car.pose.rot_z + car.steering_rear);
  Vec2 ry = right(rx);

  float target_speed = action.speed;
  if (action.stop_after && ((action.speed < 0.0f && car.milage() <= action.distance) || (0.0f < action.speed && action.distance <= car.milage()))) target_speed = 0.0f;
  float motor_accel = std::clamp(
      (target_speed - car.speed()) * 10.0f,
      properties->max_deceleration,
      properties->max_acceleration);

  float length_front = properties->wheel_base - properties->center_of_mass_x;
  float length_rear  = properties->center_of_mass_x;

  float alpha_f = car.steering_front - std::atan((dot(car.velocity, cy) + car.angular_velocity * length_front) / std::abs(dot(car.velocity, cx)));
  float alpha_r = car.steering_rear  - std::atan((dot(car.velocity, cy) - car.angular_velocity * length_rear ) / std::abs(dot(car.velocity, cx)));

  if (std::isnan(alpha_f)) alpha_f = 0.0f;
  if (std::isnan(alpha_r)) alpha_r = 0.0f;

  Vec2 cornering_f = fy * properties->cornering_stiffness * 100.0f * std::sin(alpha_f);
  Vec2 cornering_r = ry * properties->cornering_stiffness * 100.0f * std::sin(alpha_r);

  Vec2 forces[10] = {
    cx * length_front, fx * motor_accel * properties->mass,
    cx * length_front, cornering_f,
    cx * -length_rear, cornering_r,
    Vec2(0.0f, 0.0f),  car.velocity * length(car.velocity) * -properties->drag_coefficient,
    Vec2(0.0f, 0.0f),  car.velocity * -properties->rolling_resistance
  };

  Vec2 forces_lin_sum = {};
  float forces_ang_sum = 0.0f;

  for (size_t i = 0; i < 6; i += 2)
  {
    forces_lin_sum += forces[i+1];
    forces_ang_sum += forces[i].x * forces[i+1].y - forces[i].y * forces[i+1].x;
  }

  size_t i = 0;
  dy[i++] = car.velocity.x;
  dy[i++] = car.velocity.y;
  dy[i++] = 0.0f; // vel z
  dy[i++] = 0.0f; // rot x
  dy[i++] = 0.0f; // rot y
  dy[i++] = car.angular_velocity;
  dy[i++] = forces_lin_sum.x / properties->mass;
  dy[i++] = forces_lin_sum.y / properties->mass;
  dy[i++] = forces_ang_sum / properties->moment_of_inertia;
  dy[i++] = dsf;
  dy[i++] = dsr;
  dy[i++] = car.speed() / properties->wheel.circumference;
}
