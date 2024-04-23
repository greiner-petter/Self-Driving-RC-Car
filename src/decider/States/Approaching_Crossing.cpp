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
    bool is_at_crossing = false;

    while(!is_at_crossing){
        distance = IPC Hub get distance;

        //algorithm for slowing down towards 2cm/s

        if(distance <= threshold){ //threshold == 2cm
            is_at_crossing = true;
        }
    }
    
    statemachine->change_state(Is_At_Crossing::getInstance());
    */
}



void Approaching_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
