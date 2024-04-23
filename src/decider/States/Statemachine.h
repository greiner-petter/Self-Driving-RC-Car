#pragma once
#include "State.h"


class State;

class Statemachine {
    private:
        State* current_state;
    
    public:
        Statemachine();
        inline State* get_current_state() const { return current_state; }
        void change_state(State& state);
        void run(void *data);
};


