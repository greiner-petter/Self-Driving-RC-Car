#include "Is_At_Crossing.h"


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
            .write(ocMessageId::Driving_Task_Finished);
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
    /*
    State& crossing_state = nullptr;

    while(crossing_state==nullptr){
        crossing_type = IPC-Hub get crossing type
    
        switch(crossing_type){
            case X_3_R:
                crossing_state = Crossing_3_Way_Right::getInstance();
                break;
            case X_3_L:
                crossing_state = Crossing_3_Way_Left::getInstance();
                break;
            case X_3_T:
                crossing_state = Crossing_3_Way_T::getInstance();
                break;
            case X_4:
                crossing_state = Crossing_4_Way::getInstance();
                break;
            default:
                drive.stop()
                break;
        }
    }

    statemachine->change_state(crossing_state);
    */
    
}



void Is_At_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
