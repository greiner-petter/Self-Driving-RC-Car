#include "Parking.h"


State& Parking::get_instance(){
    static Parking singleton;
    return singleton;
}



void Parking::on_entry(Statemachine* statemachine){
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
