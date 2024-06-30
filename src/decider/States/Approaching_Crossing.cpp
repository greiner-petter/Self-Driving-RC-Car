#include "Approaching_Crossing.h"
#include "Is_At_Crossing.h"
#include "../Driver.h"
#include <math.h>
#include <signal.h>



State& Approaching_Crossing::get_instance(){
    static Approaching_Crossing singleton;
    return singleton;
}




void Approaching_Crossing::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.set_sender(ocMemberId::Approaching_Crossing);
        sup.clear_and_edit()
            .write(ocMessageId::Intersection_Detected)
            .write(ocMessageId::Lane_Detection_Values)
            .write(ocMessageId::Object_Found)
            .write(ocMessageId::Traffic_Sign_Detected);
        socket->send_packet(sup);
        is_initialized = true;
    }
}



void Approaching_Crossing::on_entry(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Approaching_Crossing);
    initialize();

    deafen.clear_and_edit()
        .write(ocMemberId::Approaching_Crossing)
        .write(false);
    socket->send_packet(deafen);

    statemachine->run(nullptr);
}


void Approaching_Crossing::run(Statemachine* statemachine, void* data){
    bool is_at_crossing = false;
    uint8_t crossing_type;
    ocPacket recv_packet;

    uint32_t min_distance = 7;
    uint32_t max_distance = 25;
    uint32_t distance = 0xFFFF;

    int16_t speed = 15;
    int16_t min_speed = 15;
    int8_t steering_front = 0;
    int8_t steering_back = 0;


    while (!is_at_crossing) {
       
        int result = socket->read_packet(recv_packet);
        bool object_found = false;
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Decider: Approaching_Crossing: Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {

            switch (recv_packet.get_message_id()){
                case ocMessageId::Intersection_Detected:{
                    auto reader = recv_packet.read_from_start();
                    distance = reader.read<uint32_t>();
                    crossing_type = reader.read<uint8_t>();
                    logger->log("Decider: Approaching_Crossing: Distance: %d", distance);
                }break;

                case ocMessageId::Lane_Detection_Values:{
                    auto reader = recv_packet.read_from_start();
                    speed = reader.read<int16_t>();
                    steering_front = reader.read<int8_t>();
                    steering_back = reader.read<int8_t>();

                }break;

                case ocMessageId::Traffic_Sign_Detected:{
                    auto reader = recv_packet.read_from_start();
                    uint16_t rawValue = reader.read<uint16_t>();
                    uint64_t distance = reader.read<uint64_t>();
                    TrafficSignType trafficSign = static_cast<TrafficSignType>(rawValue);

                    State::trafficSign = trafficSign;
                    State::distance = distance;
                }break;

                case ocMessageId::Object_Found:{
                    recv_packet.read_from_start();
                    object_found = true;
                }break;
                
                default:{
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId mbr_id = recv_packet.get_sender();
                    logger->warn("Decider: Approaching_Crossing: Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                }break;
            }
        }

        if(object_found){
            Driver::stop();

            object_found = false;
        } else if(distance <= min_distance) {
            is_at_crossing = true;

            ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
            deafen.set_sender(ocMemberId::Approaching_Crossing);
            deafen.clear_and_edit()
                .write(ocMemberId::Approaching_Crossing)
                .write(true);
            socket->send_packet(deafen);

        } else if (distance > max_distance){
            Driver::drive_both_steering_values(speed, steering_front, steering_back);
            //Driver::wait(0.1);
        } else {
            Driver::drive_both_steering_values(min_speed, steering_front, steering_back);
            //Driver::wait(0.1);
        }
        /*
        //algorithm for slowing down towards 2cm/s
        double* arr = smooth_speed(speed);
        for(int i = 0; i < 100; i++) {
            int result = socket->read_packet(recv_packet);

            if (result >= 0) {
                switch (recv_packet.get_message_id()){
                    case ocMessageId::Intersection_Detected:{
                        auto reader = recv_packet.read_from_start();
                        distance = reader.read<uint32_t>();
                        uint8_t crossing_type = reader.read<uint8_t>();
                    }break;

                    case ocMessageId::Lane_Detection_Values:{
                        auto reader = recv_packet.read_from_start();
                        speed = reader.read<int16_t>();
                        steering_front = reader.read<int8_t>();
                    }break;

                    default:
                        break;
                }

                int16_t next_speed = arr[i] > min_speed ? arr[i] : min_speed;
                Driver::drive(next_speed, steering_front);    //drive_forward(arr[i]);
                Driver::wait(0.1);
            }

            if(distance <= threshold) {
                is_at_crossing = true;
                break;
            }
            
        }

        free(arr);
        */
    }

    
    logger->log("Decider: Approaching_Crossing: Changing state from Approaching_Crossing to Is_At_Crossing");
    Is_At_Crossing::get_instance().crossing_type = crossing_type;
    statemachine->change_state(Is_At_Crossing::get_instance());
    
}



void Approaching_Crossing::on_exit(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Approaching_Crossing);
    deafen.clear_and_edit()
        .write(ocMemberId::Approaching_Crossing)
        .write(true);
    socket->send_packet(deafen);
}




double* Approaching_Crossing::smooth_speed(int16_t current_speed) {
    int range = 100;
    int nzero = current_speed;
    float a = 0.98; //0<a<1 current: 2%
    int y = 0;

    double* results = (double*)malloc(sizeof(double) * 100);

    for(int i = 0; i < range; i++) {
        y = nzero * pow(a, i);
        results[i] = y;
    }

    return results;

}