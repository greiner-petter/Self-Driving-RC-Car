#include "Driver.h"
#include "Statemachine.h"
#include "States/Normal_Drive.h"


int main(){

    Driver::initialize();

    Statemachine statemachine;
    statemachine.run(nullptr);

    //Driver::drive(20, 0, 0);
    //Driver::turn_left();
    //Driver::turn_right();
    //Driver::stop();
    //Driver::park();

    

    return 0;
}