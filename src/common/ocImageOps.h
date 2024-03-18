#pragma once

#include <cmath> // roundf
#include <cstdint> // uint8_t
#include <cstddef> // size_t
#include <cstring> // memset

#ifdef TIME
#include "ocTime.h"
#endif

#include "ocAssert.h"

enum class ocPixelFormat: uint8_t
{
    None     = 0,
    Gray_U8  = 1,
    Bgr_U8   = 2,
    Bgra_U8  = 3,
    Rgb_F32  = 4
};

const char *to_string(ocPixelFormat pixel_format);
uint8_t bytes_per_pixel(ocPixelFormat pixel_format);

void convert_gray_u8_to_gray_u8 (const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
void convert_gray_u8_to_bgr_u8  (const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
void convert_gray_u8_to_rgb_f32 (const uint8_t *src_data, size_t src_width, size_t src_height, float    *dst_data, size_t dst_width, size_t dst_height);

void convert_bgr_u8_to_gray_u8 (const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
void convert_bgr_u8_to_bgr_u8  (const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
void convert_bgr_u8_to_rgb_f32 (const uint8_t *src_data, size_t src_width, size_t src_height, float    *dst_data, size_t dst_width, size_t dst_height);

void convert_bgra_u8_to_gray_u8 (const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
void convert_bgra_u8_to_bgr_u8  (const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
void convert_bgra_u8_to_rgb_f32 (const uint8_t *src_data, size_t src_width, size_t src_height, float    *dst_data, size_t dst_width, size_t dst_height);

bool convert_to_gray_u8 (ocPixelFormat src_format, const void *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
bool convert_to_bgr_u8  (ocPixelFormat src_format, const void *src_data, size_t src_width, size_t src_height, uint8_t  *dst_data, size_t dst_width, size_t dst_height);
bool convert_to_rgb_f32 (ocPixelFormat src_format, const void *src_data, size_t src_width, size_t src_height, float    *dst_data, size_t dst_width, size_t dst_height);


namespace PixelFormat
{
  struct GrayU8
  {
    uint8_t value;

    GrayU8 operator+(GrayU8 rhs) const
    {
      return GrayU8 { (uint8_t)(value + rhs.value) };
    }
    void operator+=(GrayU8 rhs)
    {
      value = (uint8_t)(value + rhs.value);
    }
    GrayU8 operator*(float rhs) const
    {
      return GrayU8 { (uint8_t)roundf((float)value * rhs) };
    }
  };
  struct BgrU8
  {
    uint8_t b;
    uint8_t g;
    uint8_t r;

    BgrU8 operator+(BgrU8 rhs) const
    {
      BgrU8 result;
      result.b = (uint8_t)(b + rhs.b);
      result.g = (uint8_t)(g + rhs.g);
      result.r = (uint8_t)(r + rhs.r);
      return result;
    }
    void operator+=(BgrU8 rhs)
    {
      b = (uint8_t)(b + rhs.b);
      g = (uint8_t)(g + rhs.g);
      r = (uint8_t)(r + rhs.r);
    }
    BgrU8 operator*(float rhs) const
    {
      BgrU8 result;
      result.b = (uint8_t)roundf((float)b * rhs);
      result.g = (uint8_t)roundf((float)g * rhs);
      result.r = (uint8_t)roundf((float)r * rhs);
      return result;
    }
  };
  struct BgrxU8
  {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t x;

    BgrxU8 operator+(BgrxU8 rhs) const
    {
      BgrxU8 result;
      result.b = (uint8_t)(b + rhs.b);
      result.g = (uint8_t)(g + rhs.g);
      result.r = (uint8_t)(r + rhs.r);
      result.x = 0;
      return result;
    }
    void operator+=(BgrxU8 rhs)
    {
      b = (uint8_t)(b + rhs.b);
      g = (uint8_t)(g + rhs.g);
      r = (uint8_t)(r + rhs.r);
    }
    BgrxU8 operator*(float rhs) const
    {
      BgrxU8 result;
      result.b = (uint8_t)roundf((float)b * rhs);
      result.g = (uint8_t)roundf((float)g * rhs);
      result.r = (uint8_t)roundf((float)r * rhs);
      result.x = 0;
     return result;
    }
  };
  struct RgbF32
  {
    float r;
    float g;
    float b;

    RgbF32 operator+(RgbF32 rhs) const
    {
      RgbF32 result;
      result.r = r + rhs.r;
      result.g = g + rhs.g;
      result.b = b + rhs.b;
      return result;
    }
    void operator+=(RgbF32 rhs)
    {
      r += rhs.r;
      g += rhs.g;
      b += rhs.b;
    }
    RgbF32 operator*(float rhs) const
    {
      RgbF32 result;
      result.r = r * rhs;
      result.g = g * rhs;
      result.b = b * rhs;
      return result;
    }
  };

  template<typename SrcFormat, typename DstFormat>
  DstFormat convert(SrcFormat pixel);

  template<> GrayU8  convert<GrayU8,  GrayU8 >(GrayU8  pixel);
  template<> BgrU8   convert<GrayU8,  BgrU8  >(GrayU8  pixel);
  template<> RgbF32  convert<GrayU8,  RgbF32 >(GrayU8  pixel);

  template<> GrayU8  convert<BgrU8,   GrayU8 >(BgrU8   pixel);
  template<> BgrU8   convert<BgrU8,   BgrU8  >(BgrU8   pixel);
  template<> RgbF32  convert<BgrU8,   RgbF32 >(BgrU8   pixel);

  template<> GrayU8  convert<BgrxU8,  GrayU8 >(BgrxU8  pixel);
  template<> BgrU8   convert<BgrxU8,  BgrU8  >(BgrxU8  pixel);
  template<> RgbF32  convert<BgrxU8,  RgbF32 >(BgrxU8  pixel);
}

template<typename SrcFormat, typename DstFormat>
void convert_and_downscale(
  const SrcFormat *src_data,
  size_t src_width,
  size_t src_height,
  DstFormat *dst_data,
  size_t dst_width,
  size_t dst_height)
{
  oc_assert(nullptr != src_data);
  oc_assert(nullptr != dst_data);
  oc_assert(0 != dst_width);
  oc_assert(0 != dst_height);
  oc_assert(dst_width <= src_width, dst_width, src_width);
  oc_assert(dst_height <= src_height, dst_height, src_height);

  DstFormat *dst_cursor = dst_data;
  for (size_t dst_y = 0; dst_y < dst_height; ++dst_y)
  {
    size_t src_y = dst_y * src_height / dst_height;
    const SrcFormat *src_row = &src_data[src_y * src_width];
    for (size_t dst_x = 0; dst_x < dst_width; ++dst_x)
    {
      size_t src_x = dst_x * src_width / dst_width;
      *dst_cursor++ = PixelFormat::convert<SrcFormat, DstFormat>(src_row[src_x]);
    }
  }
}
