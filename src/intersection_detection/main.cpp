#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <csignal>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

using cv::Point2f;
using cv::Mat;
using cv::Point;

using std::vector;

static bool running = true;
ocLogger *logger;

static void signal_handler(int)
{
    running = false;
}

int main() {
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    

    ocMember member(ocMemberId::Intersection_Detected, "Intersection_Detected");
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
                            for (size_t j = 0; j < num_points; ++j) {
                                Point p;
                                reader.read(&p.x);
                                reader.read(&p.y);
                                points.push_back(p);
                            }
                            lines.push_back(points);
                        }
                        // TODO: Handle data
                        // logger->log("Got %zd lines", lines.size());

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
