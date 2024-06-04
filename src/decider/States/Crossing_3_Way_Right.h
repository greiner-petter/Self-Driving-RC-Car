#pragma once
#include "../../common/ocMember.h"
#include "../../common/ocCar.h"
#include "State.h"
#include "../Statemachine.h"
#include "TrafficSign.h"
#include "../Driver.h"

class ocMember;
class ocIpcSocket;

class Crossing_3_Way_Right: State {
    public:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Crossing_3_Way_Right, "Crossing_3_Way_Right");
        static inline ocIpcSocket *socket;

        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        static inline ocLogger    *logger;
        static void initialize();
        Crossing_3_Way_Right(){}
        Crossing_3_Way_Right(const Crossing_3_Way_Right& other);
        Crossing_3_Way_Right& operator=(const Crossing_3_Way_Right& other);
};

