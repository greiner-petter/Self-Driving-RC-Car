#pragma once

#include "ocCommon.h"
#include "ocImageOps.h" // ocPixelFormat
#include "ocMat.h"
#include "ocPose.h"
#include "ocVec.h"

#include <cstdint> // uint32_t, ...

// Unless stated otherwise, these are the units used here:
// distance     : cm
// speed        : cm/s
// acceleration : cm/s^2
// time         : s
// angles       : radians
// angular speed: radians/s
// mass         : kg

// The coordinate system relative to the car is the following:
// x points out of the front of the car
// y points out of the right of the car
// z points up
// the origin is the center of the rear axle in the x and y direction and the
// surface of the floor in the z direction.

struct ocCarProperties final
{
  // distance between the front and rear axle
  float wheel_base;

  // distance from the left to the right wheel steering pivots
  float axle_width;

  struct ocWheelProperties
  {
    // if the wheel is a cylinder, this value would be the height
    float width;

    // should be obvious
    float diameter;

    float circumference;

    // distance from the steering pivot to the center of the wheel
    float offset;
  } wheel;

  struct ocCamProperties
  {
    // position and orientation of the camera relative to the cars origin
    ocPose pose;

    // horizontal field of view in radians
    float fov;

    // offset of the sensor inside the camera
    float sensor_offset_x;
    float sensor_offset_y;

    float distortion;

    uint32_t image_width;
    uint32_t image_height;
    ocPixelFormat pixel_format;
  } cam;

  // full mass of the car with battery while driving
  float mass;

  float moment_of_inertia;

  float cornering_stiffness;

  float drag_coefficient;

  float rolling_resistance;

  // center of mass of the vehicle. X points forward along the vehicle,
  // Y points right, Z points upwards.
  float center_of_mass_x;
  float center_of_mass_y;
  float center_of_mass_z;

  // Maximum angle the wheels can be steered towards.
  // It is assumed that steering is symmetrical and the same for both axles.
  float min_steering_angle_front;
  float max_steering_angle_front;
  float min_steering_angle_rear;
  float max_steering_angle_rear;

  // Angle that the axles are offset from being straight
  float steering_offset_front;
  float steering_offset_rear;

  // Maximum angular speed the wheel heading can be changed at.
  float steering_speed;

  // maximum change in speed
  float max_acceleration; // should be positive
  float max_deceleration; // should be negative

  // maximum speed
  float max_forward_speed;
  float max_backward_speed;

  // number of markings on the odometry disc
  uint32_t odo_ticks_number;

  // multiply this with the wheel RPM to get the odometry RPM
  float odo_gear_ratio;

  // multiply this with the wheel RPM to get the motor RPM
  float motor_gear_ratio;

  float steps_to_cm(float steps) const;
  float cm_to_steps(float cm) const;

  int8_t front_steering_angle_to_byte(float angle) const;
  float  byte_to_front_steering_angle(int8_t byte) const;
  int8_t rear_steering_angle_to_byte(float angle) const;
  float  byte_to_rear_steering_angle(int8_t byte) const;

  float front_axle_dist() const;
  float rear_axle_dist() const;

  Vec3 wheel_center_fl() const;
  Vec3 wheel_center_fr() const;
  Vec3 wheel_center_rl() const;
  Vec3 wheel_center_rr() const;

  Vec3 car_center() const;
};

struct ocCarLights
{
    bool indicator_left;
    bool indicator_right;
    uint8_t headlights;
};

class ocCameraProjector
{
private:
  uint32_t _image_width;
  uint32_t _image_height;
  float    _sensor_offset_x;
  float    _sensor_offset_y;
  Mat4     _ego_to_world_mat;
  Mat4     _world_to_ego_mat;
  float    _tan_half_fov;
  float    _tan_half_fov_over_tan_dist;
  float    _distortion;

public:
  ocCameraProjector(
    uint32_t image_width, uint32_t image_height,
    float sensor_offset_x, float sensor_offset_y,
    ocPose pose,
    float fov,
    float distortion_strength);


  /**
   * This method tries to simulate a camera and lens. For every point in the world this method
   * can calculate where in the camera image that point should appear. Points are given in cm and
   * are returned in (sub-) pixels.
   * If no world_z is given, a value of 0 is assumed, which is the height of the floor.
   */
  bool world_to_ego(float world_x, float world_y, float *img_h, float *img_v, float *img_d) const;
  bool world_to_ego(float world_x, float world_y, float world_z, float *img_h, float *img_v, float *img_d) const;
  Vec3 world_to_ego(Vec2 world_xy) const;
  Vec3 world_to_ego(Vec3 world_xyz) const;

  /**
   * Takes a world coordinate and checks if its projection in the camera view lands in the image bounds or not.
   */
  bool can_see(Vec3 world_xyz) const;

  /**
    * This method calculates a direction in the form of a three-dimensional normal vector for any pixel in a camera image.
    * This direction plus the cameras location can be used to turn camera-coordinates into world-coordinates.
    * If the projector is given the identity matrix as its transformation, positive X forward and positive Y is up.
    */
  void ego_to_world(float img_h, float img_v, float *world_x, float *world_y, float *world_z) const;
  Vec3 ego_to_world(Vec2 img_hv) const;

  /**
   * This method takes two pixel positions in the camera view and a distance value how far apart the two points are in
   * the real world. From that the method calculates, how far along the x-axis of the var a plane has to be moved for
   * the points to have the given distance when they lay on that plane. This is useful for example for street signs
   * where their physical dimensions and location in the camera is known and the "depth" in the image needs to be known.
   */
  float distance_from_ego_points(Vec2 ego1, Vec2 ego2, float distance) const;
};

struct ocCarAction final
{
  // values that the car is supposed to reach
  float speed;          // cm/s
  float steering_front; // radians
  float steering_rear;  // radians
  float distance;       // cm  !!Absolute distance!!
  bool  stop_after;
};

struct ocCarState final
{
  ocCarProperties *properties;

  // position and orientation of the car relative to the worlds origin
  ocPose pose;

  // physical speed of the car
  Vec2  velocity;
  float angular_velocity;

  // Rotation of the two axles. Positive angles mean, the wheels are turned
  // towards the right, negative angles are towards the left.
  // Note that turning the rear wheels left will drive the car right, so you
  // usually want these two values to have opposite signs.
  float steering_front;
  float steering_rear;

  // for the odometry. TODO: have 4 separate values
  float wheel_revolutions;

  // the lights that are currently enabled
  ocCarLights lights;

  // Timer in seconds for flashing das blinking lights
  float indicator_left_timer;
  float indicator_right_timer;

  // Boolean flag indicating the state of the remote controller override.
  bool rc_is_active;

  float speed() const;

  int32_t odo_steps() const;
  float milage() const; // returns cm, not miles though

  ocCameraProjector make_projector() const;

  void target_to_pivot(
    float target_x, float target_y,
    float target_heading,
    float *pivot_x, float *pivot_y) const;

  void target_to_pivot(
    float target_x, float target_y,
    float *pivot_x, float *pivot_y) const;

  void clamp_pivot(
    float pivot_x, float pivot_y,
    float *clamped_x, float *clamped_y) const;

  bool pivot_to_steering(
    float pivot_x, float pivot_y,
    float *steering_front, float *steering_rear) const;

  float pivot_to_radius(float pivot_x, float pivot_y) const;

  void steering_to_pivot(
    float steering_front, float steering_rear,
    float *pivot_x, float *pivot_y) const;

  bool target_to_steering(
    float target_x, float target_y,
    float target_heading,
    float *steering_front, float *steering_rear) const;

  bool target_to_steering(
    float target_x, float target_y,
    float *steering_front) const;

  float target_to_distance(
    float target_x, float target_y,
    float target_heading) const;

  float target_to_distance(
    float target_x, float target_y) const;

  float steering_to_radius(float steering_front, float steering_rear) const;

  float speeds_to_distance(float speed1, float speed2, float duration) const;

  float get_braking_distance() const;
};

ocCarState simulate_car(const ocCarState& state, const ocCarAction& action, float duration, float step_size = 0.001f);
