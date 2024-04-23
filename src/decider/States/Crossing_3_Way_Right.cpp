#include "Crossing_3_Way_Right.h"


State& Crossing_3_Way_Right::get_instance(){
    static Crossing_3_Way_Right singleton;
    return singleton;
}



void Crossing_3_Way_Right::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Crossing_3_Way_Right::run(Statemachine* statemachine, void* data){
    /*
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Crossing_3_Way_Right::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
