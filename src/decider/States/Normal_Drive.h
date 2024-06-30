#pragma once
#include "../../common/ocMember.h"
#include "../../common/ocCar.h"
#include "State.h"
#include "../Statemachine.h"

class ocMember;
class ocIpcSocket;


class Normal_Drive: State {
    public:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Normal_Drive, "Normal_Drive");
        static inline ocIpcSocket *socket = nullptr;
        
        void run(Statemachine* statemachine, void* data);
        void on_entry(Statemachine* statemachine);
        void on_exit(Statemachine* statemachine);
        static State& get_instance();


    private:
        static inline ocLogger    *logger;
        static void initialize();
        Normal_Drive(){}
        Normal_Drive(const Normal_Drive& other);
        Normal_Drive& operator=(const Normal_Drive& other);
};

