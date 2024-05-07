#pragma once

#include "../common/ocMember.h"
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
        static inline ocLogger    *logger;

        static inline struct start_driving_task_t start_driving_task;
    

    public:
        static void initialize();

        static void turn_right();
        static void turn_left();
        static void drive_forward();
        static void drive_forward(int16_t speed);
        static void stop(int duration);
};



