#include "Parking.h"


State& Parking::get_instance(){
    static Parking singleton;
    return singleton;
}

void Parking::initialize(){
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



void Parking::on_entry(Statemachine* statemachine){
    initialize();
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Parking::run(Statemachine* statemachine, void* data){
    /*
    statemachine->change_state(Parking::getInstance());
    */
    
}



void Parking::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
