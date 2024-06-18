#include "Driver.h"
#include "Statemachine.h"
#include "States/Normal_Drive.h"


int main(){

    Driver::initialize();

    Statemachine statemachine;

    Driver::logger->log("Now beginning to run the statemachine");

    statemachine.change_state(Normal_Drive::get_instance());
    statemachine.run(nullptr);

    //Driver::stop(1);
    //Driver::turn_right();
    //Driver::stop(1);
    //Driver::turn_right();
    //Driver::stop();
    
    //Driver::park();
    //Driver::wait(2);
    //Driver::park_out();
    //while(true){
    //    Driver::drive_forward();
    //    Driver::wait(0.1);
    //}

    
    
    

    return 0;
}
