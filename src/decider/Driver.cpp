#include <cstdint>
#include <chrono>
#include "Driver.h"
#include "../common/ocCar.h"



/**
 * This method is used to initialize the Driver, mainly the communication with the IPC-Hub.
*/
void Driver::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.clear_and_edit()
            .write(ocMessageId::Lane_Detection_Values);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



/**
 * This method is used to perform a right-turn.
*/
void Driver::turn_right(){
    //drive(25, 0);
    //wait(0.2);

    drive_both_steering_values(25, 100, -50);
    wait(1.85);

    //drive(20, 0);
    //wait(1);
    stop();
}



/**
 * This method is used to perform a left-turn.
*/
void Driver::turn_left(){
    drive(25, 0);
    wait(0.5);

    drive(25, -100);
    wait(3.25);

    //drive(20, 0);
    //wait(1);
    stop();
}



/**
 * This method is called to drive forward.
 * The speed and steering values are received from the lane-detection using the IPC-Hub.
*/
void Driver::drive_forward(){
    ocPacket recv_packet;

    int result = socket->read_packet(recv_packet);

    if (result < 0){
      logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
    } else {
        switch (recv_packet.get_message_id()){
            case ocMessageId::Lane_Detection_Values:{
                auto reader = recv_packet.read_from_start();

                int16_t speed = reader.read<int16_t>();
                int8_t steering_front = reader.read<int8_t>();
                //int8_t steering_back = reader.read<int8_t>();
                

                struct start_driving_task_t start_driving_task = {
                    .speed          = speed,
                    .steering_front = steering_front,
                    .steering_rear  = 0,//steering_back,
                    .id             = 1,
                    .steps_ab       = 10
                };

                int32_t send_result = socket->send(ocMessageId::Start_Driving_Task, start_driving_task);
                logger->log("Result of sending driving task: %d", send_result);
            } break;

            default:{
                ocMessageId msg_id = recv_packet.get_message_id();
                ocMemberId  mbr_id = recv_packet.get_sender();
                logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
            } break;
        }
    }
}



/**
 * This method is used to drive using the given speed and steering values.
 * @param speed int16_t: The speed with which to drive
 * @param steering int8_t: The steering value with which to drive
*/
void Driver::drive(int16_t speed, int8_t steering){
    ocCarProperties ocCarProperties;

    struct start_driving_task_t start_driving_task = {
        .speed          = speed,
        .steering_front = steering,
        .steering_rear  = 0,
        .id             = 1,
        .steps_ab       = 0
    };

    int32_t send_result = socket->send(ocMessageId::Start_Driving_Task, start_driving_task);
    logger->log("Result of sending driving task: %d", send_result);
}



/**
 * This method is used to drive using the given speed and steering values.
 * @param speed int16_t: The speed with which to drive
 * @param steering int8_t: The steering value with which to drive
*/
void Driver::drive_both_steering_values(int16_t speed, int8_t steering_front, int8_t steering_back){
    ocCarProperties ocCarProperties;

    struct start_driving_task_t start_driving_task = {
        .speed          = speed,
        .steering_front = steering_front,
        .steering_rear  = steering_back,
        .id             = 1,
        .steps_ab       = 0
    };

    int32_t send_result = socket->send(ocMessageId::Start_Driving_Task, start_driving_task);
    logger->log("Result of sending driving task: %d", send_result);
}



/**
 * This method is used to stop the car for the given duration.
 * This method is blocking!
 * @param duration float: The duration for which to stop
*/
void Driver::stop(float duration){

    struct start_driving_task_t start_driving_task = {
        .speed          = 0,
        .steering_front = 0,
        .steering_rear  = 0,
        .id             = 1,
        .steps_ab       = 0
    };

    int32_t send_result = socket->send(ocMessageId::Start_Driving_Task, start_driving_task);
    logger->log("Result of sending stop task: %d", send_result);

    wait(duration);
}



/**
 * This method is used to park the car. It is assumed that the method is called, when the car is located closely infront of a parking sign.
 * The car will proceed to first drive forward some distance and then reverse-park in an area to the right of the street.
*/
void Driver::park(){
    /*Drive forward a little bit*/
    drive(25, 0);
    wait(6.5);
    stop();

    /*Drive backward to the right*/
    drive(-20, -100);
    wait(2);
    drive(-20, 0);
    wait(1.25);

    /*Drive backward to the left*/
    drive(-20, 100);
    wait(1);
    stop();

    /*Drive forward to the right*/
    drive(20, 100);
    wait(1);
    drive(20, 0);
    wait(0.9);

    stop();
}



/**
 * This method is used to park out, after using the park()-method.
*/
void Driver::park_out(){
    /*Drive forward to the left*/
    drive(25, -100);
    wait(1.5);

    /*Drive foward a little bit*/
    drive(25, 0);
    wait(1);
    
    /*Drive forward to the right*/
    drive(25, 100);
    wait(1.15);

    stop();
}



/**
 * This method is used to wait for a given duration.
 * This method is blocking!
 * @param duration float: The duration for which to wait
*/
void Driver::wait(float duration){
    auto start = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds;

    do {
        auto now = std::chrono::system_clock::now();
        elapsed_seconds = now - start;
    } while (elapsed_seconds.count() < duration);
}
