#include "IntersectionClassification.h"
#include "Histogram.h"
#include <vector>

using cv::Mat;
using std::vector;

// draw lines on where the intersections are tested in the BEV instead of testing for them
//#define DRAW_DEBUG_LINE

IntersectionPostprocessing::IntersectionPostprocessing(Mat image, size_t found_line) {
    this->data = image;
    this->some_line = found_line;
}

bool IntersectionPostprocessing::calculate_result() {
    static Histogram<2 * intersection_classification::INITIAL_SEARCH> histo_left;
    static Histogram<2 * intersection_classification::INITIAL_SEARCH> histo_right;
    histo_left.clear();
    histo_right.clear();
    for (size_t i = 0; i < 2 * intersection_classification::INITIAL_SEARCH; ++i) {
        const int32_t search_y = (int32_t) this->some_line + ((int32_t) intersection_classification::INITIAL_SEARCH - (int32_t) i);
        if (search_y < 0 || (uint32_t) search_y >=  399) {
            // out of bounds
            continue;
        }
        constexpr uint32_t SEARCH_LEFT = intersection_classification::BEV_MID - intersection_classification::LINETEST_DIFF;
        constexpr uint32_t SEARCH_RIGHT = intersection_classification::BEV_MID + intersection_classification::LINETEST_DIFF;

        const uint32_t data_left = this->data.at<uint8_t>(search_y, SEARCH_LEFT);
        const uint32_t data_right = this->data.at<uint8_t>(search_y, SEARCH_RIGHT);
        histo_left.add_to_index(i, data_left);
        histo_right.add_to_index(i, data_right);

#ifdef DRAW_DEBUG_LINE
        this->data.at<uint8_t>(search_y, SEARCH_LEFT) = 255;
        this->data.at<uint8_t>(search_y, SEARCH_RIGHT) = 255;
#endif
    }

    //histo.debug_print();
    vector<size_t> peaks_left = histo_left.get_peaks(20);
    vector<size_t> peaks_right = histo_right.get_peaks(20);
    if (peaks_left.size() != 2 && peaks_right.size() != 2) {
        return false;
    }

    vector<size_t> &peaks = peaks_left;
    if (peaks_left.size() != 2) {
        peaks = peaks_right;
    }

    // the lower line has the higher y!
    this->lower_line = intersection_classification::INITIAL_SEARCH - peaks.at(0) + this->some_line;
    this->higher_line = intersection_classification::INITIAL_SEARCH - peaks.at(1) + this->some_line;

    // TODO: (ggf. Rechteck um erwartete Kreuzung und schauen, wie viele Pixel weiß sind
    // --> Plausiblitätsprüfung)

    uint8_t possibilities = 0;
    possibilities |= this->test_forward();
    possibilities |= this->test_left();
    possibilities |= this->test_right();

    // if all directions are free it's most likely that canny didn't output the line furthest away
    if (std::popcount(possibilities) == 3) {
        possibilities &= ~(0b100);
    } else if (std::popcount(possibilities) == 1) {
        // this isn't safe but there is probably light interfering *somewhere*; assume it's also possible to drive straight
        possibilities |= 0b100;
    } else if (std::popcount(possibilities) != 2) {
        // this is **really** unsafe since it's really just a guess to where we could drive since the decider expects this...
        // a better approach would be to publish that the data isn't stable
        // FIXME: FIX THIS! Especially since it's not true for large distances!
        possibilities = 0b101;
    }
    this->possible_directions = possibilities;

    // Mit Linien basierend auf Linien herausfinden, in welche Richtungen gefahren werden könnte
    // --> In Variable
    return true;
}

size_t IntersectionPostprocessing::get_height() {
    return this->lower_line;
}

uint8_t IntersectionPostprocessing::get_possible_directions() {
    return this->possible_directions;
}


uint8_t IntersectionPostprocessing::test_forward() {
    int32_t x = 200;
    size_t y_low = this->higher_line - (3 * intersection_classification::INTERSECTION_SQUARE_PX_H) / 4;
    for (size_t y = y_low; y >= y_low - intersection_classification::INTERSECTION_SQUARE_PX_H / 2; --y) {
#ifdef DRAW_DEBUG_LINE
        this->data.at<uint8_t>((int32_t) y, (int32_t) x) = 255;
#else
        if (this->data.at<uint8_t>((int32_t) y, x) > 0) {
            return 0;
        }
#endif
    }
    return 0b100;
}

uint8_t IntersectionPostprocessing::test_left() {
    size_t y = this->higher_line - (intersection_classification::INTERSECTION_SQUARE_PX_H * 3) / 4;
    size_t x_right = 200 - (intersection_classification::INTERSECTION_SQUARE_PX_W * 3) / 4;
    for (size_t x = x_right; x >= x_right - intersection_classification::INTERSECTION_SQUARE_PX_W / 2; --x) {
#ifdef DRAW_DEBUG_LINE
        this->data.at<uint8_t>((int32_t) y, (int32_t) x) = 255;
#else
        if (this->data.at<uint8_t>((int32_t) y, (int32_t) x) > 0) {
            return 0;
        }
#endif
    }
    return 0b1;
}

uint8_t IntersectionPostprocessing::test_right() {
    size_t y = this->higher_line - intersection_classification::INTERSECTION_SQUARE_PX_H / 4;
    size_t x_left = 200;
    for (size_t x = x_left; x < x_left + intersection_classification::INTERSECTION_SQUARE_PX_W / 2; ++x) {
#ifdef DRAW_DEBUG_LINE
        this->data.at<uint8_t>((int32_t) y, (int32_t) x) = 255;
#else
        if (this->data.at<uint8_t>((int32_t) y, (int32_t) x) > 0) {
            return 0;
        }
#endif
    }
    return 0b10;
}
