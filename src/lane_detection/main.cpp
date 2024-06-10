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

//#define DEBUG_WINDOW

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

    return std::pair(histogram, intersections);
}

int main()
{
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    ocMember member(ocMemberId::Lane_Detection_Values, "Lane Detection");
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

                    cv::imwrite("bev.jpg", matrix);
                   
                    auto histoIntersections = calcHistogram(&matrix);

                    std::array<int, 25> histogram = histoIntersections.first;
                    std::vector<cv::Point> intersections = histoIntersections.second;

                    std::vector<std::pair<int, int>> max;

                    for(int i = 1; i < histogram.size()-1; i++) {
                        if(histogram[i] > histogram[i+1] && histogram[i] > histogram[i-1] ) {
                            max.push_back(std::pair<int, int>(i, histogram[i]));
                        } 
                    }

                    // Histogram evaluation
                    std::sort(max.begin(), max.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                        return a.second > b.second;
                    });

                    std::vector<std::pair<int, int>> greatest_values(max.begin(), max.begin() + (max.size() >= 3 ? 3 : max.size()));

                    std::sort(greatest_values.begin(), greatest_values.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                        return a.first < b.first;
                    });

                    std::vector<cv::Point> leftVec;
                    std::vector<cv::Point> midVec;
                    std::vector<cv::Point> rightVec;

                    // Retrieve lanes by maximas of histogram
                    for(int i = 0; i < intersections.size(); i++) {
                        int x = intersections.at(i).x/16;

                        if(greatest_values.size() == 1) {
                            if(greatest_values.at(0).first == x || greatest_values.at(0).first == x-1) {
                                rightVec.push_back(intersections.at(i));
                            } 
                        } else if(greatest_values.size() == 2) {
                            if(greatest_values.at(0).first == x) {
                                midVec.push_back(intersections.at(i));
                            } else if(greatest_values.at(1).first == x || greatest_values.at(1).first == x-1) {
                                rightVec.push_back(intersections.at(i));
                            } 
                        } else if(greatest_values.size() >= 3) {
                            if(greatest_values.at(0).first == x || greatest_values.at(0).first == x+1) {
                                leftVec.push_back(intersections.at(i));
                            } else if(greatest_values.at(1).first == x) {
                                midVec.push_back(intersections.at(i));
                            } else if(greatest_values.at(2).first == x || greatest_values.at(2).first == x-1) {
                                rightVec.push_back(intersections.at(i));
                            } 
                        }
                    }

                    int right = 0;
                    int mid = 0;

                    // Retrieve right and mid x coordinates to calculate middle of lane
                    if(rightVec.size() != 0) {
                        for(const auto& i : rightVec) {
                            right += i.x;
                        }

                        right /= rightVec.size();
                    }

                    if(midVec.size() != 0) {
                        for(const auto& i : midVec) {
                            mid += i.x;
                        }

                        mid /= midVec.size();
                    }

                    int distance_to_horizontal = 0;
                    int count = 0;

                    for(int radius = 50; radius <= 200; radius+=25) {
                        int pointCount = 0;
                        for(double pi = 0; pi < M_PI; pi += 0.001) {
                            int x = 200 + round(cos(pi) * radius);
                            int y = 400 - round(sin(pi) * radius);

                            if(x >= 400 || x < 0 || y >= 400 || y < 0) {
                                continue;
                            }

                            int color = matrix.at<uint8_t>(y, x);

                            if(color > 50 && x > (mid + 10) && x < (right - 10)) {
                                pointCount++;
                            }
                        }

                        //logger->log("%d", pointCount);
                        if (pointCount > 10) {
                            distance_to_horizontal += radius;
                            count ++;
                        }

                        if (count > 0) {
                            distance_to_horizontal /= count;
                        }   
                    }

                    //logger->log("%d", distance_to_horizontal);
                    int dest = (right + mid) / 2;

                    if(mid == 0) { // Keep left from right lane with a margin of 10 when no mid line found
                        dest = right -10;
                    }

                    if(mid == 0 && right == 0) { // Move straight when nothing seen
                        dest = 200;
                    }

                    if(dest > 399) { // Fix destination out of window
                        dest = 399;
                    } else if(dest < 0) {
                        dest = 0;
                    }

                    float angle = (dest - 200) * 2.54; // MAPPING TO INT 8 -255 to 254 for angle

                    angle = std::clamp((int) angle, -170, 170); // Clamp between -200 and 200 so tire doesn't get stuck due to too high angle

                    float speed = 90 * (100 / (std::abs(angle) + 100));

            #ifdef DEBUG_WINDOW
                    speed = 20 * (254 / (std::abs(angle) + 254));

                    for(int radius = 50; radius <= 200; radius+=25) {
                        cv::circle(matrix, cv::Point(200,400), radius, cv::Scalar(255,255,255,1), 5);
                    }

                    for(const auto& i : intersections) {
                        cv::circle(matrix, i, 5, cv::Scalar(255,255,255,1), 5);
                    }

                    cv::line(matrix, cv::Point(200, 400), cv::Point(dest, 300), cv::Scalar(255,255,255,1));
                    cv::imshow("Lane Detection", matrix);
                    char key = cv::waitKey(30);
                    if (key == 'q')
                    {
                        cv::destroyAllWindows();
                        return 0;
                    }
            #endif

                    ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                    ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                    ipc_packet.clear_and_edit()
                        .write<int16_t>(speed)
                        .write<int8_t>(angle); 
                        //.write<int8_t>(-angle/2);
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