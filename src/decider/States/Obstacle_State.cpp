#include <thread>
#include <chrono>
#include "Obstacle_State.h"
#include "Normal_Drive.h"
#include "../Driver.h"


State& Obstacle_State::get_instance(){    
    static Obstacle_State singleton;
    return singleton;
}

void Obstacle_State::initialize(){
    if(!Obstacle_State::is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.set_sender(ocMemberId::Obstacle_State);
        sup.clear_and_edit()
            .write(ocMessageId::Object_Found);
        socket->send_packet(sup);
        logger->log("Decider: Obstacle_State: send subscribe packet");

        Obstacle_State::is_initialized = true;
    }
}



void Obstacle_State::on_entry(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Obstacle_State);
    
    Obstacle_State::initialize();

    deafen.clear_and_edit()
        .write(ocMemberId::Obstacle_State)
        .write(false);
    socket->send_packet(deafen);

    logger->log("Decider: Initialized Obstacle_State. Running the state next");

    Driver::stop();
    statemachine->run(nullptr);
}



void Obstacle_State::run(Statemachine* statemachine, void* data){
    ocPacket recv_packet;
    bool object_found = true;
    bool object_found_again = true;

    
    while (object_found_again) {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        int result = socket->read_packet(recv_packet, false);

        if (result < 0) {
            logger->error("Decider: Obstacle_State: Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {
            switch (recv_packet.get_message_id()){
                case ocMessageId::Object_Found:{
                    recv_packet.read_from_start();
                    object_found = true; 
                }break;

                default:{
                    if(!object_found){
                        object_found_again = false;
                    }
                    object_found = false; 
                }break;
            }
        }
    }
    
    logger->log("Decider: Obstacle_State: Changing state from Obstacle_State to Normal_Drive");
    statemachine->change_state(Normal_Drive::get_instance()); 
}



void Obstacle_State::on_exit(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Obstacle_State);
    deafen.clear_and_edit()
        .write(ocMemberId::Obstacle_State)
        .write(true);
    socket->send_packet(deafen);
}



