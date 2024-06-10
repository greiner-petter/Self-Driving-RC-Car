#include "../common/ocTypes.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <csignal>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

// uncomment to use on systems that have no CUDA
// #define FORBID_CUDA

// if this is set the BEV is redrawn only with the
// detected lines for easier debugging/visuliazation. This
// is normally disabled since it requires an entire Matrix copy
// and adds other graphical overhead by the drawing itself

//#define DRAW_POLYLINES_ON_EMPTY_OUTPUT


// use cuda acceleration by default
#ifndef FORBID_CUDA
using namespace cv::cuda;
#else
using namespace cv;
#endif

using cv::Point2f;
using cv::Mat;
using cv::Point;

using cv::RETR_LIST;
using cv::CHAIN_APPROX_NONE;
using cv::Size_;

using std::vector;

static bool running = true;
ocLogger *logger;

Point2f src_vertices[4];
Point2f dst_vertices[4];
Mat M;

static constexpr auto BLUR_SIZE = 7;
static constexpr auto POST_CANNY_BLUE_SIZE = 9;

static void signal_handler(int)
{
    running = false;
}

void initializeTransformParams() {
    src_vertices[0] = Point2f(70,210);
    src_vertices[1] = Point2f(330,210);
    src_vertices[2] = Point2f(780, 310);
    src_vertices[3] = Point2f(-380, 310);

    /*src_vertices[0] = Point2f(120,230);
    src_vertices[1] = Point2f(280,230);
    src_vertices[2] = Point2f(420, 370);
    src_vertices[3] = Point2f(-20, 370);*/


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

                        // TODO: Consider changing the internal implementation to use
                        // OpenCV. Currently it's a single threaded loop! Convert img
                        // Convert img from color to bw
                        convert_to_gray_u8(tempCamData->pixel_format, tempCamData->img_buffer, tempCamData->width, tempCamData->height, shared_memory->bev_data[0].img_buffer, 400, 400);

                        shared_memory->last_written_bev_data_index = 0;

                        // Apply birds eye view

                        Mat src(400, 400, CV_8UC1, shared_memory->bev_data[0].img_buffer);
                        Mat dst(400, 400, CV_8UC1, shared_memory->bev_data[0].img_buffer);

                        if(std::getenv("CAR_ENV") != NULL) {
                            cv::imwrite("cam_image.jpg", src);
                        } 

                        toBirdsEyeView(src, dst);

                        Mat dst2(400, 400, CV_8UC1, shared_memory->bev_data[1].img_buffer);

                        GaussianBlur(dst, dst, Size_(BLUR_SIZE, BLUR_SIZE), 0);

                        if(std::getenv("CAR_ENV") != NULL) {
                            cv::imwrite("cam_image_gaussian.jpg", dst);
                        } 

                        Canny(dst, dst, 30, 50, 3, true);

                        if(std::getenv("CAR_ENV") != NULL) {
                            cv::imwrite("cam_image_canny.jpg", dst);
                        } 

                        GaussianBlur(dst, dst, Size_(POST_CANNY_BLUE_SIZE, POST_CANNY_BLUE_SIZE), 0);

                        if(std::getenv("CAR_ENV") != NULL) {
                            cv::imwrite("cam_image_gaussian2.jpg", dst);
                        } 

                        dst.copyTo(dst2);

                        // notify others about available picture
                        ipc_packet.set_sender(ocMemberId::Image_Processing);
                        ipc_packet.set_message_id(ocMessageId::Birdseye_Image_Available);
                        socket->send_packet(ipc_packet);
                        
#ifndef hideContours
                        vector<vector<Point>> contours;
                        findContours(dst, contours, RETR_LIST, CHAIN_APPROX_NONE);
#ifdef DRAW_POLYLINES_ON_EMPTY_OUTPUT
                        Mat redrewed_image = Mat::zeros(dst2.size(), CV_8UC1);
#endif

                        vector<vector<Point>> cleaned_data;
                        for (auto &contour : contours) {
                            double len = cv::arcLength(contour, false);
                            if (len < 30) {
                                continue;
                            }
                            vector<Point> reduced_contour;
                            double epsilon = 0.007 * arcLength(contour, false);
                            approxPolyDP(contour, reduced_contour, epsilon, false);
                            cleaned_data.push_back(reduced_contour);
                        }

                        ipc_packet.set_sender(ocMemberId::Image_Processing);
                        ipc_packet.set_message_id(ocMessageId::Lines_Available);
                        ocBufferWriter writer = ipc_packet.clear_and_edit();
                        writer.write(cleaned_data.size());

                        for (size_t i = 0; i < cleaned_data.size(); ++i)
                        {
                            auto &contour = cleaned_data.at(i);
                            writer.write(contour.size());
                            for (size_t j = 0; j < contour.size(); ++j) {
                                Point &point = contour.at(j);
                                writer.write(point.x);
                                writer.write(point.y);
                            }
#ifdef DRAW_POLYLINES_ON_EMPTY_OUTPUT
                            cv::Scalar color = cv::Scalar(255);
                            polylines(redrewed_image, contour, false, color, 1,
                                      cv::LINE_8, 0);
#endif
                        }

                        // notify other about found lines in BEV
                        socket->send_packet(ipc_packet);


#ifdef DRAW_POLYLINES_ON_EMPTY_OUTPUT
                        redrewed_image.copyTo(dst2);
#endif
#endif
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
