/*
 * main.cpp
 *
 *  Created on: Aug 25, 2016
 *      Author: marian
 */

#include <cstdlib> // EXIT_FAILURE
#include "ipc_hub.h"

int main()
{
    IpcHub hub;

    if (hub.create_shared_memory() == EXIT_FAILURE) exit(1);
    if (hub.start_server() == EXIT_FAILURE) exit(1);

    while(true) hub.process_clients();
}


