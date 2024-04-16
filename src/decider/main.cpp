#include "Drive.h"
#include "States/Statemachine.h"
#include "States/Crossing_3_Way_Left.h"

/*
enum class States {
    NORMAL_DIRVE,
    APPROACHING_CROASSING,
    IS_AT_CROSSING,
    CROSSING_3_WAY_LEFT,
    CROSSING_3_WAY_RIGHT,
    CROSSING_3_WAY_T,
    CROSSING_4_WAY
};


States state{States::NORMAL_DIRVE};
bool on_entry = true;
bool on_exit = false;
*/

int main(){

    Statemachine statemachine;
    statemachine.set_current_state((State*) new Crossing_3_Way_Left());

    /*

    switch(state){
        case States::NORMAL_DIRVE:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;

        
        case States::APPROACHING_CROASSING:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;


        case States::IS_AT_CROSSING:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;

        
        case States::CROSSING_3_WAY_LEFT:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;


        case States::CROSSING_3_WAY_RIGHT:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;


        case States::CROSSING_3_WAY_T:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;


        case States::CROSSING_4_WAY:
            if (on_entry){
                on_entry = false;
                //tbd
            }

            //tbd

            if(on_exit){
                on_exit = false;

                //tbd

                on_entry = true;
            }
            break;
    }
    */


    return 0;
}