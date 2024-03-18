#include "ocCar.h"

#include "ocAssert.h"

#include <algorithm> // std::clamp
#include <cmath> // std::round, std::isinf, std::cos, ...

/******************************************************************************
                               ocCarProperties
******************************************************************************/

float ocCarProperties::steps_to_cm(float steps) const
{
  return steps / (float)odo_ticks_number / odo_gear_ratio * wheel.circumference;
}

float ocCarProperties::cm_to_steps(float cm) const
{
  return cm / wheel.circumference * odo_gear_ratio * (float)odo_ticks_number;
}

int8_t ocCarProperties::front_steering_angle_to_byte(float angle) const
{
  if (angle < steering_offset_front) {
    return (int8_t)std::round(-90 * ((angle - steering_offset_front) / (min_steering_angle_front - steering_offset_front)));
  } else {
    return (int8_t)std::round( 90 * ((angle - steering_offset_front) / (max_steering_angle_front - steering_offset_front)));
  }
}
float  ocCarProperties::byte_to_front_steering_angle(int8_t byte) const
{
  if (byte < 0) {
    return (float)byte / -90.0f * (min_steering_angle_front - steering_offset_front) + steering_offset_front;
  } else {
    return (float)byte /  90.0f * (max_steering_angle_front - steering_offset_front) + steering_offset_front;
  }
}
int8_t ocCarProperties::rear_steering_angle_to_byte(float angle) const
{
  if (angle < steering_offset_rear) {
    return (int8_t)std::round(-90 * ((angle - steering_offset_rear) / (min_steering_angle_rear - steering_offset_rear)));
  } else {
    return (int8_t)std::round( 90 * ((angle - steering_offset_rear) / (max_steering_angle_rear - steering_offset_rear)));
  }
}
float  ocCarProperties::byte_to_rear_steering_angle(int8_t byte) const
{
  if (byte < 0) {
    return (float)byte / -90.0f * (min_steering_angle_rear - steering_offset_rear) + steering_offset_rear;
  } else {
    return (float)byte /  90.0f * (max_steering_angle_rear - steering_offset_rear) + steering_offset_rear;
  }
}


float ocCarProperties::front_axle_dist() const
{
  return wheel_base - center_of_mass_x;
}
float ocCarProperties::rear_axle_dist() const
{
  return center_of_mass_x;
}

Vec3 ocCarProperties::wheel_center_fl() const
{
  return Vec3{
    front_axle_dist(),
    -axle_width * 0.5f - center_of_mass_y,
    wheel.diameter * 0.5f - center_of_mass_z 
  };
}
Vec3 ocCarProperties::wheel_center_fr() const
{
  return Vec3{
    front_axle_dist(),
    axle_width * 0.5f - center_of_mass_y,
    wheel.diameter * 0.5f - center_of_mass_z 
  };
}
Vec3 ocCarProperties::wheel_center_rl() const
{
  return Vec3{
    -rear_axle_dist(),
    -axle_width * 0.5f - center_of_mass_y,
    wheel.diameter * 0.5f - center_of_mass_z 
  };
}
Vec3 ocCarProperties::wheel_center_rr() const
{
  return Vec3{
    -rear_axle_dist(),
    axle_width * 0.5f - center_of_mass_y,
    wheel.diameter * 0.5f - center_of_mass_z 
  };
}

Vec3 ocCarProperties::car_center() const
{
  return Vec3{
    (front_axle_dist() - rear_axle_dist()) * 0.5f,
    -center_of_mass_y,
    cam.pose.pos.z * 0.5f - center_of_mass_z
  };
}


/******************************************************************************
                               ocCameraProjector
******************************************************************************/

ocCameraProjector::ocCameraProjector(
    uint32_t image_width,
    uint32_t image_height,
    float sensor_offset_x,
    float sensor_offset_y,
    ocPose pose,
    float fov,
    float distortion_strength)
{
  _image_width = image_width;
  _image_height = image_height;
  _sensor_offset_x = sensor_offset_x;
  _sensor_offset_y = sensor_offset_y;
  _ego_to_world_mat = pose.get_generalize_mat();
  _world_to_ego_mat = pose.get_specialize_mat();
  _tan_half_fov = tanf(fov * 0.5f);
  _tan_half_fov_over_tan_dist = _tan_half_fov / tanf(distortion_strength);
  _distortion = distortion_strength;
}

bool ocCameraProjector::world_to_ego(float world_x, float world_y, float *img_h, float *img_v, float *img_d) const
{
  return world_to_ego(world_x, world_y, 0.0f, img_h, img_v, img_d);
}
bool ocCameraProjector::world_to_ego(float world_x, float world_y, float world_z, float *img_h, float *img_v, float *img_d) const
{
  Vec4 v(world_x, world_y, world_z, 1);

  Vec4 transformed = _world_to_ego_mat * v;

  if (transformed.x == 0.0f) return false;

  float pu = transformed.y / transformed.x;
  float pv = -transformed.z / transformed.x;
  float g = 0.0f;
  if (0.0f == _distortion)
  {
    g = 1.0f / _tan_half_fov;
  }
  else
  {
    float pl = std::sqrt(pu * pu + pv * pv);
    if (0.0f != pl)
    {
      g = std::atan(pl / _tan_half_fov_over_tan_dist) / _distortion / pl;
    }
  }

  if (img_h) *img_h = ((pu * g * (float)_image_width) + (float)_image_width ) * 0.5f + _sensor_offset_x;
  if (img_v) *img_v = ((pv * g * (float)_image_width) + (float)_image_height) * 0.5f + _sensor_offset_y;
  if (img_d) *img_d = transformed.x;
  return true;
}
Vec3 ocCameraProjector::world_to_ego(Vec2 world_xy) const
{
  Vec3 result = {};
  world_to_ego(world_xy.x, world_xy.y, &result.x, &result.y, &result.z);
  return result;
}
Vec3 ocCameraProjector::world_to_ego(Vec3 world_xyz) const
{
  Vec3 result = {};
  world_to_ego(world_xyz.x, world_xyz.y, world_xyz.z, &result.x, &result.y, &result.z);
  return result;
}

bool ocCameraProjector::can_see(Vec3 world_xyz) const
{
  float img_h, img_v, img_d;
  return world_to_ego(world_xyz.x, world_xyz.y, world_xyz.z, &img_h, &img_v, &img_d) &&
    0.0f <= img_h && img_h < (float)_image_width &&
    0.0f <= img_v && img_v < (float)_image_height &&
    0.0f <= img_d;
}

void ocCameraProjector::ego_to_world(float img_h, float img_v, float *world_x, float *world_y, float *world_z) const
{
  float oh = (2.0f * (img_h - _sensor_offset_x) - (float)_image_width ) / (float)_image_width;
  float ov = (2.0f * (img_v - _sensor_offset_y) - (float)_image_height) / (float)_image_width;
  float pl = std::sqrt(oh * oh + ov * ov);
  float g = 0.0f;
  if (0.0f == _distortion)
  {
    g = _tan_half_fov;
  }
  else if (0.0f != pl)
  {
    g = std::tan(pl * _distortion) * _tan_half_fov_over_tan_dist / pl;
  }

  Vec4 v(1, oh * g, -ov * g, 0);

  Vec3 normal = normalize((_ego_to_world_mat * v).xyz());
  *world_x = normal.x;
  *world_y = normal.y;
  *world_z = normal.z;
}
Vec3 ocCameraProjector::ego_to_world(Vec2 img_hv) const
{
  Vec3 result;
  ego_to_world(img_hv.x, img_hv.y, &result.x, &result.y, &result.z);
  return result;
}

float ocCameraProjector::distance_from_ego_points(Vec2 ego1, Vec2 ego2, float distance) const
{
  Vec3 normal1 = ego_to_world(ego1);
  Vec3 normal2 = ego_to_world(ego2);
  Vec3 a = normal1 / normal1.x;
  Vec3 b = normal2 / normal2.x;
  float c = length(a - b);
  return distance / c;
}

/******************************************************************************
                               ocCarState
******************************************************************************/

float ocCarState::speed() const
{
  //return length(velocity);
  auto sf = angle_to_vector(pose.heading + steering_front);
  auto sr = angle_to_vector(pose.heading + steering_rear);
  return (dot(velocity, sf) + dot(velocity, sr)) * 0.5f;
}

int32_t ocCarState::odo_steps() const
{
  return (int32_t)properties->cm_to_steps(wheel_revolutions * properties->wheel.circumference);
}

float ocCarState::milage() const
{
  return wheel_revolutions * properties->wheel.circumference;
}

ocCameraProjector ocCarState::make_projector() const
{
  ocCarProperties::ocCamProperties *cp = &properties->cam;
  ocCameraProjector result(
    cp->image_width, cp->image_height,
    cp->sensor_offset_y, cp->sensor_offset_y,
    ocPose::compose(pose, properties->cam.pose),
    cp->fov,
    cp->distortion);
  return result;
}

void ocCarState::target_to_pivot(
  float target_x,
  float target_y,
  float target_heading,
  float *pivot_x,
  float *pivot_y) const
{
  target_heading = normalize_radians(target_heading);
  if (pose.heading == target_heading)
  {
    *pivot_x = INFINITY;
    *pivot_y = INFINITY;
  }
  else if (pose.pos.x == target_x && pose.pos.y == target_y)
  {
    *pivot_x = pose.pos.x;
    *pivot_y = pose.pos.y;
  }
  else
  {
    float dx = target_x - pose.pos.x;
    float dy = target_y - pose.pos.y;
    float dh = target_heading - pose.heading;
    float d = std::sqrt(dx * dx + dy * dy);
    float e = d * 0.5f * std::cos(dh * 0.5f) / std::sin(dh * 0.5f);
    float rx = -dy / d;
    float ry =  dx / d;
    *pivot_x = (pose.pos.x + target_x) * 0.5f + rx * e;
    *pivot_y = (pose.pos.y + target_y) * 0.5f + ry * e;
  }
}

void ocCarState::target_to_pivot(
  float target_x,
  float target_y,
  float *pivot_x,
  float *pivot_y) const
{
  if (pose.pos.x == target_x && pose.pos.y == target_y)
  {
    *pivot_x = pose.pos.x;
    *pivot_y = pose.pos.y;
  }
  else
  {
    float ch = std::cos(pose.heading);
    float sh = std::sin(pose.heading);
    float dx = ch * (target_x - pose.pos.x) + sh * (target_y - pose.pos.y);
    float dy = ch * (target_y - pose.pos.y) - sh * (target_x - pose.pos.x);
    float r = (dx * dx + dy * dy) / (2.0f * dy);
    *pivot_x = sh * r;
    *pivot_y = ch * r;
  }
}

void ocCarState::clamp_pivot(
  float pivot_x, float pivot_y,
  float *clamped_x, float *clamped_y) const
{
  const float PI = 3.14159265358979f;
  if (std::isinf(pivot_x) || std::isinf(pivot_y))
  {
    *clamped_x = pivot_x;
    *clamped_y = pivot_y;
  }
  else
  {
    float car_normal_x = std::cos(pose.heading);
    float car_normal_y = std::sin(pose.heading);
    float dx = pivot_x - pose.pos.x;
    float dy = pivot_y - pose.pos.y;
    float px = car_normal_x * dx + car_normal_y * dy;
    float py = car_normal_y * dx - car_normal_x * dy;
    if (0 <= py)
    {
      float steering_front_normal_x = std::cos(properties->max_steering_angle_front);
      float steering_front_normal_y = std::sin(properties->max_steering_angle_front);
      float steering_rear_normal_x = std::cos(properties->min_steering_angle_rear);
      float steering_rear_normal_y = std::sin(properties->min_steering_angle_rear);
      py = std::max(properties->wheel_base * steering_front_normal_y * -steering_rear_normal_y / std::sin(PI - properties->max_steering_angle_front + properties->min_steering_angle_rear), py);
      float dist1 = std::max(0.0f, px * steering_rear_normal_x + py * steering_rear_normal_y);
      float dist2 = std::max(0.0f, (px - properties->wheel_base) * -steering_front_normal_x + py * -steering_front_normal_y);
      px += -steering_rear_normal_x * dist1 + steering_front_normal_x * dist2;
      py += -steering_rear_normal_y * dist1 + steering_front_normal_y * dist2;
    }
    else
    {
      float steering_front_normal_x = std::cos(properties->min_steering_angle_front);
      float steering_front_normal_y = std::sin(properties->min_steering_angle_front);
      float steering_rear_normal_x = std::cos(properties->max_steering_angle_rear);
      float steering_rear_normal_y = std::sin(properties->max_steering_angle_rear);
      py = std::min(-properties->wheel_base * -steering_front_normal_y * steering_rear_normal_y / std::sin(PI + properties->min_steering_angle_front - properties->max_steering_angle_rear), py);
      float dist1 = std::max(0.0f, px * steering_rear_normal_x + py * steering_rear_normal_y);
      float dist2 = std::max(0.0f, (px - properties->wheel_base) * -steering_front_normal_x + py * -steering_front_normal_y);
      px += -steering_rear_normal_x * dist1 + steering_front_normal_x * dist2;
      py += -steering_rear_normal_y * dist1 + steering_front_normal_y * dist2;
    }
    *clamped_x = pose.pos.x + car_normal_x * px + car_normal_y * py;
    *clamped_y = pose.pos.y + car_normal_y * px - car_normal_x * py;
  }
}

bool ocCarState::pivot_to_steering(
  float pivot_x,
  float pivot_y,
  float *steering_front,
  float *steering_rear) const
{
  if (std::isinf(pivot_x) || std::isinf(pivot_y))
  {
    *steering_front = 0.0f;
    *steering_rear  = 0.0f;
  }
  else
  {
    float car_normal_x = std::cos(pose.heading);
    float car_normal_y = std::sin(pose.heading);
    float drx = pivot_x - pose.pos.x;
    float dry = pivot_y - pose.pos.y;
    float prx = car_normal_x * drx + car_normal_y * dry;
    float pry = car_normal_y * drx - car_normal_x * dry;
    float rr = std::sqrt(prx * prx + pry * pry);
    *steering_rear = std::asin(prx / rr) * (pry < 0.0f ? -1.0f : 1.0f);
    float dfx = pivot_x - (pose.pos.x + car_normal_x * properties->wheel_base);
    float dfy = pivot_y - (pose.pos.y + car_normal_y * properties->wheel_base);
    float pfx = car_normal_x * dfx + car_normal_y * dfy;
    float pfy = car_normal_y * dfx - car_normal_x * dfy;
    float rf = std::sqrt(pfx * pfx + pfy * pfy);
    *steering_front = std::asin(pfx / rf) * (pfy < 0.0f ? -1.0f : 1.0f);
  }
  return properties->min_steering_angle_front <= *steering_front &&
         *steering_front <= properties->max_steering_angle_front &&
         properties->min_steering_angle_rear <= *steering_rear &&
         *steering_rear <= properties->max_steering_angle_rear;
}

float ocCarState::pivot_to_radius(float pivot_x, float pivot_y) const
{
  if (std::isinf(pivot_x) || std::isinf(pivot_y)) return INFINITY;
  float dx = pivot_x - pose.pos.x;
  float dy = pivot_y - pose.pos.y;
  return std::sqrt(dx * dx + dy * dy);
}

void ocCarState::steering_to_pivot(
  float steering_front,
  float steering_rear,
  float *pivot_x,
  float *pivot_y) const
{
  if (steering_front == steering_rear)
  {
    *pivot_x = INFINITY;
    *pivot_y = INFINITY;
  }
  else
  {
    float radius = steering_to_radius(steering_front, steering_rear);
    float cos_h = std::cos(pose.heading);
    float sin_h = std::sin(pose.heading);
    float cos_r = std::cos(steering_rear);
    float sin_r = std::sin(steering_rear);
    *pivot_x = pose.pos.x - (cos_h * sin_r + sin_h * cos_r) * radius;
    *pivot_y = pose.pos.y - (sin_h * sin_r - cos_h * cos_r) * radius;
  }
}

bool ocCarState::target_to_steering(
  float target_x,
  float target_y,
  float target_heading,
  float *steering_front,
  float *steering_rear) const
{
  float pivot_x, pivot_y;
  target_to_pivot(target_x, target_y, target_heading, &pivot_x, &pivot_y);
  clamp_pivot(pivot_x, pivot_y, &pivot_x, &pivot_y);
  return pivot_to_steering(pivot_x, pivot_y, steering_front, steering_rear);
}


bool ocCarState::target_to_steering(
  float target_x,
  float target_y,
  float *steering_front) const
{
  float pivot_x, pivot_y;
  target_to_pivot(target_x, target_y, &pivot_x, &pivot_y);
  float steering_rear; // should always be set to 0 by pivot_to_steering.
  return pivot_to_steering(pivot_x, pivot_y, steering_front, &steering_rear);
}

float ocCarState::target_to_distance(
  float target_x, float target_y,
  float target_heading) const
{
  float pivot_x, pivot_y;
  target_to_pivot(target_x, target_y, target_heading, &pivot_x, &pivot_y);
  float dx = target_x - pose.pos.x;
  float dy = target_y - pose.pos.y;
  float dl = std::sqrt(dx * dx + dy * dy);
  float radius = pivot_to_radius(pivot_x, pivot_y);
  if (0.0f == radius || std::isinf(radius)) return dl;
  float cos_a = 1.0f - (dl * dl) / (2 * radius * radius);
  float dist = std::acos(cos_a) * radius;
  if (0.0f == dist) return dl;
  return dist;
}

float ocCarState::target_to_distance(
  float target_x, float target_y) const
{
  float pivot_x, pivot_y;
  target_to_pivot(target_x, target_y, &pivot_x, &pivot_y);
  float dx = target_x - pose.pos.x;
  float dy = target_y - pose.pos.y;
  float dl = std::sqrt(dx * dx + dy * dy);
  float radius = pivot_to_radius(pivot_x, pivot_y);
  if (0.0f == radius || std::isinf(radius)) return dl;
  float cos_a = 1.0f - (dl * dl) / (2 * radius * radius);
  float dist = std::acos(cos_a) * radius;
  if (0.0f == dist) return dl;
  return dist;
}

float ocCarState::steering_to_radius(float steering_front, float steering_rear) const
{
  return properties->wheel_base * std::cos(steering_front) / std::sin(steering_front - steering_rear);
}

float ocCarState::speeds_to_distance(float speed1, float speed2, float duration) const
{
  float celeration = (speed1 < speed2) ? properties->max_acceleration : properties->max_deceleration;
  float time = (speed2 - speed1) / celeration;
  if (duration <= time)
  {
    return (speed1 + celeration * duration * 0.5f) * duration;
  }
  float dist1 = (speed1 + celeration * time * 0.5f) * time;
  return dist1 + (speed2 * (duration - time));
}

float ocCarState::get_braking_distance() const
{
  float speed = this->speed();
  if (speed < 0.0f)
  {
    return (speed * speed) / (2.0f * properties->max_acceleration);
  }
  else
  {
    return (speed * speed) / (2.0f * -properties->max_deceleration);
  }
}

static ocCarState simulate_car_step(const ocCarState& state, const ocCarAction& action, float duration)
{
  oc_assert(0.0f < duration, duration);

  ocCarState result = state;
  float target_speed = action.speed;
  float target_angle_front = std::clamp(action.steering_front, state.properties->min_steering_angle_front, state.properties->max_steering_angle_front);
  float target_angle_rear  = std::clamp(action.steering_rear,  state.properties->min_steering_angle_rear,  state.properties->max_steering_angle_rear);
  if (action.stop_after && ((action.speed < 0.0f && state.milage() <= action.distance) || (0.0f < action.speed && action.distance <= state.milage())))
  {
    target_speed = 0.0f;
  }
  float speed = state.velocity.x;
  float motor_accel;
  float front_steer_accel;
  float rear_steer_accel;

  // TODO: create a more accurate speed control simulation
  if (speed < target_speed)
    motor_accel = state.properties->max_acceleration;
  else
    motor_accel = state.properties->max_deceleration;

  if (state.steering_front < target_angle_front)
    front_steer_accel = state.properties->steering_speed;
  else
    front_steer_accel = -state.properties->steering_speed;

  if (state.steering_rear < target_angle_rear)
    rear_steer_accel = state.properties->steering_speed;
  else
    rear_steer_accel = -state.properties->steering_speed;

  float distance = state.speeds_to_distance(speed, speed + motor_accel * duration, duration);
  result.velocity = {speed + motor_accel * duration, 0.0f};
  result.steering_front = state.steering_front + front_steer_accel * duration;
  result.steering_rear  = state.steering_rear  + rear_steer_accel  * duration;

  if (state.steering_front == state.steering_rear)
  {
    float angle = state.pose.heading + state.steering_front;
    result.pose.pos.x = state.pose.pos.x + std::cos(angle) * distance;
    result.pose.pos.y = state.pose.pos.y + std::sin(angle) * distance;
  }
  else
  {
    float radius = state.steering_to_radius(state.steering_front, state.steering_rear);
    float angle = distance / radius;
    float pivot_x, pivot_y;
    state.steering_to_pivot(state.steering_front, state.steering_rear, &pivot_x, &pivot_y);
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    result.pose.pos.x = (state.pose.pos.x - pivot_x) * cos_a - (state.pose.pos.y - pivot_y) * sin_a + pivot_x;
    result.pose.pos.y = (state.pose.pos.x - pivot_x) * sin_a + (state.pose.pos.y - pivot_y) * cos_a + pivot_y;
    result.pose.pos.z = state.pose.pos.z;
    result.pose.heading = normalize_radians(state.pose.heading + angle);
  }

  result.wheel_revolutions += distance / state.properties->wheel.circumference;

  if (state.lights.indicator_left)
  {
    result.indicator_left_timer = state.indicator_left_timer + duration;
  }
  else
  {
    result.indicator_left_timer = 0.0f;
  }
  if (state.lights.indicator_right)
  {
    result.indicator_right_timer = state.indicator_right_timer + duration;
  }
  else
  {
    result.indicator_right_timer = 0.0f;
  }

  return result;
}

ocCarState simulate_car(const ocCarState& state, const ocCarAction& action, float duration, float step_size)
{
  ocCarState result = state;
  while (step_size < duration)
  {
    result = simulate_car_step(result, action, step_size);
    duration -= step_size;
  }
  if (0.0f < duration)
  {
    result = simulate_car_step(result, action, duration);
  }
  return result;
}
