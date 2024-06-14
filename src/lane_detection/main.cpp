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

ocMember member(ocMemberId::Lane_Detection_Values, "Lane Detection");

ocLogger *logger;
ocPacket ipc_packet;
ocIpcSocket *socket;
ocSharedMemory *shared_memory;
ocCarProperties car_properties;

std::list<int> last_angles; 

static bool running = true;
bool onStreet = true;
int onStreetCount = 0;

static void signal_handler(int)
{
    running = false;
}

double calc_dist(std::pair<double, double> p1, std::pair<double, double> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
}

bool is_lane_dist(int x1, int x2) {
    return std::abs(x1 - x2) > 40 && std::abs(x1 - x2) < 75; 
}

bool check_if_on_street(std::array<int, 25> histogram) {
    for(auto& bin : histogram) {
        if(bin > 20) {
            return true;
        }
    }

    onStreet = false;

    return false;
}

float get_angle(int dest) {
    float angle = 0;
    if(std::abs((dest - 200)) > 30) {
        angle = (dest - 200)*(dest - 200)*(dest - 200) * 0.32; // MAPPING TO INT 8 -65 and 65 for angle
    } else {
        angle = (dest - 200) * 2;
    }
    logger->log("%f", angle);

    return std::clamp((int) angle, -65, 65); // Clamp between -65 and 65 so tire doesn't get stuck due to too high angle
}

int get_dest(int mid, int right) {
    int dest = (right + mid) / 2;

    if(mid == 0) { // Keep left from right lane with a margin of 10 when no mid line found
        dest = right - 20;
    }

    if(mid == 0 && right == 0) { // Move straight when nothing seen
        dest = 200;
    }

    if(dest > 399) { // Fix destination out of window
        dest = 399;
    } else if(dest < 0) {
        dest = 0;
    }

    return dest;
}

void return_to_street(int angle, std::array<int, 25> histogram) {


    float front_angle = angle;
    float back_angle = 0;
    if(angle > 75) {
        front_angle = 75;
        back_angle = angle - 75;
    }
    if(angle < -75) {
        front_angle = -75;
        back_angle = angle + 75;
    } 

    if(!check_if_on_street(histogram)) {
        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
        ipc_packet.clear_and_edit()
            .write<int16_t>(-30)
            .write<int8_t>(front_angle)
            .write<int8_t>(-back_angle);
        socket->send_packet(ipc_packet);    
    }//while no lane is detected
    // do drive backward negative average angle
    
}

std::pair<std::array<int, 25>, std::vector<cv::Point>> calcHistogram(cv::Mat *matrix) {
    std::array<int, 25> histogram;
    std::vector<cv::Point> intersections;

    std::fill(histogram.begin(), histogram.end(), 0);

    for(int radius = 50; radius <= 200; radius+=25) {
        std::pair<int, int> point[2];

        for(double pi = 0; pi < M_PI; pi += 0.001) {
            int x = 200 + round(cos(pi) * radius);
            int y = 400 - round(sin(pi) * radius);

            int x2 = 200 + round(cos(pi+0.01) * radius);
            int y2 = 400 - round(sin(pi+0.01) * radius);

            if(x+1 >= 400 || x-1 < 0 || y+1 >= 400 || y-1 < 0) {
                continue;
            }

            if(x2+1 >= 400 || x2-1 < 0 || y2+1 >= 400 || y2-1 < 0) {
                continue;
            }

            int color = matrix->at<uint8_t>(y, x);
            int color2 = matrix->at<uint8_t>(y2, x2);

            if(color2 - color > 25 && point[0].first != 0) {
                point[0] = std::pair(x,y);
            }

            if(color - color2 > 25) {
                point[1] = std::pair(x,y);

                //double dist = calcDist(point[0], point[1]);

                intersections.push_back(cv::Point(point[0].first,point[0].second));
                intersections.push_back(cv::Point(point[1].first,point[1].second));

                point[0] = std::pair(0,0);
                point[1] = std::pair(0,0);
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

    member.attach();

    socket = member.get_socket();
    shared_memory = member.get_shared_memory();
    logger = member.get_logger();

    read_config_file(CAR_CONFIG_FILE, car_properties, *logger);

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
                    cv::Mat matrix = cv::Mat(400,400,CV_8UC1, shared_memory->bev_data[0].img_buffer);

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::imwrite("bev.jpg", matrix);
                    } 
                   
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

                    int average_angle = 0;
                    if(last_angles.size() > 3) {
                        last_angles.pop_front();

                        for(auto& i : last_angles) {
                            average_angle += i;
                        }
                        average_angle /= 3;
                    }

                    int right = 0;
                    int mid = 0;
                    int left = 0;

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


                    if(leftVec.size() != 0) {
                        for(const auto& i : leftVec) {
                            left += i.x;
                        }

                        left /= leftVec.size();
                    }

                    /*if(!is_lane_dist(right, mid) && std::abs(average_angle) < 10) {
                        right = mid;
                        mid = left;

                        for(int i = 0; i < last_angles.size(); i++) {
                            last_angles.pop_front();
                            last_angles.push_back(get_dest(mid, right));
                        }

                        if(last_angles.size() > 3) {
                            for(auto& i : last_angles) {
                                average_angle += i;
                            }
                            average_angle /= 3;
                        }
                    }*/

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
                    int dest = get_dest(mid, right);

                    float angle = get_angle(dest);

                    last_angles.push_back(angle);
                    
                    int speed = int(50 * float(100.0f / float(std::abs(float(average_angle)) + 100.0f)));

                    #ifdef DEBUG_WINDOW
                        speed = 10;

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
                    #else
                        for(int radius = 50; radius <= 200; radius+=25) {
                            cv::circle(matrix, cv::Point(200,400), radius, cv::Scalar(255,255,255,1), 5);
                        }

                        for(const auto& i : intersections) {
                            cv::circle(matrix, i, 5, cv::Scalar(255,255,255,1), 5);
                        }

                        for(const auto& p : rightVec) {
                            cv::rectangle(matrix, p-cv::Point(5,5), p+cv::Point(5,5), 5);
                        }

                        for(const auto& p : midVec) {
                            cv::rectangle(matrix, p-cv::Point(5,5), p+cv::Point(5,5), 5); 
                        }

                        for(const auto& p : leftVec) {
                        cv::rectangle(matrix, p-cv::Point(5,5), p+cv::Point(5,5), 5);    
                        }

                        cv::line(matrix, cv::Point(200, 400), cv::Point(dest, 300), cv::Scalar(255,255,255,1));

                        if(std::getenv("CAR_ENV") != NULL) {
                            cv::imwrite("bev_lines.jpg", matrix);
                        }
                    #endif

                    float front_angle = average_angle;
                    float back_angle = 0;
                    if(average_angle >= 60) {
                        front_angle = 60;
                        back_angle = average_angle - 60;
                    }
                    if(average_angle <= -60) {
                        front_angle = -60;
                        back_angle = average_angle + 60;
                    }


                    if(check_if_on_street(histogram) && onStreet) {
                        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(speed)
                            .write<int8_t>(front_angle)
                            .write<int8_t>(-back_angle);
                        socket->send_packet(ipc_packet);
                    } else if(check_if_on_street(histogram) && !onStreet) {
                        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(-30)
                            .write<int8_t>(0)
                            .write<int8_t>(0); 
                        socket->send_packet(ipc_packet);
                        
                        if(onStreetCount > 5) {
                            onStreet = true;
                            onStreetCount = 0;
                        } else {
                            onStreetCount++;
                        }
                        
                    } else if (!onStreet){
                        return_to_street(average_angle, histogram);
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
}