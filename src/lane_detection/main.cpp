#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include "../common/ocCar.h"
#include "../common/ocCarConfig.h"
#include <signal.h>
#include <vector>
#include <unistd.h>
#include <deque>
#include <numeric> // Für std::accumulate

#include "../common/ocWindow.h"
#include "./helper.cpp"
#include "./circle.cpp"

#include <opencv2/opencv.hpp>

#define CAR_CONFIG_FILE "../car_properties.conf"

#define DRAW_LINE_SAMPLES

//#define DEBUG

//#define DEBUG_WINDOW


ocMember member(ocMemberId::Lane_Detection_Values, "Lane Detection");

ocLogger *logger;
ocPacket ipc_packet;
ocIpcSocket *socket;
ocSharedMemory *shared_memory;
ocCarProperties car_properties;

Helper helper;
Circle circle;

std::deque<float> last_angles;

static bool running = true;
bool onStreet = true;
int onStreetCount = 0;

#define ANGLE_OFFSET_FRONT 10

#ifdef DEBUG
    #define ANGLE_OFFSET_FRONT 0 // Positive to right, negative to left
#endif

static void signal_handler(int)
{
    running = false;
}

void add_last_angle(float angle) {
    if (last_angles.size() > 10) {
        last_angles.pop_back(); // Remove the oldest angle (from the back)
    }
    last_angles.push_front(angle); // Add the new angle to the front
}

float average_angle() {
    if (last_angles.empty()) {
        return 0.0f; // Falls die Liste leer ist, gib 0 zurück
    }
    float sum = 0.0f;
    for (float angle : last_angles) {
        sum += angle;
    }
    return sum / last_angles.size();
}

bool check_if_on_street() { // TODO:
   return true;
}

void return_to_street(float front_angle) { //TODO: 
    if(!check_if_on_street()) {
        ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
        ipc_packet.clear_and_edit()
            .write<int16_t>(-30)
            .write<int8_t>(front_angle)
            .write<int8_t>(0);
        socket->send_packet(ipc_packet);    
    }
}

std::pair<int, int> drive_circle_in_angle(float angle) {
    angle *= 2;

    float front_angle = angle;
    float back_angle = 0;

    if(angle == 0) {
        front_angle = angle;
        back_angle = angle;
        return std::pair<int, int>(front_angle+ANGLE_OFFSET_FRONT, back_angle);
    }

    if(angle >= 65) {
        front_angle = 65;
        back_angle = -angle + 65;
    } else if(angle >= 15 && angle <= 30) {
        front_angle = angle;
        back_angle = 30 - angle;
    } else if(angle < 15 && angle > 0) {
        front_angle = angle;
        back_angle = angle;
    }
    
    if(angle < 0) {
        std::pair<int, int> pos_result = drive_circle_in_angle(-angle/2);

        front_angle = -pos_result.first;
        back_angle = -pos_result.second;
    }

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
                    cv::Mat camImageMatrix = cv::Mat(400,400,CV_8UC1, shared_memory->bev_data[0].img_buffer);
                    cv::Mat matrix;
                    cv::Mat matrix2;

                    camImageMatrix.copyTo(matrix);
                    matrix.copyTo(matrix2);

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::imwrite("bev.jpg", matrix);
                    } 

                    int radius;
                    cv::Point center;

                    std::tie(center, radius) = helper.calculate_radius(&matrix, &matrix2);

                    //auto [front_angle, back_angle] = drive_circle_in_angle(helper.map(radius));

                    float radius_in_cm = 0.6 * radius;

                    float height = 60.0;
                    float angle;

                    float direction = abs(radius_in_cm)/radius_in_cm;

                    //angle = std::asin(height / radius_in_cm) * (180/3.14);
                    if(abs(radius_in_cm) > 1000) {
                        angle = 0;
                    } else if(abs(radius_in_cm) > 500) {
                        angle = 20 * direction;
                    } else if(abs(radius_in_cm) > 200) {
                        angle = 40 * direction;
                    } else {
                        angle = 60;
                    }

                    ///add_last_angle(angle);

                    //angle = average_angle();
/*
                    float lowestY = 0;
                    float destX = 0;

                    for (double pi = 0; pi < 3.14; pi += 0.01) {
                        float x = center.x + std::cos(pi) * radius;
                        float y = center.y + std::sin(pi) * radius;

                        if(x > 400 || x < 0 || y > 400 || y < 0) {
                            continue;
                        }

                        if(y < lowestY) {
                            lowestY = y;
                            destX = x;
                        }
                    }

                    std::cout << "DESTX: " << destX;

                    cv::line(matrix2, cv::Point(400, lowestY), cv::Point(0, lowestY), cv::Scalar(100,0,255,0), 3);
                    cv::circle(matrix2, cv::Point(destX, lowestY), 5, cv::Scalar(0,0,255,0), 5);

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::imwrite("bev_lines.jpg", matrix2);
                    }

                    float angle = helper.map(destX);

                    auto [front_angle, back_angle] = drive_circle_in_angle(angle);
*/
                    int speed = (950/(abs(angle)+30))*5;//std::abs(radius) / 10;
                    speed = std::clamp(speed, 0, 60);


                    //logger->log("Radius in cm %f, ANGLE: %f, BANGLE: %f, DESTX: %d", radius_in_cm, front_angle, back_angle, destX);

                     logger->log("Radius in cm %f, ANGLE: %f", radius_in_cm, angle);

                    ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(speed)
                            .write<int8_t>(angle)
                            .write<int8_t>(-angle);
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