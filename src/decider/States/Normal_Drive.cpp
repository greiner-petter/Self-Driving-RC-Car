#include "Normal_Drive.h"
#include "Approaching_Crossing.h"
#include "../Driver.h"
#include "../../traffic_sign_detection/TrafficSign.h"
#include "Crossing_3_Way_Left.h"
#include "Crossing_3_Way_Right.h"
#include "Crossing_3_Way_T.h"
#include "Obstacle_State.h"
#include "Is_At_Crossing.h"



State& Normal_Drive::get_instance(){    
    static Normal_Drive singleton;
    return singleton;
}

void Normal_Drive::initialize(){
    if(!Normal_Drive::is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.set_sender(ocMemberId::Normal_Drive);
        sup.clear_and_edit()
            .write(ocMessageId::Intersection_Detected)
            .write(ocMessageId::Object_Found)
            .write(ocMessageId::Lane_Detection_Values)
            .write(ocMessageId::Traffic_Sign_Detected);
        socket->send_packet(sup);
        logger->log("Decider: Normal_Drive: send subscribe packet");

        Normal_Drive::is_initialized = true;
    }
}



void Normal_Drive::on_entry(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Normal_Drive);

    
    Normal_Drive::initialize();

    
    deafen.clear_and_edit()
        .write(ocMemberId::Normal_Drive)
        .write(false);
    socket->send_packet(deafen);

    logger->log("Decider: Initialized Normale_Drive. Running the state next");
    State::distance = 0;
    State::trafficSign = TrafficSignType::None;  

    statemachine->run(nullptr);
}


void Normal_Drive::run(Statemachine* statemachine, void* data){
    
    ocPacket recv_packet;
    
    while (true) {
       
        int result = socket->read_packet(recv_packet);
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Decider: Normal_Drive: Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {
            switch (recv_packet.get_message_id()){
                case ocMessageId::Intersection_Detected:{
                    auto reader = recv_packet.read_from_start();
                    uint32_t distance = reader.read<uint32_t>();
                    uint8_t crossing_type = reader.read<uint8_t>();
                    if(distance <= 5){
                        Is_At_Crossing::get_instance().crossing_type = crossing_type;
                        logger->log("Decider: Normal_Drive: Changing state from Normal_Drive to Is_At_Crossing");
                        statemachine->change_state(Is_At_Crossing::get_instance());  
                    } else{
                        logger->log("Decider: Normal_Drive: Changing state from Normal_Drive to Approaching_Crossing");
                        statemachine->change_state(Approaching_Crossing::get_instance());  
                    }
                }break;

                case ocMessageId::Object_Found:{
                    recv_packet.read_from_start();
                    logger->log("Decider: Normal_Drive: Changing state from Normal_Drive to Obstacle_State");
                    statemachine->change_state(Obstacle_State::get_instance());  
                }break;

                case ocMessageId::Lane_Detection_Values:{
                    auto reader = recv_packet.read_from_start();
                    int16_t speed = reader.read<int16_t>();
                    int8_t steering_front = reader.read<int8_t>();
                    int8_t steering_back = reader.read<int8_t>();
                    Driver::drive_both_steering_values(speed, steering_front, steering_back);
                }break;

                case ocMessageId::Traffic_Sign_Detected:{
                    auto reader = recv_packet.read_from_start();
                    uint16_t rawValue = reader.read<uint16_t>();
                    uint64_t distance = reader.read<uint64_t>();
                    TrafficSignType trafficSign = static_cast<TrafficSignType>(rawValue);

                    State::trafficSign = trafficSign;
                    State::distance = distance;
                }break;
                
                default:{
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId mbr_id = recv_packet.get_sender();
                    logger->warn("Decider: Normal_Drive: Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                }break;
            }
        }
    }

    
}



void Normal_Drive::on_exit(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Normal_Drive);
    deafen.clear_and_edit()
        .write(ocMemberId::Normal_Drive)
        .write(true);
    socket->send_packet(deafen);
}
