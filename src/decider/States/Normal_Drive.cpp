#include "Normal_Drive.h"
#include "Approaching_Crossing.h"
#include "../Driver.h"


State& Normal_Drive::get_instance(){
    static Normal_Drive singleton;
    return singleton;
}

void Normal_Drive::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.clear_and_edit()
            .write(ocMessageId::Intersection_Detected)
            .write(ocMessageId::Object_Found)
            .write(ocMessageId::Lane_Detection_Values);
        socket->send_packet(sup);
        logger->log("Decider: Normal_Drive: send subscribe packet");

        is_initialized = true;
    }
}



void Normal_Drive::on_entry(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Normal_Drive);
    
   initialize();

    
    deafen.clear_and_edit()
        .write(ocMemberId::Normal_Drive)
        .write(false);
    socket->send_packet(deafen);

   logger->log("Decider: Initialized Normale_Drive. Running the state next");

   statemachine->run(nullptr);
}


void Normal_Drive::run(Statemachine* statemachine, void* data){
    
    ocPacket recv_packet;
    
    while (true) {
       
        int result = socket->read_packet(recv_packet);
        bool object_found = false;
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Decider: Normal_Drive: Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {
            switch (recv_packet.get_message_id()){
                case ocMessageId::Intersection_Detected:{
                    recv_packet.read_from_start();
                    logger->log("Decider: Normal_Drive: Changing state from Normal_Drive to Approaching_Crossing");
                    statemachine->change_state(Approaching_Crossing::get_instance());  
                }break;

                case ocMessageId::Object_Found:{
                    recv_packet.read_from_start();
                    object_found = true;
                }break;

                case ocMessageId::Lane_Detection_Values:{
                    auto reader = recv_packet.read_from_start();

                    int16_t speed = reader.read<int16_t>();
                    int8_t steering_front = reader.read<int8_t>();
                    int8_t steering_back = reader.read<int8_t>();

                    if(object_found){
                        Driver::stop();
                        object_found = false;
                    } else{
                        Driver::drive_both_steering_values(speed, steering_front, steering_back);
                    }

                }break;
                
                default:{
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId mbr_id = recv_packet.get_sender();
                    logger->warn("Decider: Normal_Drive: Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                }break;
            }
        }

        /*
        if(object_found){
            Driver::stop();
        } else{
            Driver::drive_forward();
        }
        */
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
