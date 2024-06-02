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

static void signal_handler(int)
{
    running = false;
}

double calcDist(std::pair<double, double> p1, std::pair<double, double> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
}

std::pair<std::array<int, 25>, std::vector<cv::Point>> calcHistogram(cv::Mat *matrix) {
    std::array<int, 25> histogram;
    std::vector<cv::Point> intersections;

    std::fill(histogram.begin(), histogram.end(), 0);

    for(int radius = 50; radius <= 200; radius+=25) {
        for(double pi = 0; pi < M_PI; pi += 0.001) {
            int x = 200 + round(cos(pi) * radius);
            int y = 400 - round(sin(pi) * radius);

            if(x >= 400 || x < 0 || y >= 400 || y < 0) {
                continue;
            }

            int color = matrix->at<uint8_t>(y, x);

            if(color > 50) {
                intersections.push_back(cv::Point(x,y));
                histogram[x/16]++;
            }
        }
    }

    for(int radius = 50; radius <= 200; radius+=25) {
        cv::circle(*matrix, cv::Point(200,400), radius, cv::Scalar(255,255,255,1), 5);
    }

    for(const auto& i : intersections) {
        cv::circle(*matrix, i, 5, cv::Scalar(255,255,255,1), 5);
    }

    std::string histo = "";

    for(int i = 0; i < 25; i++) {
        histo += std::to_string(histogram[i]);
        histo += "\t";
    }

    return std::pair(histogram, intersections);
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
                    cv::Mat matrix = cv::Mat(400,400,CV_8UC1, shared_memory->bev_data[1].img_buffer);
                   
                    std::array<int, 25> histogram = calcHistogram(&matrix).first;
                    std::vector<cv::Point> intersections = calcHistogram(&matrix).second;

                    std::vector<std::pair<int, int>> max;

                    for(int i = 1; i < histogram.size()-1; i++) {
                        if(histogram[i] > histogram[i+1] && histogram[i] > histogram[i-1] ) {
                            max.push_back(std::pair<int, int>(i, histogram[i]));
                        } 
                    }

                    std::sort(max.begin(), max.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                        return a.second > b.second;
                    });

                    std::vector<std::pair<int, int>> greatest_values(max.begin(), max.begin() + 3);

                    std::sort(greatest_values.begin(), greatest_values.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                        return a.first < b.first;
                    });

                    std::vector<cv::Point> leftVec;
                    std::vector<cv::Point> midVec;
                    std::vector<cv::Point> rightVec;

                    for(int i = 0; i < intersections.size(); i++) {
                        int x = intersections.at(i).x/16;

                        if(greatest_values.at(0).first == x) {
                            leftVec.push_back(intersections.at(i));
                        } else if(greatest_values.at(1).first == x) {
                            midVec.push_back(intersections.at(i));
                        } else if(greatest_values.at(2).first == x) {
                            rightVec.push_back(intersections.at(i));
                        }
                    }

                    int right = 0;
                    int mid = 0;

                    for(const auto& i : rightVec) {
                        right += i.x;
                    }

                    for(const auto& i : midVec) {
                        mid += i.x;
                    }

                    right /= rightVec.size();
                    mid /= midVec.size();



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
                        .write<int16_t>(20)
                        .write<int8_t>(0)
                        .write<int8_t>(0)
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