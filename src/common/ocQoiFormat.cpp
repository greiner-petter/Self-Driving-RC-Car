#include "ocQoiFormat.h"

#include <cstdint> // uint8_t

#include "ocCommon.h"

// https://qoiformat.org/qoi-specification.pdf

namespace oc::qoi
{

DecodeStatus decode_metadata(
  const std::byte *input,
  size_t           input_length,
  size_t          *width,
  size_t          *height,
  size_t          *channels)
{
  struct Header
  {
    char     magic[4];
    uint32_t width_be;
    uint32_t height_be;
    uint8_t  channels;
    uint8_t  colorspace;
  };

  if (input_length < sizeof(Header)) return DecodeStatus::Input_Too_Short;
  const Header *header = (const Header *)input;

  const char *magic = "qoif";
  for (int i = 0; i < 4; ++i)
    if (magic[i] != header->magic[i])
      return DecodeStatus::Magic_Number_Wrong;

  if (3 != header->channels && 4 != header->channels)
    return DecodeStatus::Unsupported_Channels;

  if (0 != header->colorspace && 1 != header->colorspace)
    return DecodeStatus::Unsupported_Colorspace;

  if (width)    *width    = byteswap(header->width_be);
  if (height)   *height   = byteswap(header->height_be);
  if (channels) *channels = header->channels;

  return DecodeStatus::Success;
}

DecodeResult decode(
  const std::byte *input,
  size_t           input_length,
  std::byte       *output,
  size_t           output_capacity)
{
  size_t width, height, channels;
  DecodeStatus status = decode_metadata(input, input_length, &width, &height, &channels);

  if (DecodeStatus::Success != status)
    return {status, {}, {}};

  if (output_capacity < width * height * channels)
    return {DecodeStatus::Too_Many_Pixels, {}, {}};

  uint8_t array[64][4] = {};
  uint8_t r = 0, g = 0, b = 0, a = 255;

  size_t pixel_index = 0;
  size_t index = 14;
  while (index < input_length - 8)
  {
    uint32_t run = 1;
    if (0xFE == uint8_t(input[index])) // QOI_OP_RGB
    {
      r = uint8_t(input[index + 1]);
      g = uint8_t(input[index + 2]);
      b = uint8_t(input[index + 3]);
      index += 4;
    }
    else if (0xFF == uint8_t(input[index])) // QOI_OP_RGBA
    {
      r = uint8_t(input[index + 1]);
      g = uint8_t(input[index + 2]);
      b = uint8_t(input[index + 3]);
      a = uint8_t(input[index + 4]);
      index += 5;
    }
    else if (0x00 == (uint8_t(input[index]) & 0xC0)) // QOI_OP_INDEX
    {
      r = array[uint8_t(input[index]) & 0x3F][0];
      g = array[uint8_t(input[index]) & 0x3F][1];
      b = array[uint8_t(input[index]) & 0x3F][2];
      a = array[uint8_t(input[index]) & 0x3F][3];
      index += 1;
    }
    else if (0x40 == (uint8_t(input[index]) & 0xC0)) // QOI_OP_DIFF
    {
      r = uint8_t(r + ((uint8_t(input[index]) & 0x30) >> 4) - 2);
      g = uint8_t(g + ((uint8_t(input[index]) & 0x0C) >> 2) - 2);
      b = uint8_t(b + ((uint8_t(input[index]) & 0x03) >> 0) - 2);
      index += 1;
    }
    else if (0x80 == (uint8_t(input[index]) & 0xC0)) // QOI_OP_LUMA
    {
      uint32_t dg = (uint8_t(input[index]) & 0x3F) - 32;
      r += uint8_t(((uint8_t(input[index + 1]) & 0xF0) >> 4) - 8 + dg);
      g += uint8_t(dg);
      b += uint8_t(((uint8_t(input[index + 1]) & 0x0F) >> 0) - 8 + dg);
      index += 2;
    }
    else //if (0xC0 == uint8_t(input[index]) & 0xC0) // QOI_OP_RUN
    {
      run = (uint8_t(input[index]) & 0x3F) + 1;
      index += 1;
    }

    while (run--)
    {
      output[pixel_index * channels + 0] = std::byte(b);
      output[pixel_index * channels + 1] = std::byte(g);
      output[pixel_index * channels + 2] = std::byte(r);
      if (4 == channels)
      {
        output[pixel_index * channels + 3] = std::byte(a);
      }
      pixel_index += 1;
      if (width * height <= pixel_index)
      {
        // We could check for the 8 end-bytes here, but why bother?
        return {
          DecodeStatus::Success,
          width * height * channels,
          (3 == channels) ? ocPixelFormat::Bgr_U8 : ocPixelFormat::Bgra_U8
        };
      }
    }

    uint32_t arr_index = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
    array[arr_index][0] = r;
    array[arr_index][1] = g;
    array[arr_index][2] = b;
    array[arr_index][3] = a;
  }

  return {DecodeStatus::Too_Few_Pixels, {}, {}};
}

EncodeResult encode(
  const std::byte *input,
  size_t           input_width,
  size_t           input_height,
  ocPixelFormat    input_format,
  std::byte       *output,
  size_t           output_capacity)
{
  if (output_capacity < 14)
    return {EncodeStatus::Output_Too_Small, {}};

  size_t channels;
  switch (input_format)
  {
    case ocPixelFormat::Gray_U8:
      channels = 1;
      break;
    case ocPixelFormat::Bgr_U8:
      channels = 3;
      break;
    case ocPixelFormat::Bgra_U8:
      channels = 4;
      break;
    default:
      return {EncodeStatus::Unsupported_Format, {}};
  }

  size_t output_index = 0;
  output[output_index++] = std::byte('q');
  output[output_index++] = std::byte('o');
  output[output_index++] = std::byte('i');
  output[output_index++] = std::byte('f');
  output[output_index++] = std::byte(input_width >> 24);
  output[output_index++] = std::byte(input_width >> 16);
  output[output_index++] = std::byte(input_width >>  8);
  output[output_index++] = std::byte(input_width >>  0);
  output[output_index++] = std::byte(input_height >> 24);
  output[output_index++] = std::byte(input_height >> 16);
  output[output_index++] = std::byte(input_height >>  8);
  output[output_index++] = std::byte(input_height >>  0);
  output[output_index++] = std::byte(4 == channels ? 4 : 3);
  output[output_index++] = std::byte(0);

  uint8_t array[64][4] = {};
  uint8_t pr = 0, pg = 0, pb = 0, pa = 255; // previous color
  size_t run = 0;

  for (size_t y = 0; y < input_height; ++y)
  for (size_t x = 0; x < input_width;  ++x)
  {
    uint8_t r, g, b, a = 255;

    b = uint8_t(input[(x + y * input_width) * channels + 0]);
    if (3 <= channels)
    {
      g = uint8_t(input[(x + y * input_width) * channels + 1]);
      r = uint8_t(input[(x + y * input_width) * channels + 2]);
      if (4 == channels)
      {
        a = uint8_t(input[(x + y * input_width) * channels + 3]);
      }
    }
    else
    {
      g = r = b;
    }

    int8_t dr = int8_t(r - pr);
    int8_t dg = int8_t(g - pg);
    int8_t db = int8_t(b - pb);
    int8_t da = int8_t(a - pa);

    pr = r;
    pg = g;
    pb = b;
    pa = a;

    if (0 == dr && 0 == dg && 0 == db && 0 == da)
    {
      run += 1;
      if (62 == run)
      {
        if (output_capacity < output_index + 1)
          return {EncodeStatus::Output_Too_Small, {}};
        output[output_index++] = std::byte(0xFD);
        run = 0;
      }
      continue;
    }

    if (run)
    {
      if (output_capacity < output_index + 1)
        return {EncodeStatus::Output_Too_Small, {}};
      output[output_index++] = std::byte(0xC0 | (run - 1));
      run = 0;
    }

    uint32_t arr_index = (r * 3 + g * 5 + b * 7 + a * 11) % 64;
    if (r == array[arr_index][0] &&
        g == array[arr_index][1] &&
        b == array[arr_index][2] &&
        a == array[arr_index][3])
    {
      if (output_capacity < output_index + 1)
        return {EncodeStatus::Output_Too_Small, {}};
      output[output_index++] = std::byte(0x00 | arr_index);
      continue;
    }

    array[arr_index][0] = r;
    array[arr_index][1] = g;
    array[arr_index][2] = b;
    array[arr_index][3] = a;

    if (-2 <= dr && dr <= 1 &&
        -2 <= dg && dg <= 1 &&
        -2 <= db && db <= 1 &&
        0 == da)
    {
      if (output_capacity < output_index + 1)
        return {EncodeStatus::Output_Too_Small, {}};
      output[output_index++] = std::byte(0x40 | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2));
      continue;
    }

    int8_t dr_dg = int8_t(dr - dg);
    int8_t db_dg = int8_t(db - dg);
    if (-32 <= dg    && dg    <= 31 &&
         -8 <= dr_dg && dr_dg <=  7 &&
         -8 <= db_dg && db_dg <=  7 &&
        0 == da)
    {
      if (output_capacity < output_index + 2)
        return {EncodeStatus::Output_Too_Small, {}};
      output[output_index++] = std::byte(0x80 | (dg + 32));
      output[output_index++] = std::byte(((dr_dg + 8) << 4) | (db_dg + 8));
      continue;
    }

    if (da == 0)
    {
      if (output_capacity < output_index + 4)
        return {EncodeStatus::Output_Too_Small, {}};
      output[output_index++] = std::byte(0xFE);
      output[output_index++] = std::byte(r);
      output[output_index++] = std::byte(g);
      output[output_index++] = std::byte(b);
      continue;
    }

    if (output_capacity < output_index + 5)
      return {EncodeStatus::Output_Too_Small, {}};
    output[output_index++] = std::byte(0xFF);
    output[output_index++] = std::byte(r);
    output[output_index++] = std::byte(g);
    output[output_index++] = std::byte(b);
    output[output_index++] = std::byte(a);
  }

  if (run)
  {
    if (output_capacity < output_index + 1)
      return {EncodeStatus::Output_Too_Small, {}};
    output[output_index++] = std::byte(0xC0 | (run - 1));
  }

  if (output_capacity < output_index + 8)
    return {EncodeStatus::Output_Too_Small, {}};
  output[output_index++] = std::byte(0x01);
  output[output_index++] = std::byte(0x00);
  output[output_index++] = std::byte(0x00);
  output[output_index++] = std::byte(0x00);
  output[output_index++] = std::byte(0x00);
  output[output_index++] = std::byte(0x00);
  output[output_index++] = std::byte(0x00);
  output[output_index++] = std::byte(0x00);

  return {EncodeStatus::Success, output_index};
}

} // namespace oc::qoi
