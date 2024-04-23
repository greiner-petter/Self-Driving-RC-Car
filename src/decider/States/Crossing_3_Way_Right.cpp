#include "Crossing_3_Way_Right.h"


State& Crossing_3_Way_Right::get_instance(){
    static Crossing_3_Way_Right singleton;
    return singleton;
}



void Crossing_3_Way_Right::on_entry(Statemachine* statemachine){
    /*
    Get IPC-hub messages regarding traffic signs;
    Create array of traffic-signs (types and distances);
    Array an statemachine->run übergeben;
    */

   statemachine->run(nullptr);
}


void Crossing_3_Way_Right::run(Statemachine* statemachine, void* data){
    /*

    bool drive_right = false;
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
                    drive_right = true;
                    break;
                case Right:
                    drive_right = true;
                    break;
            }
        }

    if(drive_right && drive_forward){
        drive_forward = false;
    }

    //if obstacle, stop

    if(drive_right){
        drive.turn_right();
    } else if (drive_forward){
        drive.drive_forward();
    } else{
        drive.turn_right();
    }

    statemachine->change_state(Normal_Drive::getInstance());
    */
}



void Crossing_3_Way_Right::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
