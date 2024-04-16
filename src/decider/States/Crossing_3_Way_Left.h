#ifndef CROSSING_3_WAY_LEFT_H
#define CROSSING_3_WAY_LEFT_H

//#include "Statemachine.h"

class Statemachine;

class Crossing_3_Way_Left: State {
    void run(Statemachine& statemachine, void* data);
    void on_entry(Statemachine& statemachine);
    void on_exit(Statemachine& statemachine);

    
};

#endif