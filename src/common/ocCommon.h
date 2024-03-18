#pragma once

#include "ocVec.h"

#include <cstdint>
#include <iostream>
#include <string_view>

bool die();

bool parse_signed_int(std::string_view str, int64_t *target);
bool parse_unsigned_int(std::string_view str, uint64_t *target);
bool parse_float32(std::string_view str, float *target);
bool parse_float64(std::string_view str, double *target);

uint32_t byteswap(uint32_t value);

/**
 * A floating-point comparison function that uses the smallest representable
 * number at the magnitude of the given numbers.
 */
bool are_close(float a, float b, int max_ulps);
bool are_close(double a, double b, int max_ulps);

float normalize_radians(float radians);
float normalize_degrees(float degrees);

float sign(float f);
float sign_or_zero(float f);

uint32_t random_uint32();
uint32_t random_uint32(uint32_t min, uint32_t max);
uint64_t random_uint64();
uint64_t random_uint64(uint64_t min, uint64_t max);
int32_t random_int32();
int32_t random_int32(int32_t min, int32_t max);
int64_t random_int64();
int64_t random_int64(int64_t min, int64_t max);
float random_float();
float random_float(float min, float max);
float topheavy_random(float min, float max);
float normal_random(float mean, float variance);
