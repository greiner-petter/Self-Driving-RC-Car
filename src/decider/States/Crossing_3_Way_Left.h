#pragma once
#include "State.h"
#include "../Statemachine.h"



class Crossing_3_Way_Left: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Crossing_3_Way_Left(){}
        Crossing_3_Way_Left(const Crossing_3_Way_Left& other);
        Crossing_3_Way_Left& operator=(const Crossing_3_Way_Left& other);
};

