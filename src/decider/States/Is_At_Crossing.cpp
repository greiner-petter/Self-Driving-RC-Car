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
        //ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        //sup.clear_and_edit()
        //    .write(ocMessageId::Intersection_Detected);
        //socket->send_packet(sup);

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

   //1 == left free
   //2 == right free
   //4 == front free
    
   
    if(this->crossing_type & 0b101) {
        logger->log("Decider: Is_At_Crossing: Changing state from Is_At_Crossing to Crossing_3_Way_Left");
        statemachine->change_state(Crossing_3_Way_Left::get_instance());
    } else if(this->crossing_type & 0b110) {
        logger->log("Decider: Is_At_Crossing: Changing state from Is_At_Crossing to Crossing_3_Way_Right");
        statemachine->change_state(Crossing_3_Way_Right::get_instance());
    } else if(this->crossing_type & 0b011) {
        logger->log("Decider: Is_At_Crossing: Changing state from Is_At_Crossing to Crossing_3_Way_T");
        statemachine->change_state(Crossing_3_Way_T::get_instance());
    } else{
        logger->log("Decider: Is_At_Crossing: No correct crossing detected");
        statemachine->change_state(crossing_state);
    }
    
    
    
}



void Is_At_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
