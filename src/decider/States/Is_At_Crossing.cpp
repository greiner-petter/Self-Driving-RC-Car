#include "Is_At_Crossing.h"
#include "Crossing_3_Way_Left.h"
#include "Crossing_3_Way_Right.h"
#include "Crossing_3_Way_T.h"
#include "../Driver.h"


State& Is_At_Crossing::get_instance(){
    static Is_At_Crossing singleton;
    return singleton;
}

void Is_At_Crossing::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.clear_and_edit()
            .write(ocMessageId::Intersection_Detected);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



void Is_At_Crossing::on_entry(Statemachine* statemachine){
    /*
    Code
    */
   initialize();

   statemachine->run(nullptr);
}


void Is_At_Crossing::run(Statemachine* statemachine, void* data){
    
    ocPacket recv_packet;
    bool crossing_type_received = false; 

    uint8_t crossing_type;
    State& crossing_state = Crossing_3_Way_Right::get_instance();

    Driver::stop();

    while (!crossing_type_received) {
       
        int result = socket->read_packet(recv_packet);
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {

            switch (recv_packet.get_message_id()){
                case ocMessageId::Intersection_Detected:{
                    auto reader = recv_packet.read_from_start();
                    uint32_t distance = reader.read<uint32_t>();
                    crossing_type = reader.read<uint8_t>();
                    crossing_type_received = true;
                } break;
                
                default:{
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId mbr_id = recv_packet.get_sender();
                    logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                } break;
            }

        }
    

        if(crossing_type_received){
            if(crossing_type & 1) {
                crossing_state = Crossing_3_Way_Left::get_instance();
                logger->log("Changing state from Is_At_Crossing to Crossing_3_Way_Left");
            } else if(crossing_type & 2) {
                crossing_state = Crossing_3_Way_Right::get_instance();
                logger->log("Changing state from Is_At_Crossing to Crossing_3_Way_Right");
            } else if(crossing_type & 4) {
                crossing_state = Crossing_3_Way_T::get_instance();
                logger->log("Changing state from Is_At_Crossing to Crossing_3_Way_T");
            }
        } else {
            Driver::drive(5, 0);
        }

    }
    
    statemachine->change_state(crossing_state);
    
    
    
}



void Is_At_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
