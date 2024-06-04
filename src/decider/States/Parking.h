#pragma once
#include "State.h"
#include "../Statemachine.h"



class Parking: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Parking(){}
        Parking(const Parking& other);
        Parking& operator=(const Parking& other);
};

