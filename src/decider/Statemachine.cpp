#include "Statemachine.h"
#include "States/Crossing_3_Way_Right.h"
#include "States/Crossing_3_Way_Left.h"
#include "States/Crossing_3_Way_T.h"
#include "States/Crossing_4_Way.h"
#include "States/Is_At_Crossing.h"
#include "States/Approaching_Crossing.h"
#include "States/Normal_Drive.h"



Statemachine::Statemachine(){
    this->current_state = &Crossing_3_Way_Left::get_instance();
}


void Statemachine::change_state(State& state){
    if(current_state != nullptr){
        current_state->on_exit(this);
    }
    current_state = &state;
    current_state->on_entry(this);
}


void Statemachine::run(void *data){
    if(current_state != nullptr){
        current_state->run(this, data);
    }
}
