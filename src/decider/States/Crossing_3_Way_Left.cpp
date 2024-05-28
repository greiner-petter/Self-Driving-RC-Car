#include "Crossing_3_Way_Left.h"


State& Crossing_3_Way_Left::get_instance(){
    static Crossing_3_Way_Left singleton;
    return singleton;
}

void Crossing_3_Way_Left::initialize(){
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


void Crossing_3_Way_Left::on_entry(Statemachine* statemachine){
    /*
    Get IPC-hub messages regarding traffic signs;
    Create array of traffic-signs (types and distances);
    Array an statemachine->run übergeben;
    */
   initialize();
   statemachine->run(nullptr);
}


void Crossing_3_Way_Left::run(Statemachine* statemachine, void* data){
    /*

    bool drive_left = false;
    bool drive_forward = false;

    for sign in array:
        if (distance < 50){ //50cm == width of crossing; If distance larger, than sign is irrelevant for crossing
            switch(sign_type){
                case Stop:
                    drive.stop(2000); //stop for 2s
                    break;
                case RightOfWay:
                    drive_forward = true;
                    break;
                case Left:
                    drive_left = true;
                    break;
                case Right:
                    drive_left = true;
                    break;
            }
        }

    if(drive_left && drive_forward){
        drive_left = false;
    }

    //if obstacle, stop

    if(drive_left){
        drive.turn_left();
    } else if (drive_forward){
        drive.drive_forward();
    } else{
        drive.drive_forward();
    }

    statemachine->change_state(Normal_Drive::getInstance());
    */
}



void Crossing_3_Way_Left::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
