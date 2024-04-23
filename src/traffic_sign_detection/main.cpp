#include <iostream>
#include "../common/ocMember.h"

#include "SignDetector.h"

int main()
{
    // ocMember represents the interface to the IPC system
    ocMember member = ocMember(ocMemberId::Sign_Detection, "Traffic Sign Detection Process");

    // Creating an ocMember alone is not enough. This call will make the
    // ocMember try to connect to the IPC system.
    member.attach();

    // Some functionality of the ocMember is put in separate types. We grab
    // pointers to them here so we don't have to call the getters every time.
    ocIpcSocket* socket = member.get_socket();
    ocLogger*    logger = member.get_logger();

    SignDetector::Init(socket, logger);

    while (true)
    {
        logger->log("No Signs detected.");
    }

    return 0;
}
