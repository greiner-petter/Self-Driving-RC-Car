#include "Approaching_Crossing.h"


State& Approaching_Crossing::get_instance(){
    static Approaching_Crossing singleton;
    return singleton;
}



void Approaching_Crossing::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Approaching_Crossing::run(Statemachine* statemachine, void* data){
    /*
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Approaching_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
