#include "Normal_Drive.h"
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
            .write(ocMessageId::Driving_Task_Finished);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



void Normal_Drive::on_entry(Statemachine* statemachine){
    /*
    Code
    */
   initialize();

   statemachine->run(nullptr);
}


void Normal_Drive::run(Statemachine* statemachine, void* data){
    /*
    ocPacket recv_packet;
    
    while (true) {
       
        int result = socket->read_packet(recv_packet);
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
            break;
        }

        switch (recv_packet.get_message_id())
        {
        case ocMessageId::Intersection_Detected:{
            statemachine->change_state(Approaching_Crossing::getInstance());  
            }break;
        
        default:{
            ocMessageId msg_id = recv_packet.get_message_id();
            ocMemberId mbr_id = recv_packet.get_sender();
            logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
            }break;
        }

        }

    }

    drive.drive_forward();
    */

   while(true){

    ocMember member = ocMember(ocMemberId::Driver, "Driver");
    ocIpcSocket *socket;

    int8_t angle = 0;
    Driver::drive(20, angle);
   }
    
}



void Normal_Drive::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
