#pragma once
#include "../../common/ocMember.h"
#include "../../common/ocCar.h"
#include "State.h"
#include "../Statemachine.h"

class ocMember;
class ocIpcSocket;


class Obstacle_State: State {
    public:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Obstacle_State, "Obstacle_State");
        static inline ocIpcSocket *socket;
        
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        static inline ocLogger    *logger;
        static void initialize();
        Obstacle_State(){}
        Obstacle_State(const Obstacle_State& other);
        Obstacle_State& operator=(const Obstacle_State& other);

};