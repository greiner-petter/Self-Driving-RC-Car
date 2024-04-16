#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <csignal>

static bool running = true;

static void signal_handler(int)
{
    running = false;
}

int main() {
    // Catch some signals to allow us to gracefully shut down the camera
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    ocMember member(ocMemberId::Lane_Detection, "Lane_Detection");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocLogger *logger = member.get_logger();
    ocSharedMemory *shared_memory = member.get_shared_memory();

    logger->log("Subscribing to camera image");

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Camera_Image_Available);
    socket->send_packet(ipc_packet);

    logger->log("Waiting for camera image...");

    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
                {
                    case ocMessageId::Camera_Image_Available:
                    {
                        if(shared_memory->last_written_cam_data_index != 0) {
                            continue;
                        }

                        logger->log("Received camera image");

                        ocTime frameTime;
                        uint32_t frameNumber;
                        ptrdiff_t memoryAdressOffset;
                        size_t dataSize;

                        ipc_packet.read_from_start()
                            .read<ocTime>(&frameTime)
                            .read<uint32_t>(&frameNumber)
                            .read<ptrdiff_t>(&memoryAdressOffset)
                            .read<size_t>(&dataSize);
                        
                        ocCamData* tempCamData = (ocCamData*) ((std::byte *)shared_memory + memoryAdressOffset);

                        shared_memory->bev_data[0] = (ocBevData) {
                            tempCamData->frame_time,
                            tempCamData->frame_number,
                            0,(int32_t) 400,
                            0,(int32_t) 400,
                            {}
                        };

                        convert_bgra_u8_to_gray_u8(tempCamData->img_buffer, tempCamData->width, tempCamData->height, shared_memory->bev_data[0].img_buffer, 400, 400);

                        shared_memory->last_written_bev_data_index = 0;

                        ipc_packet.set_message_id(ocMessageId::Birdseye_Image_Available);
                        socket->send_packet(ipc_packet);

                        //free(camData);
                    } break;
                    default:
                    {
                        ocMessageId msg_id = ipc_packet.get_message_id();
                        ocMemberId  mbr_id = ipc_packet.get_sender();
                        logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                    } break;
                }
        }
    }

    return 0;
}