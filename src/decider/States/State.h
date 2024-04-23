#pragma once
#include "Statemachine.h"


class Statemachine;

class State{
    public:
        virtual void on_entry(Statemachine* statemachine) = 0;
        virtual void run(Statemachine* statemachine, void *data) = 0;
        virtual void on_exit(Statemachine* statemachine) = 0;
        virtual ~State(){}
};

