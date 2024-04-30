#include "Crossing_3_Way_T.h"


State& Crossing_3_Way_T::get_instance(){
    static Crossing_3_Way_T singleton;
    return singleton;
}



void Crossing_3_Way_T::on_entry(Statemachine* statemachine){
    /*
    Get IPC-hub messages regarding traffic signs;
    Create array of traffic-signs (types and distances);
    Array an statemachine->run übergeben;
    */

   statemachine->run(nullptr);
}


void Crossing_3_Way_T::run(Statemachine* statemachine, void* data){
    /*

    bool drive_left = false;
    bool drive_right = false;

    for sign in array:
        if (distance < 50){ //50cm == width of crossing; If distance larger, than sign is irrelevant for crossing
            switch(sign_type){
                case Stop:
                    drive.stop(2000); //stop for 2s
                    break;
                case RightOfWay:
                    drive_right = true;
                    break;
                case Left:
                    drive_left = true;
                    break;
                case Right:
                    drive_right = true;
                    break;
            }
        }

    if(drive_left && drive_right){
        drive_left = false;
    }

    //if obstacle, stop

    if(drive_left){
        drive.turn_left();
    } else if (drive_right){
        drive.turn_right();
    } else{
        drive.turn_right();
    }

    statemachine->change_state(Normal_Drive::getInstance());
    */
}



void Crossing_3_Way_T::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
