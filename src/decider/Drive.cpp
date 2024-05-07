#include <cstdint>
#include <chrono>
#include "Drive.h"


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
    return 0;
}



int8_t calculate_steering_front(){
    return 0;
}



int8_t calculate_steering_rear(){
    return 0;
}






void Driver::turn_right(){

}


void Driver::turn_left(){

}


void Driver::drive_forward(){

    //IPC-HUB get vector

    /*
    int16_t speed = 32;//calculate_speed();
    int8_t steering_front = 0;//calculate_steering_front();
    int8_t steering_rear = 0;//calculate_steering_rear();

    struct start_driving_task start_driving_task = {
        .speed = speed,
        .steering_front = steering_front,
        .steering_rear = steering_rear,
        .id = 1,
        .steps_ab = 1
    };

    
    ocPacket packet(ocMessageId::Start_Driving_Task);
    packet.edit_from_end().write(&start_driving_task, sizeof(start_driving_task));
    socket->send_packet(packet);
    */



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
}


void Driver::drive_forward(int16_t speed){
    int8_t steering_front = 0;//calculate_steering_front();
    int8_t steering_rear = 0;//calculate_steering_rear();
    
    start_driving_task.speed          = speed;
    start_driving_task.steering_front = steering_front;
    start_driving_task.steering_rear  = steering_rear;
    start_driving_task.id             = 1;
    start_driving_task.steps_ab       = 1;
    
    ocPacket packet(ocMessageId::Start_Driving_Task);
    packet.edit_from_end().write(&start_driving_task, sizeof(start_driving_task_t));
    socket->send_packet(packet);

    static ocPacket recv_packet;
    int result = socket->read_packet(recv_packet);
    //Driver::logger->log("Result: %d", result);
}



void Driver::stop(int duration){

    start_driving_task.speed          = 0;
    start_driving_task.steering_front = 0;
    start_driving_task.steering_rear  = 0;
    start_driving_task.id             = 1;
    start_driving_task.steps_ab       = 0;

    ocPacket packet(ocMessageId::Start_Driving_Task);
    packet.edit_from_end().write(&start_driving_task, sizeof(start_driving_task_t));
    socket->send_packet(packet);

    auto start = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds;

    do {
        auto now = std::chrono::system_clock::now();
        elapsed_seconds = now - start;
    } while (elapsed_seconds.count() < duration);

}
