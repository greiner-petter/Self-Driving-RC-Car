#include <cstdint>
#include <chrono>
#include "Driver.h"
#include "../common/ocCar.h"


void Driver::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.clear_and_edit()
            .write(ocMessageId::Driving_Task_Finished);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



int16_t calculate_speed(){

    /*
    int current_error
    int previous_error;

    int previous_error = error;
    int error = target_speed - current_speed;
    error_sum += error;

    double result = (KP * error) + (KI * SAMPLING_TIME * error_sum) + ((KD / SAMPLING_TIME) * (error - previous_error));
    duty_cycle += result;

    if(duty_cycle > MAX_DUTY_CYCLE){

    	if(result >= 0){
    		duty_cycle = MAX_DUTY_CYCLE;
    	}else{
    		duty_cycle = MIN_DUTY_CYCLE;
    	}

    } else if(duty_cycle < MIN_DUTY_CYCLE) {

    	if(result < 0){
    		duty_cycle = MIN_DUTY_CYCLE;
    	} else{
    		duty_cycle = MAX_DUTY_CYCLE;
    	}
    }
    */
    return 0;
}



int8_t calculate_steering_front(){
    return 0;
}



int8_t calculate_steering_rear(){
    return 0;
}






void Driver::turn_right(){
    drive(25, 0);
    wait(0.2);

    drive(25, 100);
    wait(3.2);

    drive(20, 0);
    wait(1);
    stop();
}



void Driver::turn_left(){
    drive(25, 0);
    wait(2);

    drive(25, -100);
    wait(3.25);

    drive(20, 0);
    wait(1);
    stop();
}



void Driver::drive_forward(){
    int16_t speed = 0; //calculate_speed();
    int8_t steering_front = 0;//calculate_steering_front();
    int8_t steering_rear = 0;//calculate_steering_rear();

    struct start_driving_task_t start_driving_task = {
        .speed          = speed,
        .steering_front = steering_front,
        .steering_rear  = steering_rear,
        .id             = 1,
        .steps_ab       = 10
    };

    int32_t send_result = socket->send(ocMessageId::Start_Driving_Task, start_driving_task);
    logger->log("Result of sending driving task: %d", send_result);
}



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



void Driver::park(){
    drive(25, 0);
    wait(7);
    stop();

    drive(-20, -100);
    wait(2);
    drive(-20, 0);
    wait(1.5);

    drive(-20, 100);
    wait(1.5);
    stop();

    drive(20, 0);
    wait(1);

    stop();
}



void Driver::wait(float duration){
    auto start = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds;

    do {
        auto now = std::chrono::system_clock::now();
        elapsed_seconds = now - start;
    } while (elapsed_seconds.count() < duration);
}
