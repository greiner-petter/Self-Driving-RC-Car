#include "ocImageOps.h"

const char *to_string(ocPixelFormat pixel_format)
{
  switch (pixel_format)
  {
    case ocPixelFormat::None: return "ocPixelFormat::None";
    case ocPixelFormat::Gray_U8: return "ocPixelFormat::Gray_U8";
    case ocPixelFormat::Bgr_U8: return "ocPixelFormat::Bgr_U8";
    case ocPixelFormat::Bgra_U8: return "ocPixelFormat::Bgra_U8";
    case ocPixelFormat::Rgb_F32: return "ocPixelFormat::Rgb_F32";
  }
  return "<unknown>";
}

uint8_t bytes_per_pixel(ocPixelFormat pixel_format)
{
  switch (pixel_format)
  {
    case ocPixelFormat::None: return 0;
    case ocPixelFormat::Gray_U8: return 1;
    case ocPixelFormat::Bgr_U8: return 3;
    case ocPixelFormat::Bgra_U8: return 4;
    case ocPixelFormat::Rgb_F32: return 12;
  }
  return 0;
}

using namespace PixelFormat;

template<> GrayU8 PixelFormat::convert<GrayU8, GrayU8>(GrayU8 pixel)
{
  return pixel;
}
template<> BgrU8 PixelFormat::convert<GrayU8, BgrU8>(GrayU8 pixel)
{
  BgrU8 result;
  result.b = pixel.value;
  result.g = pixel.value;
  result.r = pixel.value;
  return result;
}
template<> RgbF32 PixelFormat::convert<GrayU8, RgbF32>(GrayU8 pixel)
{
  float value = (float)pixel.value * (1.0f / 255.0f);
  RgbF32 result;
  result.r = value;
  result.g = value;
  result.b = value;
  return result;
}

template<> GrayU8 PixelFormat::convert<BgrU8, GrayU8>(BgrU8 pixel)
{
  GrayU8 result;
  result.value = (uint8_t)roundf((float)pixel.r * 0.2126f +
                                 (float)pixel.g * 0.7152f +
                                 (float)pixel.b * 0.0722f);
  return result;
}
template<> BgrU8 PixelFormat::convert<BgrU8, BgrU8>(BgrU8 pixel)
{
  return pixel;
}
template<> RgbF32 PixelFormat::convert<BgrU8, RgbF32>(BgrU8 pixel)
{
  RgbF32 result;
  result.r = (float)pixel.r * (1.0f / 255.0f);
  result.g = (float)pixel.g * (1.0f / 255.0f);
  result.b = (float)pixel.b * (1.0f / 255.0f);
  return result;
}

template<> GrayU8 PixelFormat::convert<BgrxU8, GrayU8>(BgrxU8 pixel)
{
  // TODO: LUT for byte->byte
  GrayU8 result;
  result.value = (uint8_t)roundf((float)pixel.r * 0.2126f +
                                 (float)pixel.g * 0.7152f +
                                 (float)pixel.b * 0.0722f);
  return result;
}
template<> BgrU8 PixelFormat::convert<BgrxU8, BgrU8>(BgrxU8 pixel)
{
  BgrU8 result;
  result.b = pixel.b;
  result.g = pixel.g;
  result.r = pixel.r;
  return result;
}
template<> RgbF32 PixelFormat::convert<BgrxU8, RgbF32>(BgrxU8 pixel)
{
  RgbF32 result;
  result.r = (float)pixel.r * (1.0f / 255.0f);
  result.g = (float)pixel.g * (1.0f / 255.0f);
  result.b = (float)pixel.b * (1.0f / 255.0f);
  return result;
}

void convert_gray_u8_to_gray_u8(const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<GrayU8, GrayU8>(
    (const GrayU8 *)src_data,
    src_width, src_height,
    (GrayU8 *)dst_data,
    dst_width, dst_height);
}
void convert_gray_u8_to_bgr_u8(const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<GrayU8, BgrU8>(
    (const GrayU8 *)src_data,
    src_width, src_height,
    (BgrU8 *)dst_data,
    dst_width, dst_height);
}
void convert_gray_u8_to_rgb_f32(const uint8_t *src_data, size_t src_width, size_t src_height, float *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<GrayU8, RgbF32>(
    (const GrayU8 *)src_data,
    src_width, src_height,
    (RgbF32 *)dst_data,
    dst_width, dst_height);
}

void convert_bgr_u8_to_gray_u8(const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<BgrU8, GrayU8>(
    (const BgrU8 *)src_data,
    src_width, src_height,
    (GrayU8 *)dst_data,
    dst_width, dst_height);
}
void convert_bgr_u8_to_bgr_u8(const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<BgrU8, BgrU8>(
    (const BgrU8 *)src_data,
    src_width, src_height,
    (BgrU8 *)dst_data,
    dst_width, dst_height);
}
void convert_bgr_u8_to_rgb_f32(const uint8_t *src_data, size_t src_width, size_t src_height, float *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<BgrU8, RgbF32>(
    (const BgrU8 *)src_data,
    src_width, src_height,
    (RgbF32 *)dst_data,
    dst_width, dst_height);
}

void convert_bgra_u8_to_gray_u8(const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<BgrxU8, GrayU8>(
    (const BgrxU8 *)src_data,
    src_width, src_height,
    (GrayU8 *)dst_data,
    dst_width, dst_height);
}
void convert_bgra_u8_to_bgr_u8(const uint8_t *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<BgrxU8, BgrU8>(
    (const BgrxU8 *)src_data,
    src_width, src_height,
    (BgrU8 *)dst_data,
    dst_width, dst_height);
}
void convert_bgra_u8_to_rgb_f32(const uint8_t *src_data, size_t src_width, size_t src_height, float *dst_data, size_t dst_width, size_t dst_height)
{
  convert_and_downscale<BgrxU8, RgbF32>(
    (const BgrxU8 *)src_data,
    src_width, src_height,
    (RgbF32 *)dst_data,
    dst_width, dst_height);
}

bool convert_to_gray_u8(ocPixelFormat src_format, const void *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  switch (src_format)
  {
    case ocPixelFormat::Gray_U8:  convert_gray_u8_to_gray_u8((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    case ocPixelFormat::Bgr_U8:   convert_bgr_u8_to_gray_u8((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    case ocPixelFormat::Bgra_U8:  convert_bgra_u8_to_gray_u8((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    default: return false;
  }
  return true;
}
bool convert_to_bgr_u8(ocPixelFormat src_format, const void *src_data, size_t src_width, size_t src_height, uint8_t *dst_data, size_t dst_width, size_t dst_height)
{
  switch (src_format)
  {
    case ocPixelFormat::Gray_U8:  convert_gray_u8_to_bgr_u8((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    case ocPixelFormat::Bgr_U8:   convert_bgr_u8_to_bgr_u8((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    case ocPixelFormat::Bgra_U8:  convert_bgra_u8_to_bgr_u8((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    default: return false;
  }
  return true;
}
bool convert_to_rgb_f32(ocPixelFormat src_format, const void *src_data, size_t src_width, size_t src_height, float *dst_data, size_t dst_width, size_t dst_height)
{
  switch (src_format)
  {
    case ocPixelFormat::Gray_U8:  convert_gray_u8_to_rgb_f32((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    case ocPixelFormat::Bgr_U8:   convert_bgr_u8_to_rgb_f32((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    case ocPixelFormat::Bgra_U8:  convert_bgra_u8_to_rgb_f32((const uint8_t *)src_data, src_width, src_height, dst_data, dst_width, dst_height); break;
    default: return false;
  }
  return true;
}
