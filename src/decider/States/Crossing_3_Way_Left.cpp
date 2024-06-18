#include "Crossing_3_Way_Left.h"
#include "Normal_Drive.h"
#include "../../traffic_sign_detection/TrafficSign.h"


State& Crossing_3_Way_Left::get_instance(){
    static Crossing_3_Way_Left singleton;
    return singleton;
}

void Crossing_3_Way_Left::initialize(){
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



void Crossing_3_Way_Left::on_entry(Statemachine* statemachine){
    /*
    Get IPC-hub messages regarding traffic signs;
    Create array of traffic-signs (types and distances);
    Array an statemachine->run übergeben;
    */
    initialize();
    ocPacket recv_packet;

    while (true) {
       
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
            uint16_t rawValue = reader.read<uint16_t>();
            trafficSign = static_cast<TrafficSignType>(rawValue);
            } break;
        
        default:{
            ocMessageId msg_id = recv_packet.get_message_id();
            ocMemberId mbr_id = recv_packet.get_sender();
            logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
            }break;
        }

    }

    statemachine->run(nullptr);
}


void Crossing_3_Way_Left::run(Statemachine* statemachine, void* data){

    bool drive_left = false;
    bool drive_forward = false;

    /*
    if (trafficSign.distanceCM < 50){ //50cm == width of crossing; If distance larger, than sign is irrelevant for crossing
        switch(trafficSign.type){
            case TrafficSignType::Stop:
                Driver::stop(2000); //stop for 2s
                break;
            case TrafficSignType::PriorityRoad:
                drive_forward = true;
                break;
            case TrafficSignType::Left:
                drive_left = true;
                break;
            case TrafficSignType::Right:
                drive_left = true;
                break;
        }
    }
    */

    if(drive_left && drive_forward){
        drive_left = false;
    }

    //if obstacle, stop

    // FIXME: Compiler Error
#if 0
    if(drive_left){
        Driver::turn_left();
    } else if (drive_forward){
        Driver::drive_forward();
    } else{
        Driver::drive_forward();
    }

    statemachine->change_state(Normal_Drive::get_instance());
}



void Crossing_3_Way_Left::on_exit(Statemachine* statemachine){
    /*
    Code
    */
}
