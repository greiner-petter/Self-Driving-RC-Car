#pragma once

#include "IntersectionConstants.h"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

using std::size_t;
using std::vector;

template<size_t N>
class Histogram {
    public:
    size_t get_highest_point_index();
    uint32_t get_highest_point_value();
    void clear();
    void add_to_index(size_t index, uint32_t val);
    void blur();
    vector<size_t> get_peaks(uint32_t minimal_height);
    size_t get_highest_fulfilling_condition(std::function<bool(const size_t, const uint32_t)> requirement);
    void debug_print();

    private:
        std::array<uint32_t, N> data;
};

template class Histogram<INTERSECTION_DEGREE_SIZE>;
template class Histogram<INTERSECTION_Y_LENGTH_SIZE>;
template class Histogram<2 * intersection_classification::INITIAL_SEARCH>;
