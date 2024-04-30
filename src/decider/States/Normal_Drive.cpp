#include "Normal_Drive.h"


State& Normal_Drive::get_instance(){
    static Normal_Drive singleton;
    return singleton;
}



void Normal_Drive::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Normal_Drive::run(Statemachine* statemachine, void* data){
    /*
    while(true){
        bool crossing_dtected = IPC-HUB has crossing been detected
        if (crossing_detected){
            break;
        }
        drive.drive_forward();
    }
    */
    
    //statemachine->change_state(Approaching_Crossing::getInstance());
}



void Normal_Drive::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
