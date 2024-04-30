#pragma once
#include "State.h"
#include "../Statemachine.h"
#include "ocMember.h"

class Crossing_3_Way_T: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Crossing_3_Way_T(){}
        Crossing_3_Way_T(const Crossing_3_Way_T& other);
        Crossing_3_Way_T& operator=(const Crossing_3_Way_T& other);
};
