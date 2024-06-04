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
    uint32_t message = socket->read_packet(ocMessageId::Intersection_Detected);
    while(true){
        bool crossing_dtected = (message == null ? false : true);
        if (crossing_detected){
            statemachine->change_state(Approaching_Crossing::getInstance());
            break;
        }
        drive.drive_forward();
    }
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
