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
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Normal_Drive::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
