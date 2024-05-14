#include "Driver.h"
#include "Statemachine.h"
#include <chrono>


int main(){

    Statemachine statemachine;
    statemachine.run(nullptr);

    Driver::initialize();

    //Driver::drive(20, 0, 0);
    //Driver::turn_left();
    Driver::turn_right();
    //Driver::stop();

    

    return 0;
}