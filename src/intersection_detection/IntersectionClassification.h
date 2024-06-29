#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

class IntersectionPostprocessing {
    public:
        IntersectionPostprocessing(cv::Mat image, size_t found_line);
        // return true, if a intersection was detected properly
        bool calculate_result();
        size_t get_height();

        // Bit positions:
        // If a bit is set the car can go:
        // 1: Turn left
        // 2: Turn right
        // 3: Keep driving straight
        uint8_t get_possible_directions();

    private:
        cv::Mat data;
        size_t lower_line = 0;
        size_t higher_line = 0;
        size_t some_line;
        uint8_t possible_directions = 0;
        uint8_t test_forward();
        uint8_t test_left();
        uint8_t test_right();
};
