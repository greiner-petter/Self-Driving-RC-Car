#include "Normal_Drive.h"
#include "../Driver.h"


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

   while(true){

    ocMember member = ocMember(ocMemberId::Driver, "Driver");
    ocIpcSocket *socket;

    int8_t angle = 0;
    Driver::drive(20, angle);
   }
    
    //statemachine->change_state(Approaching_Crossing::getInstance());
}



void Normal_Drive::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
