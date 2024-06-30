#pragma once
#include "../Statemachine.h"
#include <stdint.h>
#include "../../traffic_sign_detection/TrafficSign.h"


class Statemachine;

class State{
    public:
        virtual void on_entry(Statemachine* statemachine) = 0;
        virtual void run(Statemachine* statemachine, void *data) = 0;
        virtual void on_exit(Statemachine* statemachine) {
            distance = 0;
            trafficSign = TrafficSignType::None;
        }
        virtual ~State(){}
        
        inline static TrafficSignType trafficSign = TrafficSignType::None;
        inline static uint64_t distance = 0;

        
        uint8_t crossing_type;
};

