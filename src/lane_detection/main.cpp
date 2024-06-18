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


typedef std::array<int, 25> histogram_t;
typedef std::vector<cv::Point> points_vector_t;
typedef std::pair<histogram_t, points_vector_t> complete_histdata_t;

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

int ANGLE_OFFSET_FRONT = 21; // Positive to right, negative to left

static void signal_handler(int)
{
    running = false;
}

double radius_to_angle(float radius) {
    if(radius > 0) {
        return 3000000.0 * std::pow(radius, -3) + 10;
    } else if(radius < 0) {
        return 2700000.0 * std::pow(radius, -3) - 10;
    } else {
        return 0;
    }
}

double radius_to_angle(float radius) {
    if(radius > 0) {
        return 3000000.0 * std::pow(radius, -3) + 10;
    } else if(radius < 0) {
        return 2700000.0 * std::pow(radius, -3) - 10;
    } else {
        return 0;
    }
}

double calc_dist(std::pair<double, double> p1, std::pair<double, double> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
}

bool is_lane_dist(int x1, int x2) {
    return std::abs(x1 - x2) > 45 && std::abs(x1 - x2) < 70; 
}

bool check_if_on_street(std::array<int, 25> histogram_unten) {
    for(auto& bin : histogram_unten) {
        if(bin > 0) {
            return true;
        }
    }

    onStreet = false;

    return false;
}

int get_angle(int dest) {
    float angle = 0;

    int val1 = 50;
    int val2 = 2;
    double val3 = 0.001;

    if((dest - 200) > val1) {
        angle = (float) (dest - 200 - val1)*(float) (dest - 200 - val1)*(float) ((dest - 200 - val1) * val3) + (float) val1;
    } else if ((dest - 200) < -val1) {
        angle = (float) (dest - 200 + val1)*(float) (dest - 200 + val1)*(float) ((dest - 200 + val1) * val3) - (float) val1;
    } else {
        angle = (float) ((dest - 200) * val2);
    }
    //logger->log("%d", (dest - 200));

    return std::clamp((const int) angle, -65, 65); // Clamp between -65 and 65 so tire doesn't get stuck due to too high angle
}

float get_radius(cv::Point p1, cv::Point p2) {
    cv::Point origin = cv::Point(200, 400);

    // Calculate the lengths of the sides of the triangle
    float a = cv::norm(p1 - origin); // distance from origin to p1
    float b = cv::norm(p2 - origin); // distance from origin to p2
    float c = cv::norm(p2 - p1);     // distance from p1 to p2

    // Use the cosine rule to calculate the angles
    // cos(angle) = (b^2 + c^2 - a^2) / (2 * b * c)
    float angle_o_p1 = acos((b*b + c*c - a*a) / (2 * b * c));
    // cos(angle) = (a^2 + c^2 - b^2) / (2 * a * c)
    float angle_p1_p2 = acos((a*a + c*c - b*b) / (2 * a * c));
    // cos(angle) = (a^2 + b^2 - c^2) / (2 * a * b)
    float angle_o_p2 = acos((a*a + b*b - c*c) / (2 * a * b));

    
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

void return_to_street(float front_angle, std::array<int, 25> histogram_unten) {
    if(!check_if_on_street(histogram_unten)) {
        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
        ipc_packet.clear_and_edit()
            .write<int16_t>(-30)
            .write<int8_t>(front_angle)
            .write<int8_t>(0);
        socket->send_packet(ipc_packet);    
    }
}

void continue_if_blinded(float front_angle, float back_angle, std::array<int, 25> histogram_unten) {

}



std::pair<int, int> get_angles_from_average_angle(float average_angle) {
    average_angle *= 2;

    float front_angle = average_angle;
    float back_angle = 0;

    if(average_angle == 0) {
        front_angle = average_angle;
        back_angle = average_angle;
        return std::pair<int, int>(front_angle+ANGLE_OFFSET_FRONT, back_angle);
    }

    if(average_angle >= 65) {
        front_angle = 65;
        back_angle = -average_angle + 65;
    } else if(average_angle >= 15 && average_angle <= 30) {
        front_angle = average_angle;
        back_angle = 30 - average_angle;
    } else if(average_angle < 15 && average_angle > 0) {
        front_angle = average_angle;
        back_angle = average_angle;
    }
    
    if(average_angle < 0) {
        std::pair<int, int> pos_result = get_angles_from_average_angle(-average_angle/2);

        front_angle = -pos_result.first;
        back_angle = -pos_result.second;
    }

    //logger->log("avg: %f  --  fr: %f  --  ba: %f", average_angle, front_angle, back_angle);
    return std::pair<int, int>(front_angle+ANGLE_OFFSET_FRONT, back_angle);
}

std::pair<int, int> drive_parallel_in_angle(float angle) {
    if(angle >= 65) {
        return std::pair(65,65);
    }
    if(angle <= -65) {
        return std::pair(-65,-65);
    }

    return std::pair(angle, angle);
}

cv::Point find_friend(cv::Point point, float degree) {
    
}

std::tuple<int, int, int> vectors_to_coordinates(std::vector<cv::Point> left_vec, std::vector<cv::Point> mid_vec, std::vector<cv::Point> right_vec) {
    int right = 0;
    int mid = 0;
    int left = 0;

    // Retrieve right and mid x coordinates to calculate middle of lane
    if(right_vec.size() != 0) {
        for(const auto& i : right_vec) {
            right += i.x;
        }

        right /= right_vec.size();
    }

    if(mid_vec.size() != 0) {
        for(const auto& i : mid_vec) {
            mid += i.x;
        }

        mid /= mid_vec.size();
    }


    if(left_vec.size() != 0) {
        for(const auto& i : left_vec) {
            left += i.x;
        }

        left /= left_vec.size();
    }

    return std::tuple<int, int, int>{left,mid,right};
}

std::pair<points_vector_t, points_vector_t> calc_destinations(cv::Mat *matrix) {
    points_vector_t destinations;
    points_vector_t intersections;

    for(int radius = 50; radius <= 225; radius+=25) {

        points_vector_t points_per_circle;

        std::pair<int, int> point[2];

        for(double pi = 0; pi < 3.14; pi += 0.001) {
            int x = 200 + round(cos(pi) * radius);
            int y = 400 - round(sin(pi) * radius);

            int x2 = 200 + round(cos(pi+0.01) * radius);
            int y2 = 400 - round(sin(pi+0.01) * radius);

            int x3 = 200 + round(cos(pi+0.03) * radius);
            int y3 = 400 - round(sin(pi+0.03) * radius);

            if ((400-y) < (-1/2*x + 80) || (400-y) < (1/2*x - 120)) {
                continue;
            }

            if(x+1 >= 400 || x-1 < 0 || y+1 >= 400 || y-1 < 0) {
                continue;
            }

            if(x2+1 >= 400 || x2-1 < 0 || y2+1 >= 400 || y2-1 < 0) {
                continue;
            }

            if(x3+1 >= 400 || x3-1 < 0 || y3+1 >= 400 || y3-1 < 0) {
                continue;
            }

            int color = matrix->at<uint8_t>(y, x);
            int color2 = matrix->at<uint8_t>(y2, x2);
            int color3 = matrix->at<uint8_t>(y3, x3);

            if(color > 235) {
                continue;
            }

            if(color2 - color > 10 && color3 - color > 10) {
                point[0] = std::pair(x,y);
            }

            if(color - color2 > 10 && point[0].first != 0 && point[0].second != 0 && color - color3 > 10) {
                point[1] = std::pair(x,y);

                double dist = calc_dist(point[0], point[1]);

                if(dist <= 10) {
                    points_per_circle.push_back(cv::Point(point[0].first,point[0].second));

                    intersections.push_back(cv::Point(point[0].first,point[0].second));

                    point[0] = std::pair(0,0);
                    point[1] = std::pair(0,0);
                }
            }
        }

        //get_depth_of_line(point, start_degree) --> return length of points following this point


        //points_per_circle auslesen und anzahl und position prüfen --> ziel ausrechnen, ziel liegt auf kreis
        cv::Point dest;
        if(points_per_circle.size() == 1) {
            dest = points_per_circle.at(0);
            double pi = 0;
            while(calc_dist(std::pair(points_per_circle.at(0).x, points_per_circle.at(0).y), std::pair(dest.x, dest.y)) < 25) {
                int x = 200 + round(cos(pi) * radius);
                int y = 400 - round(sin(pi) * radius);

                if(x > dest.x) {
                    dest = cv::Point(x,y);
                    continue;
                }

                pi += 0.0001;

                if(pi > 3.14) {
                    break;
                }
            }
        } else if(points_per_circle.size() >= 2) {
            double dist = calc_dist(std::pair(points_per_circle.at(0).x, points_per_circle.at(0).y), std::pair(points_per_circle.at(1).x, points_per_circle.at(1).y));
            if (dist > 30 && dist < 70) {
                dest = cv::Point((points_per_circle.at(0).x + points_per_circle.at(1).x)/2, (points_per_circle.at(0).y + points_per_circle.at(1).y)/2);
            } else if (dist >= 70) {
                dest = cv::Point((points_per_circle.at(0).x*3 + points_per_circle.at(1).x)/4, (points_per_circle.at(0).y*3 + points_per_circle.at(1).y)/4);
            } else {
                dest = points_per_circle.at(0);
                
                double pi = 0;

                while(calc_dist(std::pair(points_per_circle.at(0).x, points_per_circle.at(0).y), std::pair(dest.x, dest.y)) < 25) {
                    int x = 200 + round(cos(pi) * radius);
                    int y = 400 - round(sin(pi) * radius);

                    if(x > dest.x) {
                        dest = cv::Point(x,y);
                        continue;
                    }

                    pi += 0.0001;

                    if(pi > 3.14) {
                        break;
                    }
                }
            }
        }
        destinations.push_back(dest);
    }

    return std::pair<points_vector_t, points_vector_t>(destinations, intersections);
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
                   
                    auto calced_destinations = calc_destinations(&matrix);

                    points_vector_t destinations = calced_destinations.first;
                    points_vector_t intersections = calced_destinations.second;

                    std::string line = "";

                    logger->log("%s", line.c_str());

                    std::vector<std::pair<int, int>> max;

                    int dest = 10; //TODO

                    int angle = get_angle(dest);

                    last_angles.push_back(angle);

                    int average_angle = 0;

                    if(last_angles.size() > 3) {
                        last_angles.pop_front();

                        for(auto& i : last_angles) {
                            average_angle += i;
                            average_angle += i;
                        }
                        average_angle /= 3;
                    }
                    
                    int speed = int(75 * float(100.0f / float(std::abs(float(average_angle)) + 100.0f)));

                    #ifdef DEBUG_WINDOW
                        int distance_to_horizontal = 0;
                        int count = 0;

                        for(int radius = 50; radius <= 200; radius+=25) {
                            int point_count = 0;
                            for(double pi = 0; pi < M_PI; pi += 0.001) {
                                int x = 200 + round(cos(pi) * radius);
                                int y = 400 - round(sin(pi) * radius);

                                if(x >= 400 || x < 0 || y >= 400 || y < 0) {
                                    continue;
                                }

                                int color = matrix.at<uint8_t>(y, x);

                                if(color > 50 && x > (mid + 10) && x < (right - 10)) {
                                    point_count++;
                                }
                            }

                            //logger->log("%d", point_count);
                            if (point_count > 1) {
                                distance_to_horizontal += radius;
                                count ++;
                            }

                            if (count > 0) {
                                distance_to_horizontal /= count;
                            }   
                        }
                        speed = 10;

                        for(int radius = 50; radius <= 200; radius+=25) {
                            cv::circle(matrix, cv::Point(200,400), radius, cv::Scalar(255,255,255,1), 5);
                        }

                        for(const auto& i : intersections_unten) {
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
                        for(int radius = 50; radius <= 225; radius+=25) {
                            cv::circle(matrix, cv::Point(200,400), radius, cv::Scalar(255,255,255,1), 5);
                        }

                        for(const auto& i : destinations) {
                            cv::circle(matrix, i, 5, cv::Scalar(255,255,255,1), 5);
                        }

                        for(const auto& i : intersections) {
                            cv::circle(matrix, i, 5, cv::Scalar(255,255,255,1), 5);
                        }

                        cv::line(matrix, cv::Point(200, 400), cv::Point(dest, 300), cv::Scalar(255,255,255,1));

                        if(std::getenv("CAR_ENV") != NULL) {
                            cv::imwrite("bev_lines.jpg", matrix);
                        }
                    #endif

                    auto [front_angle, back_angle] = get_angles_from_average_angle(average_angle);

                    ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(30)
                            .write<int8_t>(front_angle)
                            .write<int8_t>(back_angle);
                        socket->send_packet(ipc_packet);

                /*

                    if((check_if_on_street(histogram_unten) && onStreet)) {
                        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(30)
                            .write<int8_t>(front_angle)
                            .write<int8_t>(back_angle);
                        socket->send_packet(ipc_packet);
                    } else if(check_if_on_street(histogram_unten) && !onStreet) {
                        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(-30)
                            .write<int8_t>(-front_angle)
                            .write<int8_t>(0); 
                        socket->send_packet(ipc_packet);
                        
                        if(onStreetCount > 15) {
                            onStreet = true;
                            onStreetCount = 0;
                        } else {
                            onStreetCount++;
                        }
                        
                    } else if (!onStreet){
                        return_to_street(front_angle, histogram_unten);
                    }*/
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