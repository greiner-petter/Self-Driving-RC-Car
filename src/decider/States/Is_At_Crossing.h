#pragma once
#include "State.h"
#include "../Statemachine.h"



class Is_At_Crossing: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Is_At_Crossing(){}
        Is_At_Crossing(const Is_At_Crossing& other);
        Is_At_Crossing& operator=(const Is_At_Crossing& other);
};

