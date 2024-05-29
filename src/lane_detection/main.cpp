#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include "../common/ocCar.h"
#include "../common/ocCarConfig.h"
#include <signal.h>
#include <vector>
#include <unistd.h>

#include "../common/ocWindow.h"

#include <opencv2/opencv.hpp>

#define CAR_CONFIG_FILE "../car_properties.conf"

#define DRAW_LINE_SAMPLES

ocLogger *logger;

static bool running = true;

struct Point {
    double x, y;
};

std::vector<Point> topLeftPolygon = {{20,0}, {200,0}, {200, 20}, {20, 20}};
std::vector<Point> topRightPolygon = {{200,0}, {380,0}, {380, 20}, {200, 20}};

std::vector<Point> leftTopPolygon = {{0,0}, {20,0}, {20, 400}, {0, 400}};

std::vector<Point> rightTopPolygon = {{400,0}, {400,400}, {380, 400}, {380, 0}};

static void signal_handler(int)
{
    running = false;
}

double calcDist(std::pair<double, double> p1, std::pair<double, double> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
};

std::pair<float, float> getWindow(float x) {
    return std::pair<float, float> (x-25, x+25);
};


// Checking if a point is inside a polygon
bool point_in_polygon(Point point, std::vector<Point> polygon)
{
    int num_vertices = polygon.size();
    double x = point.x, y = point.y;
    bool inside = false;
 
    // Store the first point in the polygon and initialize
    // the second point
    Point p1 = polygon[0], p2;
 
    // Loop through each edge in the polygon
    for (int i = 1; i <= num_vertices; i++) {
        // Get the next point in the polygon
        p2 = polygon[i % num_vertices];
 
        // Check if the point is above the minimum y
        // coordinate of the edge
        if (y > std::min(p1.y, p2.y)) {
            // Check if the point is below the maximum y
            // coordinate of the edge
            if (y <= std::max(p1.y, p2.y)) {
                // Check if the point is to the left of the
                // maximum x coordinate of the edge
                if (x <= std::max(p1.x, p2.x)) {
                    // Calculate the x-intersection of the
                    // line connecting the point to the edge
                    double x_intersection
                        = (y - p1.y) * (p2.x - p1.x)
                              / (p2.y - p1.y)
                          + p1.x;
 
                    // Check if the point is on the same
                    // line as the edge or to the left of
                    // the x-intersection
                    if (p1.x == p2.x
                        || x <= x_intersection) {
                        // Flip the inside flag
                        inside = !inside;
                    }
                }
            }
        }
 
        // Store the current point as the first point for
        // the next iteration
        p1 = p2;
    }
 
    // Return the value of the inside flag
    return inside;
}

int main()
{
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    ocMember member(ocMemberId::Lane_Detection, "Lane Detection");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();
    logger = member.get_logger();

    ocCarProperties car_properties;
    read_config_file(CAR_CONFIG_FILE, car_properties, *logger);

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Lines_Available);
    socket->send_packet(ipc_packet);

    logger->log("Lane Detection started!");

    int direction = 0;

    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Lines_Available:
                {
                    cv::Mat matrix = cv::Mat(400,400,CV_8UC1, shared_memory->bev_data[1].img_buffer);
                   
                    bool isTopLeft = false;
                    bool isTopRight = false;
                    bool isLeft = false;
                    bool isRight = false;

                    for(double x = 0; x < 400; x++) {
                        for(double y = 0; y < 200; y++) {
                            int color = matrix.at<uint8_t>(y, x);

                            if(color >= 200) { // White Point
                                logger->log("%f %f", x, y);
                                if(!isTopLeft && point_in_polygon({.x= x, .y= y}, topLeftPolygon)) {
                                    isTopLeft = true;
                                } else if(!isTopRight && point_in_polygon({.x= x, .y= y}, topRightPolygon)) {
                                    isTopRight = true;
                                } else if(!isLeft && point_in_polygon({.x= x, .y= y}, leftTopPolygon)) {
                                    isLeft = true;
                                } else if(!isRight && point_in_polygon({.x= x, .y= y}, rightTopPolygon)) {
                                    isRight = true;
                                }else if(isLeft) {
                                    direction = -2;
                                } else if( isRight) {
                                  direction = 2;
                                }
                            }
                        }
                    }

                    if(isTopRight) {
                        direction = 0;
                    } 
                    
                    if(!isTopLeft && !isTopRight && isLeft) {
                        direction = -2;
                    } else if(!isTopLeft && !isTopRight && isRight) {
                        direction = 2;
                    } else if(!isTopRight && isRight) {
                        direction = 1;
                    } else if(!isTopLeft && isLeft) {
                        direction = -1;
                    } 

                    double xDest = 200 + 110 * direction;

                    double xStart = 200;
                    double yStart = 400;

                    #ifdef DRAW_LINE_SAMPLES
                        cv::line(matrix, cv::Point(xStart, yStart), cv::Point(xDest, 100), cv::Scalar(255,255,255,1), 8);
                    #endif

                    #ifdef DRAW_LINE_SAMPLES
                        cv::line(matrix, cv::Point(380, 20), cv::Point(380, 380), cv::Scalar(255,255,255,1), 2);
                        cv::line(matrix, cv::Point(20, 20), cv::Point(20, 380), cv::Scalar(255,255,255,1), 2);
                        cv::line(matrix, cv::Point(20, 20), cv::Point(380, 20), cv::Scalar(255,255,255,1), 2);
                        cv::line(matrix, cv::Point(20, 380), cv::Point(380, 380), cv::Scalar(255,255,255,1), 2);
                    #endif

                    int8_t front_angle = car_properties.front_steering_angle_to_byte(15 * direction * M_PI / 180);
                    int8_t back_angle = car_properties.rear_steering_angle_to_byte(0);

                    cv::imshow("Lane Detection", matrix);
                    char key = cv::waitKey(30);
                    if (key == 'q')
                    {
                        cv::destroyAllWindows();
                        return 0;
                    }

                    ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Start_Driving_Task);
                    ipc_packet.clear_and_edit()
                        .write<int16_t>(30)
                        .write<int8_t>(front_angle)
                        .write<int8_t>(back_angle)
                        .write<uint8_t>(0x8)
                        .write<int32_t>(car_properties.cm_to_steps(1));
                    socket->send_packet(ipc_packet);
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
}