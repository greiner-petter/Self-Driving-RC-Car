#include "Is_At_Crossing.h"


State& Is_At_Crossing::get_instance(){
    static Is_At_Crossing singleton;
    return singleton;
}



void Is_At_Crossing::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Is_At_Crossing::run(Statemachine* statemachine, void* data){
    /*
    Code
    */
    
    //statemachine->change_state(SomeState::getInstance());
}



void Is_At_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
