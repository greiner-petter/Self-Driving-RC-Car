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

static bool running = true;
ocLogger *logger;

constexpr bool in_curve = true;

static void signal_handler(int)
{
    running = false;
}

int main() {
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    

    ocMember member(ocMemberId::Intersection_Detection, "Intersection_Detection");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    logger = member.get_logger();

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Lines_Available);
    socket->send_packet(ipc_packet);

    // Listen for detected Lines
    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
                {
                    case ocMessageId::Lines_Available:
                    {
                        vector<vector<Point>> lines;
                        ocBufferReader reader = ipc_packet.read_from_start();
                        size_t num_contours;
                        reader.read(&num_contours);

                        for (size_t i = 0; i < num_contours; ++i) {
                            vector<Point> points;
                            size_t num_points;
                            reader.read(&num_points);
                            double num_in_roi = 0;
                            for (size_t j = 0; j < num_points; ++j) {
                                Point p;
                                reader.read(&p.x);
                                reader.read(&p.y);
                                points.push_back(p);
                                // TODO: detect lane and use that as ROI
                                if (p.inside(Rect(170, 0, 390, 370))) {
                                    ++num_in_roi;
                                }
                            }
                            // only accept line, if at least a certain percentage is in the ROI
                            if (num_in_roi > (double) points.size() * 0.66) {
                                lines.push_back(points);
                            }
                        }

                        vector<vector<Point>> lines_filtered;
                        uint32_t distance = 0;
                        uint8_t directions = 6;
                        for (auto points : lines) {
                            // TODO: Put a way better filtering in place here!
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
                                if ((degree > 86 && degree < 94) || 
                                        (degree > 276 && degree < 274)) {
                                    auto norm = cv::norm(pos);
                                    if (norm > 30) {
                                        distance = 123;
                                        logger->log("degree = %d | norm = %f", (int) degree, norm);

                                    }
                                }
                            }
                        }
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
    }

    return 0;
}
