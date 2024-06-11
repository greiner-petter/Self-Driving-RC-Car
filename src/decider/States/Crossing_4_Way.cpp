#include "Crossing_4_Way.h"


State& Crossing_4_Way::get_instance(){
    static Crossing_4_Way singleton;
    return singleton;
}

void Crossing_4_Way::initialize(){
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



void Crossing_4_Way::on_entry(Statemachine* statemachine){
    /*
    Code
    */
   initialize();

   statemachine->run(nullptr);
}


void Crossing_4_Way::run(Statemachine* statemachine, void* data){
    /*
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Crossing_4_Way::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
