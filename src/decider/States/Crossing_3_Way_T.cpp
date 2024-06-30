#include "Crossing_3_Way_T.h"
#include "Normal_Drive.h"
#include "../../traffic_sign_detection/TrafficSign.h"


State& Crossing_3_Way_T::get_instance(){
    static Crossing_3_Way_T singleton;
    return singleton;
}

void Crossing_3_Way_T::initialize(){
    if(!is_initialized){
        member.attach();
        socket = member.get_socket();
        logger = member.get_logger();
        ocPacket sup = ocPacket(ocMessageId::Subscribe_To_Messages);
        sup.set_sender(ocMemberId::Crossing_3_Way_T);
        sup.clear_and_edit()
            .write(ocMessageId::Traffic_Sign_Detected);
            //.write(ocMessageId::Object_Found);
        socket->send_packet(sup);

        is_initialized = true;
    }
}



void Crossing_3_Way_T::on_entry(Statemachine* statemachine){
    /*
    Get IPC-hub messages regarding traffic signs;
    Create array of traffic-signs (types and distances);
    Array an statemachine->run übergeben;

    //Bekommen Array vom struct übergeben
    struct TrafficSign{
        TrafficSignType trafficSign;
        uint64_t distance;
    };

    */
    initialize();
    ocPacket recv_packet;

    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Crossing_3_Way_T);
    deafen.clear_and_edit()
        .write(ocMemberId::Crossing_3_Way_T)
        .write(false);
    socket->send_packet(deafen);

    bool received_sign_package = false;
    
    for(int i = 0; i < 10 && !received_sign_package; i++){
        int result = socket->read_packet(recv_packet, false);
        ocTime now = ocTime::now();

        if (result < 0) {
            logger->error("Decider: Crossing_3_Way_T: Error reading the IPC socket: (%i) %s", errno, strerror(errno));
        } else {
            switch (recv_packet.get_message_id()){
                case ocMessageId::Traffic_Sign_Detected:{
                    auto reader = recv_packet.read_from_start();
                    uint16_t rawValue = reader.read<uint16_t>();
                    distance = reader.read<uint64_t>();
                    trafficSign = static_cast<TrafficSignType>(rawValue);
                    received_sign_package = true;
                } break;
                
                default:{
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId mbr_id = recv_packet.get_sender();
                    logger->warn("Decider: Crossing_3_Way_T: Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                } break;
            }
        }
    }

    statemachine->run(nullptr);    
    
    
}


void Crossing_3_Way_T::run(Statemachine* statemachine, void* data){

    bool drive_left = false;
    bool drive_right = false;

    if (distance < 50){ //50cm == width of crossing; If distance larger, than sign is irrelevant for crossing
        switch(trafficSign){
            case TrafficSignType::Stop:{
                Driver::stop(2); //stop for 2s
                }break;
            case TrafficSignType::PriorityRoad:{
                drive_right = true;
                }break;
            case TrafficSignType::Left:{
                drive_left = true;
                }break;
            case TrafficSignType::Right:{
                drive_right = true;
                }break;
            default:
                logger->warn("Decider: Crossing_3_Way_T: No sign detected");
                break;
        }
        logger->error("Decider: TrafficSign: %s", TrafficSignTypeToString(trafficSign).c_str());
    }

    if(drive_left && drive_right){
        drive_left = false;
    }

    /*
    bool object_found = true;
    while(object_found){
        ocPacket recv_packet;
        int result = socket->read_packet(recv_packet);
        if (result >= 0) {
            switch (recv_packet.get_message_id()){
                case ocMessageId::Object_Found:{
                    object_found = true;
                } break;
                
                default:{
                    object_found = false;
                } break;
            }
        }
    }
    */

    if(drive_left){
        Driver::turn_left();
    } else if (drive_right){
        Driver::turn_right();
    } else{
        Driver::turn_right();
    }

    logger->log("Decider: Crossing_3_Way_T: Changing state from Crossing_3_Way_T to Normal_Drive");
    statemachine->change_state(Normal_Drive::get_instance());
    
}



void Crossing_3_Way_T::on_exit(Statemachine* statemachine){
    ocPacket deafen = ocPacket(ocMessageId::Deafen_Member);
    deafen.set_sender(ocMemberId::Crossing_3_Way_T);
    deafen.clear_and_edit()
        .write(ocMemberId::Crossing_3_Way_T)
        .write(true);
    socket->send_packet(deafen);

    distance = 0;
    trafficSign = TrafficSignType::None;
}
