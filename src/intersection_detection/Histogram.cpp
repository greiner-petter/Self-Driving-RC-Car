#include "Histogram.h"
#include <cassert>
#include <iostream>

template <size_t N>
void Histogram<N>::clear() {
    for (size_t i = 0; i < N; ++i) {
        this->data.at(i) = 0;
    }
}

template <size_t N>
size_t Histogram<N>::get_highest_point_index() {
    uint32_t max = this->data.at(0);
    size_t index = 0;
    for (size_t i = 0; i < N; ++i) {
        if (data.at(i) > max) {
            index = i;
            max = data.at(i);
        }
    }
    return index;
}

template <size_t N>
uint32_t Histogram<N>::get_highest_point_value() {
    return this->data.at(this->get_highest_point_index());
}

template <size_t N>
void Histogram<N>::add_to_index(size_t index, uint32_t val) {
    assert(index < N);
    if (index >= N) {
        return;
    }
    this->data.at(index) += val;
}

template <size_t N>
void Histogram<N>::blur() {
    std::array<uint32_t, N> new_data;
    auto &base_data = this->data;
    // TODO: Do the first and last values also via a loop
    // TODO: also consider a dynamic blur size
    uint32_t val = base_data.at(0)
        + base_data.at(1)
        + base_data.at(2);
    new_data.at(0) = val / 3;
    val = base_data.at(0)
        + base_data.at(1)
        + base_data.at(2)
        + base_data.at(3);
    new_data.at(1) = val / 4;
    for (size_t i = 2; i < N - 2; ++i) {
        val = base_data.at(i - 2)
            + base_data.at(i - 1)
            + base_data.at(i)
            + base_data.at(i + 1)
            + base_data.at(i + 2);
        new_data.at(i) = val / 5;
    }
    val = base_data.at(N - 4)
        + base_data.at(N - 3)
        + base_data.at(N - 2)
        + base_data.at(N - 1);
    new_data.at(N - 2) = val / 4;
    val = base_data.at(N - 3)
        + base_data.at(N - 2)
        + base_data.at(N - 1);
    new_data.at(N - 1) = val / 3;
    this->data = new_data;
}

template <size_t N>
vector<size_t> Histogram<N>::get_peaks(uint32_t minimal_height) {
    vector<size_t> result;
    size_t first = 0;
    bool currently_found = false;
    for (size_t i = 0; i < N; ++i) {
        bool this_time_found = this->data.at(i) > minimal_height;
        if (currently_found != this_time_found) {
            if (!currently_found) {
                first = i;
            } else {
                size_t mid = ((i - first) / 2) + first;
                result.push_back(mid);
            }
        }
        currently_found = this_time_found;
    }
    if (currently_found) {
        size_t mid = ((N - 1 - first) / 2) + first;
        result.push_back(mid);
    }
    return result;
}

template <size_t N>
void Histogram<N>::debug_print() {
    std::cout << "Debug print for Histogram with " << N << " entries:\n";
    std::cout << "--------------\n";
    for (const auto &val : this->data) {
        std::cout << val << '\n';
    }
    std::cout << "--------------" << std::endl;
}

template <size_t N>
size_t Histogram<N>::get_highest_fulfilling_condition(std::function<bool(const size_t, const uint32_t)> requirement) {
    for (size_t i = N - 1; i >= 1; --i) {
        if (requirement(i, this->data.at(i))) {
            return i;
        }
    }
    // extra check so we don't have a infinite loop due to signess!
    if (requirement(0, this->data.at(0))) {
        return 0;
    }
    return N;
}
