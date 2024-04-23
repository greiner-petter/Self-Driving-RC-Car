#pragma once
#include "State.h"
#include "../Statemachine.h"



class Normal_Drive: State {
    public:
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        Normal_Drive(){}
        Normal_Drive(const Normal_Drive& other);
        Normal_Drive& operator=(const Normal_Drive& other);
};

