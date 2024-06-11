#include "Approaching_Crossing.h"
#include <math.h>


State& Approaching_Crossing::get_instance(){
    static Approaching_Crossing singleton;
    return singleton;
}

void Approaching_Crossing::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.clear_and_edit()
            .write(ocMessageId::Driving_Task_Finished);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



void Approaching_Crossing::on_entry(Statemachine* statemachine){
    initialize();

    statemachine->run(nullptr);
}


void Approaching_Crossing::run(Statemachine* statemachine, void* data){
    /*
    
    bool is_at_crossing = false;
    ocPacket recv_packet;
    uint32_t threshold = 2;

    while (!is_at_crossing) {
       
        int result = socket->read_packet(recv_packet);
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
            break;
        }

        switch (recv_packet.get_message_id())
        {
        case ocMessageId::Traffic_Sign_Detected:{
            auto reader = recv_packet.read_from_start();
            uint32_t distance = reader.read<uint32_t>();
            }break;
        
        default:{
            ocMessageId msg_id = recv_packet.get_message_id();
            ocMemberId mbr_id = recv_packet.get_sender();
            logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
            }break;
        }

        }
        //algorithm for slowing down towards 2cm/s
        double* arr = smooth_speed(curr_speed);
        for(int i = 0; i < 100; i++) {
            drive_forward(arr[i]);
            delay(x)
        }

        if(distance <= threshold) {
            is_at_crossing = true;
        }

    }

    statemachine->change_state(Is_At_Crossing::getInstance());
    */
}



void Approaching_Crossing::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}

double* smooth_speed(int current_speed) {
    int range = 100;
    int nzero = current_speed;
    float a = 0.98; //0<a<1 current: 2%
    int y = 0;

    double results[range];

    for(int i = 0; i < range; i++) {
        y = nzero * pow(a, i);
        results[i] = y;
        //drive_forward(y);
    }

    return results;

}