#pragma once

#include <cstdint>

// allowed +/- for horizontal lines
#define ALLOWED_DEGREE_RANGE 8

// amount of values to take for blurring the histogram; changing this
// requires manual changes in the code too!
#define HISTOGRAM_BLUR_SIZE 4

// for a part of the "blurred" histogram to be considered
// it has to be at least LONGEST * REQUIRED_LENGTH_MULT
#define REQUIRED_LENGTH_MULT 0.8

// maximum required amount of parts in histogram that fulfils
// the previous condition. If it's too much there is bad data
// *or* a curve
#define MAX_FOUND_RESULTS 2

// required positive images in the last 8 frames to count
// result as actual positive and publish data
#define REQUIRED_CONSEC 3


constexpr std::uint32_t REQUIRED_H_LENGTH_INTERSECTION = 7;

constexpr std::size_t INTERSECTION_DEGREE_SIZE = 180;
constexpr std::size_t INTERSECTION_Y_LENGTH_SIZE = 400;

// measured with chessboard
constexpr double CM_PER_PIXEL = 39.9 / 63.0;

namespace intersection_classification {
    // search in both directions to find start and end
    constexpr std::size_t INITIAL_SEARCH = 20;

    constexpr std::size_t INTERSECTION_SQUARE_PX_W = 50;
    constexpr std::size_t INTERSECTION_SQUARE_PX_H = 90;
}
