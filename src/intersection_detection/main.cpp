#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include "Histogram.h"
#include "IntersectionConstants.h"
#include "IntersectionClassification.h"
#include <signal.h>
#include <csignal>
#include <bit>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

using cv::Point2f;
using cv::Rect;
using cv::Point;
using cv::Mat;

using cv::RETR_LIST;
using cv::CHAIN_APPROX_NONE;

using std::vector;

//#define LOG_NEGATIVE_RESULTS

static bool running = true;
ocLogger *logger;

static void signal_handler(int)
{
    running = false;
}

static void debug_print_directions(uint8_t directions) {
    std::cout << "Links: " << (directions & 1 ? "true" : "false") << "; Rechts: " << (directions & 2 ? "true" : "false") << "; Geradeaus: " << (directions & 4 ? "true" : "false") << std::endl;
}

static uint32_t distance_between_points_indexed(const vector<Point> &points, size_t &start_index) {
    assert(start_index < points.size() - 1);
    const Point &p1 = points.at(start_index);
    const Point &p2 = points.at(start_index + 1);
    return (uint32_t) cv::norm(p1 - p2);
}

static uint32_t calculate_angle_indexed(const vector<Point> &points, size_t &start_index) {
    assert(start_index < points.size() - 1);
    const Point &p1 = points.at(start_index);
    const Point &p2 = points.at(start_index + 1);
    Point pos = p2 - p1;
    double rad = atan2(pos.y, pos.x) - 0.5 * CV_PI;
    if (rad < 0) {
        rad += 2 * CV_PI;
    }
    double degree = rad * (180 / CV_PI);
    // 0° means pointing left, 90° means upward and 180° means pointing right
    return (uint32_t)(((int) degree + 90) % 180);
}

static void generate_angle_length_histogram(Histogram<INTERSECTION_DEGREE_SIZE> &histogram, const vector<vector<Point>> &input_data) {
    for (const vector<Point> &points : input_data) {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            uint32_t degree_adjusted = calculate_angle_indexed(points, i);
            uint32_t distance = distance_between_points_indexed(points, i);
            histogram.add_to_index(degree_adjusted, distance);
        }
    }
}

static void generate_filtered_histogram(Histogram<INTERSECTION_Y_LENGTH_SIZE> &histogram, const vector<vector<Point>> &input_data) {
    for (auto &points : input_data) {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            uint32_t degree_adjusted = calculate_angle_indexed(points, i);
            if (degree_adjusted < ALLOWED_DEGREE_RANGE || degree_adjusted > 180 - ALLOWED_DEGREE_RANGE) {
                uint32_t distance = distance_between_points_indexed(points, i);
                size_t mid = ((size_t)points.at(i).y + (size_t) points.at(i + 1).y) / 2;
                histogram.add_to_index(mid, distance);
            }
        }
    }
}

static vector<vector<Point>> detect_lines_in_image(Mat &image) {
    vector<vector<Point>> contours;
    findContours(image, contours, RETR_LIST, CHAIN_APPROX_NONE);

    vector<vector<Point>> cleaned_data;
    for (auto &contour : contours) {
        double len = cv::arcLength(contour, false);
        if (len < 30) {
            continue;
        }
        vector<Point> reduced_contour;
        double epsilon = 0.007 * arcLength(contour, false);
        approxPolyDP(contour, reduced_contour, epsilon, false);
        cleaned_data.push_back(reduced_contour);
    }
    return cleaned_data;
}

int main() {
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    

    ocMember member(ocMemberId::Intersection_Detection, "Intersection_Detected");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    logger = member.get_logger();
    ocSharedMemory *shared_memory = member.get_shared_memory();

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Birdseye_Image_Available);
    socket->send_packet(ipc_packet);

    // Listen for detected Lines
    int32_t socket_status;
    while (running && 0 < (socket_status = socket->read_packet(ipc_packet, true)))
    {
        switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Birdseye_Image_Available:
                {
                    ocBufferReader reader = ipc_packet.read_from_start();
                    uint8_t bit;
                    reader.read(&bit);
                    static uint32_t distance;
                    distance = 0;
                    Mat image(400, 400, CV_8UC1, shared_memory->bev_data[2 | bit].img_buffer);
                    vector<vector<Point>> lines = detect_lines_in_image(image);
                    static Histogram<INTERSECTION_DEGREE_SIZE> angle_length_hist;
                    angle_length_hist.clear();
                    generate_angle_length_histogram(angle_length_hist, lines);
                    angle_length_hist.blur();
                    static uint8_t last_found = 0;
                    last_found <<= 1;

                    // indices of where the peaks are
                    vector<size_t> histogram_peak_indices = angle_length_hist.get_peaks((uint32_t) ((double) angle_length_hist.get_highest_point_value() * REQUIRED_LENGTH_MULT));

                    if (histogram_peak_indices.size() > MAX_FOUND_RESULTS) {
#ifdef LOG_NEGATIVE_RESULTS
                        logger->log("Skipping this frame since we found %llu results", (unsigned long long) histogram_peak_indices.size());
#endif
                        continue;
                    }
                    // TODO: Find diagonal (45° and 135°) and if they exist skip due to curve

                    static Histogram<INTERSECTION_Y_LENGTH_SIZE> histogram_filtered;
                    histogram_filtered.clear();
                    generate_filtered_histogram(histogram_filtered, lines);
                    histogram_filtered.blur();

                    uint32_t highest_in_filtered = histogram_filtered.get_highest_point_value();
                    if (highest_in_filtered < REQUIRED_H_LENGTH_INTERSECTION) {
#ifdef LOG_NEGATIVE_RESULTS
                        logger->log("Skipping this frame since the length was only %lu", (unsigned long) highest_in_filtered);
#endif
                        continue;
                    }

                    // y in bev
                    size_t found_line_y = histogram_filtered.get_highest_fulfilling_condition([](const size_t index, const uint32_t val) -> bool {
                            return index <= 394 && val >= REQUIRED_H_LENGTH_INTERSECTION;
                    });

                    IntersectionPostprocessing proc(image, found_line_y);
                    if (!proc.calculate_result()) {
#ifdef LOG_NEGATIVE_RESULTS
                        logger->log("Skipping frame since calculating the result didn't yield the required result");
#endif
                        continue;
                    }

                    size_t pixel_height = 399 - proc.get_height();
                    distance = (uint32_t) (((double) pixel_height) * CM_PER_PIXEL);
                    if (distance > 40) {
#ifdef LOG_NEGATIVE_RESULTS
                        logger->log("Not publishing the distance %lu since it's too high", (unsigned long) distance);
#endif
                        continue;
                    }

            logger->log("Distance in cm: %lu", (unsigned long) distance);

                    last_found |= 1;

                    if (std::popcount(last_found) < REQUIRED_CONSEC) {
#ifdef LOG_NEGATIVE_RESULTS
                        logger->log("Skipping this frame since only the last %u results were positive", (unsigned int) std::popcount(last_found));
#endif
                        continue;
                    }

                    uint8_t directions = proc.get_possible_directions();
                    // debug_print_directions(directions);
                    if (distance != 0) {
                        ipc_packet.clear();
                        ipc_packet.set_sender(ocMemberId::Intersection_Detection);
                        ipc_packet.set_message_id(ocMessageId::Intersection_Detected);
                        ipc_packet.clear_and_edit()
                            .write(distance)
                            .write(directions);
                        socket->send_packet(ipc_packet);
                    }

                } break;
                default:
                {
                    ocMessageId msg_id = ipc_packet.get_message_id();
                    ocMemberId  mbr_id = ipc_packet.get_sender();
                    logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                } break;
            }
    }

    return 0;
}
