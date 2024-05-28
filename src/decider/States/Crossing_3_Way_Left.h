#pragma once
#include "../../common/ocMember.h"
#include "../../common/ocCar.h"
#include "State.h"
#include "../Statemachine.h"

class ocMember;
class ocIpcSocket;


class Crossing_3_Way_Left: State {
    public:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Crossing_3_Way_Left, "Crossing_3_Way_Left");
        static inline ocIpcSocket *socket;

        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        static inline ocLogger    *logger;
        static void initialize();
        Crossing_3_Way_Left(){}
        Crossing_3_Way_Left(const Crossing_3_Way_Left& other);
        Crossing_3_Way_Left& operator=(const Crossing_3_Way_Left& other);
};

