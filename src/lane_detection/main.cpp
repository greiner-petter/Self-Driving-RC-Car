#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include "../common/ocCar.h"
#include "../common/ocCarConfig.h"
#include <signal.h>
#include <vector>
#include <unistd.h>

#include "../common/ocWindow.h"
#include "./helper.cpp"

#include <opencv2/opencv.hpp>

#define CAR_CONFIG_FILE "../car_properties.conf"

#define DRAW_LINE_SAMPLES

//#define DEBUG

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

Helper helper;

std::list<int> last_angles; 

static bool running = true;
bool onStreet = true;
int onStreetCount = 0;

#define ANGLE_OFFSET_FRONT 21

#ifdef DEBUG
    #define ANGLE_OFFSET_FRONT 0 // Positive to right, negative to left
#endif

static void signal_handler(int)
{
    running = false;
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

std::pair<int, int> drive_circle_in_angle(float average_angle) {
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
        std::pair<int, int> pos_result = drive_circle_in_angle(-average_angle/2);

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

                    int radius = helper.calculate_radius(&matrix);
                   
                    int speed = std::abs(radius) / 10;
                    if(speed < 10) {
                        speed = 10;
                    }

                    speed = std::clamp(speed, 0, 30);

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::imwrite("bev_lines.jpg", matrix);
                    }

                    logger->log("%d", radius);

                    auto [front_angle, back_angle] = drive_circle_in_angle(helper.radius_to_angle(radius));

                    ipc_packet.set_sender(ocMemberId::Lane_Detection_Values);
                        ipc_packet.set_message_id(ocMessageId::Lane_Detection_Values);
                        ipc_packet.clear_and_edit()
                            .write<int16_t>(speed)
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