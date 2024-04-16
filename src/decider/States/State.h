#ifndef STATE_H
#define STATE_H

class Statemachine;

class State{
    public:
        virtual void on_entry(Statemachine& statemachine);
        virtual void run(Statemachine& statemachine, void *data);
        virtual void on_exit(Statemachine& statemachine);
};

#endif
