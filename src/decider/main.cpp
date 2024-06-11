#include "Driver.h"
#include "Statemachine.h"
#include "States/Normal_Drive.h"


int main(){

    Driver::initialize();

    //Statemachine statemachine;
    //statemachine.run(nullptr);

    Driver::stop(1);
    Driver::turn_right();
    Driver::stop(1);
    //Driver::turn_right();
    //Driver::stop();
    
    //Driver::park();
    //Driver::wait(2);
    //Driver::park_out();
    //Driver::stop();
    /*
    while(true){
        Driver::drive_forward();
        Driver::wait(0.1);
    }
    */
    
    
    

    return 0;
}
