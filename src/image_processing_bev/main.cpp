#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <csignal>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>
using namespace cv;

static bool running = true;
ocLogger *logger;

Point2f src_vertices[4];
Point2f dst_vertices[4];
Mat M;

static void signal_handler(int)
{
    running = false;
}

void initializeTransformParams() {
    src_vertices[0] = Point2f(130,190);
    src_vertices[1] = Point2f(270,190);
    src_vertices[2] = Point2f(1200, 400);
    src_vertices[3] = Point2f(-800, 400);

    dst_vertices[0] = Point2f(0, 0);
    dst_vertices[1] = Point2f(400, 0);
    dst_vertices[2] = Point2f(400, 400);
    dst_vertices[3] = Point2f(0, 400);

    M = getPerspectiveTransform(src_vertices, dst_vertices);
}

void toBirdsEyeView(Mat &src, Mat &dst) {
    warpPerspective(src, dst, M, dst.size());
}

int main() {
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    

    ocMember member(ocMemberId::Image_Processing, "Image_Processing");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    logger = member.get_logger();
    ocSharedMemory *shared_memory = member.get_shared_memory();

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Camera_Image_Available);
    socket->send_packet(ipc_packet);

    initializeTransformParams();

    // Listen for Camera Image Available Message on IPC

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

                        // Move to shared memory

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

                        // Convert img from color to bw
                        convert_to_gray_u8(tempCamData->pixel_format, tempCamData->img_buffer, tempCamData->width, tempCamData->height, shared_memory->bev_data[0].img_buffer, 400, 400);

                        shared_memory->last_written_bev_data_index = 0;

                        // Apply birds eye view

                        Mat src(400, 400, CV_8UC1, shared_memory->bev_data[0].img_buffer);
                        Mat dst(400, 400, CV_8UC1, shared_memory->bev_data[0].img_buffer);

                        toBirdsEyeView(src, dst);

                        // notify others about available picture

                        ipc_packet.set_sender(ocMemberId::Image_Processing);
                        ipc_packet.set_message_id(ocMessageId::Birdseye_Image_Available);
                        socket->send_packet(ipc_packet);
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