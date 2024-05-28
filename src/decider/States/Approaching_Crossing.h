#pragma once
#include "../../common/ocMember.h"
#include "../../common/ocCar.h"
#include "State.h"
#include "../Statemachine.h"

class ocMember;
class ocIpcSocket;

class Approaching_Crossing: State {
    public:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Approaching_Crossing, "Approaching_Crossing");
        static inline ocIpcSocket *socket;

        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        static inline ocLogger    *logger;
        static void initialize();
        Approaching_Crossing(){}
        Approaching_Crossing(const Approaching_Crossing& other);
        Approaching_Crossing& operator=(const Approaching_Crossing& other);
        double* smooth_speed(int current_speed);
};

