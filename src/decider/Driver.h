#pragma once

#include "../common/ocMember.h"
#include "../common/ocCar.h"
#include <cstdint>

const int KP_SPEED = 0;
const int KD_SPEED = 0;
const int KI_SPEED = 0;

const int KP_STEER_FRONT = 0;
const int KD_STEER_FRONT = 0;
const int KI_STEER_FRONT = 0;

const int KP_STEER_BACK = 0;
const int KD_STEER_BACK = 0;
const int KI_STEER_BACK = 0;


struct start_driving_task_t{
    int16_t speed;
    int8_t 	steering_front;
    int8_t 	steering_rear;
    int8_t 	id;
    int32_t steps_ab;
};





class ocMember;
class ocIpcSocket;



class Driver {
    private:
        static inline bool is_initialized = false;

        static inline ocMember member = ocMember(ocMemberId::Driver, "Driver");
        static inline ocIpcSocket *socket;
    

    public:
        static inline ocLogger    *logger;
        static void initialize();

        static void turn_right();
        static void turn_left();
        static void drive_forward();
        static void drive(int16_t speed, int8_t steering=0);
        static void stop(float duration=0);
        static void park();
        static void park_out();
        static void wait(float duration=0);
};



