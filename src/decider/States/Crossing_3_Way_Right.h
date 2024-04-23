#pragma once
#include "State.h"
#include "../Statemachine.h"



class Crossing_3_Way_Right: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Crossing_3_Way_Right(){}
        Crossing_3_Way_Right(const Crossing_3_Way_Right& other);
        Crossing_3_Way_Right& operator=(const Crossing_3_Way_Right& other);
};

