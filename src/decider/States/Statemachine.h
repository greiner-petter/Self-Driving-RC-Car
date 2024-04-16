#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "State.h"

class State;


class Statemachine {
    private:
        State *current_state;
    public:
        Statemachine();
        void set_current_state(State* state);
        void change_state(State* state);
        void run(void *data);
};

#endif
