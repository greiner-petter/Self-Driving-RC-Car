#pragma once
#include "../../common/ocMember.h"
#include "../../common/ocCar.h"
#include "State.h"
#include "../Statemachine.h"

class ocMember;
class ocIpcSocket;

class Crossing_4_Way: State {
    public:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Crossing_4_Way, "Crossing_4_Way");
        static inline ocIpcSocket *socket;
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        static inline ocLogger    *logger;
        static void initialize();
        Crossing_4_Way(){}
        Crossing_4_Way(const Crossing_4_Way& other);
        Crossing_4_Way& operator=(const Crossing_4_Way& other);
};

