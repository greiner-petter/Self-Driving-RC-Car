#include "Driver.h"
#include "Statemachine.h"
#include "States/Normal_Drive.h"


int main(){

    Driver::initialize();

    Statemachine statemachine;
    //statemachine.run(nullptr);

    //Driver::drive(20, 0, 0);
    //Driver::turn_left();
    //Driver::turn_right();
    //Driver::stop();
    //Driver::park();
    //Driver::wait(2);
    //Driver::park_out();
    while(true){
        Driver::drive_forward();
        Driver::wait(0.1);
    }

    
    
    

    return 0;
}