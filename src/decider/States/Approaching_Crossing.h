#pragma once
#include "State.h"
#include "../Statemachine.h"



class Approaching_Crossing: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Approaching_Crossing(){}
        Approaching_Crossing(const Approaching_Crossing& other);
        Approaching_Crossing& operator=(const Approaching_Crossing& other);
        double* smooth_speed(int current_speed);
};

