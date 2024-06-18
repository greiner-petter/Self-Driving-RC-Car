#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <csignal>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

using cv::Point2f;
using cv::Rect;
using cv::Point;

using std::vector;

// allowed +/- for horizontal lines
#define ALLOWED_DEGREE_RANGE 6

// amount of values to take for blurring the histogram; changing this
// requires manual changes in the code too!
#define HISTOGRAM_BLUR_SIZE 4

// for a part of the "blurred" histogram to be considered
// it has to be at least LONGEST / REQUIRED_LENGTH_DIV
#define REQUIRED_LENGTH_DIV 1.5

// maximum required amount of parts in histogram that fulfils
// the previous condition. If it's too much there is bad data
// *or* a curve
#define MAX_FOUND_RESULTS 2

// required consecutive images needed before it broadcasts that
// a detected intersection
#define REQUIRED_CONSEC 4

static bool running = true;
ocLogger *logger;

static void signal_handler(int)
{
    running = false;
}

static uint32_t distance_between_points(Point &p1, Point &p2) {
    return (uint32_t) cv::norm(p1 - p2);
}

static vector<vector<Point>> read_lines(ocPacket &packet) {
    vector<vector<Point>> lines;
    ocBufferReader reader = packet.read_from_start();
    size_t num_contours;
    reader.read(&num_contours);

    for (size_t i = 0; i < num_contours; ++i) {
        vector<Point> points;
        size_t num_points;
        reader.read(&num_points);
        for (size_t j = 0; j < num_points; ++j) {
            Point p;
            reader.read(&p.x);
            reader.read(&p.y);
            points.push_back(p);
        }
        lines.push_back(points);
    }
    return lines;
}

static void generate_angle_length_histogram(uint32_t *output, vector<vector<Point>> &input_data) {
    memset(output, 0, sizeof(uint32_t) * 180);
    for (auto points : input_data) {
        vector<Point> points_filtered;
        for (size_t i = 0; i < points.size() - 1; ++i) {
            Point &p1 = points.at(i);
            Point &p2 = points.at(i + 1);
            Point pos = p2 - p1;
            double rad = atan2(pos.y, pos.x) - 0.5 * CV_PI;
            if (rad < 0) {
                rad += 2 * CV_PI;
            }
            double degree = rad * (180 / CV_PI);
            // 0° means pointing left, 90° means upward and 180° means pointing right
            int degree_adjusted = ((int) degree + 90) % 180;
            uint32_t distance = distance_between_points(p1, p2);
            output[degree_adjusted] += distance;
        }
    }
}

static void generate_blurred_histogram(uint32_t *input, uint32_t *output, uint32_t *highest) {
    memset(output, 0, sizeof(uint32_t) * 180);
    *highest = 0;
    for (size_t i = 0; i < 180 - HISTOGRAM_BLUR_SIZE; ++i) {
        output[i] = (input[i] + input[i + 1] + input[i + 2] + input[i + 3]) / HISTOGRAM_BLUR_SIZE;
        if (output[i] > *highest) {
            *highest = output[i];
        }
    }
}

static void get_histogram_peak_points(uint32_t *input, const uint32_t required_value, vector<int> &output) {
    output.clear();

    bool found = false;
    for (size_t i = 0; i < 180 - HISTOGRAM_BLUR_SIZE; ++i) {
        if (input[i] > required_value) {
            if (found == false) {
                output.push_back((int) i);
            }
            found = true;
        } else {
            found = false;
        }
    }
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

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Lines_Available);
    socket->send_packet(ipc_packet);

    // Listen for detected Lines
    int32_t socket_status;
    while (running && 0 < (socket_status = socket->read_packet(ipc_packet, true)))
    {
        switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Lines_Available:
                {
                    static uint32_t distance;
                    distance = 0;
                    vector<vector<Point>> lines = read_lines(ipc_packet);
                    static uint32_t histogram[180];
                    generate_angle_length_histogram(&histogram[0], lines);

                    // the last blur_size values aren't set; also the histogram gets shifted here!
                    uint32_t histogram_blurred[180] = {0};
                    uint32_t highest;
                    generate_blurred_histogram(histogram, histogram_blurred, &highest);

                    // indices of where the peaks are
                    vector<int> histogram_peak_indices;
                    get_histogram_peak_points(histogram_blurred, (uint32_t)(highest / REQUIRED_LENGTH_DIV), histogram_peak_indices);

                    // used to avoid consecutive peaks that occur due to the blur
                    static size_t found_consec = 0;
                    if (histogram_peak_indices.size() > MAX_FOUND_RESULTS) {
                        found_consec = 0;
                        continue;
                    }
                    // TODO: Find diagonal (45° and 135°) and if they exist skip due to curve
                    bool has_in_range = false;
                    for (auto i : histogram_peak_indices) {
                        if (i > 90 - ALLOWED_DEGREE_RANGE && i < 90 + ALLOWED_DEGREE_RANGE) {
                            has_in_range = true;
                        }
                    }
                    if (!has_in_range) {
                        found_consec = 0;
                        continue;
                    }
                    if (++found_consec < REQUIRED_CONSEC) {
                        continue;
                    }
                    std::cout << "-------\n";

                    for (auto points : lines) {
                        vector<Point> points_filtered;
                        for (size_t i = 0; i < points.size() - 1; ++i) {
                            Point &p1 = points.at(i);
                            Point &p2 = points.at(i + 1);
                            Point pos = p2 - p1;
                            double rad = atan2(pos.y, pos.x) - 0.5 * CV_PI;
                            if (rad < 0) {
                                rad += 2 * CV_PI;
                            }
                            double degree = rad * (180 / CV_PI);
                            // 0° means pointing left, 90° means upward and 180° means pointing right
                            int degree_adjusted = ((int) degree + 90) % 180;
                            for (int &a : histogram_peak_indices) {
                                if (std::abs(a - degree_adjusted) < 10) {
                                    distance = (uint32_t) p1.y;
                                    std::cout << "y = " << p1.y << '\n';
                                }

                            }
                        }
                    }

                    std::cout << "\n-------\n\n\n";
                    uint8_t directions = 6;
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
