#include "Approaching_Crossing.h"
#include <math.h>


State& Approaching_Crossing::get_instance(){
    static Approaching_Crossing singleton;
    return singleton;
}



void Approaching_Crossing::on_entry(Statemachine* statemachine){
    /*
    Code
    */

   statemachine->run(nullptr);
}


void Approaching_Crossing::run(Statemachine* statemachine, void* data){
    /*
    bool is_at_crossing = false;

    while(!is_at_crossing){
        distance = IPC Hub get distance;

        //algorithm for slowing down towards 2cm/s
        double* arr = smooth_speed(curr_speed);
        for(int i = 0; i < 100; i++) {
            drive_forward(arr[i]);
            delay(x)
        }

        if(distance <= threshold){ //threshold == 2cm
            is_at_crossing = true;
        }
    }
    
    statemachine->change_state(Is_At_Crossing::getInstance());
    */
}



void Approaching_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}

double* smooth_speed(int current_speed) {
    int range = 100;
    int nzero = current_speed;
    float a = 0.98; //0<a<1 current: 2%
    int y = 0;

    double results[range];

    for(int i = 0; i < range; i++) {
        y = nzero * pow(a, i);
        results[i] = y;
        //drive_forward(y);
    }

    return results;

}