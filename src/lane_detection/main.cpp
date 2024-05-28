#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include "../common/ocCar.h"
#include "../common/ocCarConfig.h"
#include <signal.h>
#include <vector>
#include <opencv2/opencv.hpp>

#define CAR_CONFIG_FILE "../car_properties.conf"

#define DRAW_LINE_SAMPLES

// Starts at y = 40
const int line_samples[6][2] = {
    {10, 340},
    {20, 330},
    {30, 320},
    {40, 310},
    {50, 300},
    {60, 290}
};

const int line_sample_mid = 175;

const int line_three_split_samples[6][2] = {
    {110, 230},
    {117, 223},
    {124, 210},
    {140, 200},
    {143, 197},
    {147, 193}
};

std::list<float> oldAngles;


struct LineVectorData {
    double slope;
    double length;
    std::pair<double, double> closest_point;
    double distance;
};

ocLogger *logger;

static bool running = true;

static void signal_handler(int)
{
    running = false;
}

void addToOldAngleQueue(float val)
{
   oldAngles.push_front(val);
   if (oldAngles.size() > 3)
      oldAngles.pop_back();
}

float getAverageOfOldAngles()
{
   float val = 0;

   for(auto i = oldAngles.begin(); i != oldAngles.end(); ++i) {
    val += *i;
   }

   if(oldAngles.size() == 0) {
    return 0;
   }

   return val / oldAngles.size();
}

std::pair<double, double> linearRegression(std::vector<cv::Point> data) {
    double x_mean = 0;
    double y_mean = 0;

    for(int i = 0; i < data.size(); i++) {
        x_mean += data.at(i).x;
        y_mean += data.at(i).y;
    } 

    x_mean = x_mean / data.size();
    y_mean = y_mean / data.size();

    double numerator = 0;
    double denominator = 1;

    for(int i = 0; i < data.size(); i++) {
        numerator += (data.at(i).x - x_mean) * (data.at(i).y - y_mean);
        denominator += (data.at(i).y - y_mean) * (data.at(i).y - y_mean);
    } 

    double slope = numerator/denominator;

    double y_intercept = x_mean - slope * y_mean;  

    return {slope, y_intercept};
};

double calcDist(std::pair<double, double> p1, std::pair<double, double> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
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

    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Lines_Available:
                {
                    //Find Lane
                    //static struct ocBevLines lines;
                    //ipc_packet.read_from_start().read(&lines);

                    logger->log("test");

                    cv::Mat matrix = cv::Mat(400,400,CV_8UC1, shared_memory->bev_data->img_buffer);
                    int lane_mid_x_sum = 0;
                    int lane_mid_x_count = 0;

                    for(int y = 40; y <= 165; y+=25) {
                        const int *line_sample = line_samples[5 - (y-40)/25];
                        const int *line_three_sample = line_three_split_samples[5 - (y-40)/25];

                        std::vector<cv::Point> intersections;

                        for(int x = line_sample[0]; x < line_sample[1]; x++) {
                            int color = matrix.at<uint8_t>(400-y, x);

                            if(color == 255) { // White Point
                                intersections.push_back(cv::Point(x, 400-y));
                            }
                        }

                        if(intersections.size() == 0) {
                            continue;
                        }

                        int index = 1;
                        int sum = intersections.at(0).x;
                        int xOld = intersections.at(0).x;
                        

                        std::vector<cv::Point> new_intersections;

                        for(int i = 1; i < intersections.size(); i++) {
                            int xNew = intersections.at(i).x;
                            
                            if(xNew - xOld <= 16) {
                                sum += xNew;
                                xOld = xNew;
                                index++;
                            } else {
                                int x_cor = sum / index;
                                //logger->log("%d, %d, %d", x_cor, sum, index);
                                new_intersections.push_back(cv::Point(x_cor, 400-y));
                                xOld = xNew;
                                sum = xOld;
                                index = 1;
                            }
                        }

                        new_intersections.push_back(cv::Point(sum / index, 400-y));
                        cv::Point *left = nullptr;
                        cv::Point *right = nullptr;
                        cv::Point *mid = nullptr; 

                        for(int i = 0; i < new_intersections.size(); i++) {
                            cv::Point *point = &new_intersections.at(i);
                            if(point->x < line_three_sample[0]) {
                                left = point;
                            } else if(point->x > line_three_sample[1]) {
                                right = point;
                            } else {
                                mid = point;
                            }
                        }

                        int result;

                        if (mid != nullptr && right != nullptr) {
                            result = (mid->x + right->x)/2;
                        } else if (left != nullptr && right != nullptr) {
                            result = (left->x + 3*right->x)/4;
                        } else if (mid != nullptr) {
                            result = mid->x + 25;
                        } else if (left != nullptr) {
                            result = left->x + 75;
                        } else if (right != nullptr) {
                            result = right->x - 25;
                        } else {
                            continue;
                        }

                        cv::Point target = cv::Point(result, 400-y);

                        for(int i = 0; i < 6-(y-40)/25; i++) {
                            lane_mid_x_sum += target.x;
                            lane_mid_x_count++;
                        }

                        #ifdef DRAW_LINE_SAMPLES
                            cv::circle(matrix, target, 8, 255, 1);
                        #endif

                        #ifdef DRAW_LINE_SAMPLES
                            for(int i = 0; i < new_intersections.size(); i++) {
                                cv::Point point = new_intersections.at(i);
                                cv::circle(matrix, point, 8, 255, 1);
                            }
                        #endif
                    }

                    if(0 == lane_mid_x_count) {
                        continue;
                    }

                    double xDest = lane_mid_x_sum / lane_mid_x_count;
                    float xAngleFromMidline = 2*M_PI / (atan(48.5 / xDest) - M_PI) * 30 + 60;

                    addToOldAngleQueue(xAngleFromMidline);

                    double xStart = 200;
                    double yStart = 400;

                    #ifdef DRAW_LINE_SAMPLES
                        cv::line(matrix, cv::Point(xStart, yStart), cv::Point(xDest, 400-48.5), cv::Scalar(255,255,255,1), 8);
                    #endif

                    #ifdef DRAW_LINE_SAMPLES
                        for(int i = 40; i <= 165; i+=25) {
                            const int *line_sample = line_samples[5 - (i-40)/25];

                            cv::line(matrix, cv::Point(line_sample[0], 400-i), cv::Point(line_sample[1], 400-i), cv::Scalar(255,255,255,1), 2);
                        }
                    #endif

                   /* ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Lane_Found);
                    ipc_packet.clear_and_edit().write(xDest - 200);
                    socket->send_packet(ipc_packet);*/

                    float front_angle = 0;
                    float back_angle = getAverageOfOldAngles();

                    if (getAverageOfOldAngles() > 30) {
                        front_angle = (getAverageOfOldAngles() - 30);
                        back_angle = 30;
                    } else if (getAverageOfOldAngles() < -30) {
                        front_angle = (getAverageOfOldAngles() + 30);
                        back_angle = -30;
                    }

                    ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Start_Driving_Task);
                    ipc_packet.clear_and_edit()
                        .write<int16_t>(36)
                        .write<int8_t>(car_properties.rear_steering_angle_to_byte(front_angle))
                        .write<int8_t>(-car_properties.rear_steering_angle_to_byte(back_angle))
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