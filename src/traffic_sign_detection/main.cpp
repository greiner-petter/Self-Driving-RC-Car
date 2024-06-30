#include <iostream>
#include "../common/ocMember.h"

#include "SignDetector.h"
#include "HaarSignDetector.h"
#include <chrono>
#include <thread>
#include <string>
#include <vector>

int main(int argc,char* argv[])
{
    SignDetector* detector = nullptr;

    // Parsing Params
    bool supportGUI = true;
    std::vector<std::string> params{argv, argv+argc};
    for (auto i : params)
    {
        if (i == "--nogui" || i == "--headless")
        {
            supportGUI = false;
        }
    }
    detector = new HaarSignDetector();

    std::this_thread::sleep_for(std::chrono::milliseconds(8000));
    // ocMember represents the interface to the IPC system
    ocMember member = ocMember(ocMemberId::Sign_Detection, "Traffic-Sign-Detection Process");

    // Creating an ocMember alone is not enough. This call will make the
    // ocMember try to connect to the IPC system.
    member.attach();

    // Some functionality of the ocMember is put in separate types. We grab
    // pointers to them here so we don't have to call the getters every time.
    ocIpcSocket* socket = member.get_socket();
    ocSharedMemory* shared_memory = member.get_shared_memory();
    ocLogger*    logger = member.get_logger();

    detector->Init(socket, shared_memory, logger, supportGUI);

    logger->warn("Traffic-Sign-Detection: Process Shutdown.");

    return 0;
}
