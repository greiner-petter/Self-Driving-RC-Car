#pragma once
#include "State.h"
#include "../Statemachine.h"



class Crossing_4_Way: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Crossing_4_Way(){}
        Crossing_4_Way(const Crossing_4_Way& other);
        Crossing_4_Way& operator=(const Crossing_4_Way& other);
};

