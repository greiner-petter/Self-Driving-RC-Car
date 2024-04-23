#include "Crossing_3_Way_T.h"


State& Crossing_3_Way_T::get_instance(){
    static Crossing_3_Way_T singleton;
    return singleton;
}



void Crossing_3_Way_T::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Crossing_3_Way_T::run(Statemachine* statemachine, void* data){
    /*
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Crossing_3_Way_T::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
