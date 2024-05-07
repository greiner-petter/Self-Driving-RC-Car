#include "Drive.h"
#include "Statemachine.h"


int main(){

    Statemachine statemachine;
    statemachine.run(nullptr);

    Driver::initialize();
    //Driver::drive_forward(32);

    
    while(true){
        Driver::drive_forward(15);
        Driver::stop(1);
    }
    
    

    return 0;
}