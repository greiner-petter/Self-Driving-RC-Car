#include "Crossing_3_Way_Left.h"


State& Crossing_3_Way_Left::get_instance(){
    static Crossing_3_Way_Left singleton;
    return singleton;
}



void Crossing_3_Way_Left::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Crossing_3_Way_Left::run(Statemachine* statemachine, void* data){
    /*
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Crossing_3_Way_Left::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
