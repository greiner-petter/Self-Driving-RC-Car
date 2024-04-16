#include "Statemachine.h"

Statemachine::Statemachine() : current_state(nullptr){}

void Statemachine::set_current_state(State* state){
    current_state = state;
    current_state->on_entry(*this);
}


void Statemachine::change_state(State* state){
    if(current_state != nullptr){
        current_state->on_exit(*this);
        delete current_state;
    }
    current_state = state;
    current_state->on_entry(*this);
}


void Statemachine::run(void *data){
    if(current_state != nullptr){
        current_state->run(*this, data);
    }
}
