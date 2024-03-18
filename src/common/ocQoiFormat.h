#pragma once

#include "ocImageOps.h" // ocPixelFormat

#include <cstddef> // size_t, std::byte

// https://qoiformat.org/qoi-specification.pdf

namespace oc::qoi
{

enum class DecodeStatus
{
  Success,
  Input_Too_Short,
  Magic_Number_Wrong,
  Width_Too_Large,
  Height_Too_Large,
  Unsupported_Channels,
  Unsupported_Colorspace,
  Too_Many_Pixels,
  Too_Few_Pixels
};

constexpr const char *to_string(DecodeStatus status)
{
  switch (status)
  {
    case DecodeStatus::Success:                return "Success";
    case DecodeStatus::Input_Too_Short:        return "Input_Too_Short";
    case DecodeStatus::Magic_Number_Wrong:     return "Magic_Number_Wrong";
    case DecodeStatus::Width_Too_Large:        return "Width_Too_Large";
    case DecodeStatus::Height_Too_Large:       return "Height_Too_Large";
    case DecodeStatus::Unsupported_Channels:   return "Unsupported_Channels";
    case DecodeStatus::Unsupported_Colorspace: return "Unsupported_Colorspace";
    case DecodeStatus::Too_Many_Pixels:        return "Too_Many_Pixels";
    case DecodeStatus::Too_Few_Pixels:         return "Too_Few_Pixels";
  }
  return "???";
}

DecodeStatus decode_metadata(
  const std::byte *input,
  size_t           input_length,
  size_t          *width,
  size_t          *height,
  size_t          *channels);

struct DecodeResult
{
  DecodeStatus  status;
  size_t        output_length;
  ocPixelFormat output_format;
};

DecodeResult decode(
  const std::byte *input,
  size_t           input_length,
  std::byte       *output,
  size_t           output_capacity);

enum class EncodeStatus
{
  Success,
  Unsupported_Format,
  Output_Too_Small,
};

constexpr const char *to_string(EncodeStatus status)
{
  switch (status)
  {
    case EncodeStatus::Success:            return "Success";
    case EncodeStatus::Unsupported_Format: return "Unsupported_Format";
    case EncodeStatus::Output_Too_Small:   return "Output_Too_Small";
  }
  return "???";
}

struct EncodeResult
{
  EncodeStatus status;
  size_t       output_length;
};

EncodeResult encode(
  const std::byte *input,
  size_t           input_width,
  size_t           input_height,
  ocPixelFormat    input_format,
  std::byte       *output,
  size_t           output_capacity);

} // namespace oc::qoi
