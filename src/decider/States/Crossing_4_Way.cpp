#include "Crossing_4_Way.h"


State& Crossing_4_Way::get_instance(){
    static Crossing_4_Way singleton;
    return singleton;
}



void Crossing_4_Way::on_entry(Statemachine* statemachine){
    /*
    Code
    */

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
