#include <iostream>
#include "../common/ocMember.h"

#include <chrono>
#include <thread>
#include <string>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#define THRESHOLD 0.20

static double CalcObstacleCoverage(const cv::Mat& camData)
{
    int totalCount = 0, obstaclePixelCount = 0;

    for (int row = camData.rows / 3; row < camData.rows / 3 * 2; row++) {
        for (int col = camData.cols / 3; col < camData.cols / 3 * 2; col++) {
            cv::Vec3b pixel = camData.at<cv::Vec3b>(row, col);

            uchar B = pixel[0];
            uchar G = pixel[1];
            uchar R = pixel[2];

            if (G > std::max(R, B)) {
                obstaclePixelCount++;
            }
            totalCount++;
        }
    }

    return (double)obstaclePixelCount / (double)totalCount;
}

int main(int argc,char* argv[])
{
    // Parsing Params
    bool verbose = false;
    std::vector<std::string> params{argv, argv+argc};
    for (auto i : params)
    {
        if (i == "--verbose" || i == "-v")
        {
            verbose = true;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(8000));
    // ocMember represents the interface to the IPC system
    ocMember member = ocMember(ocMemberId::Obstacle_Detection, "Obstacle-Detection Process");

    // Creating an ocMember alone is not enough. This call will make the
    // ocMember try to connect to the IPC system.
    member.attach();

    // Some functionality of the ocMember is put in separate types. We grab
    // pointers to them here so we don't have to call the getters every time.
    ocIpcSocket* socket = member.get_socket();
    ocSharedMemory* shared_memory = member.get_shared_memory();
    ocLogger*    logger = member.get_logger();

    while (true)
    {
        // Fetch Camera Data
        ocCamData *cam_data = &shared_memory->cam_data[shared_memory->last_written_cam_data_index];

        int type = CV_8UC1;
        if (3 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC3;
        if (4 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC4;
        if (12 == bytes_per_pixel(cam_data->pixel_format)) type = CV_32FC3;

        cv::Mat cam_image((int)cam_data->height, (int)cam_data->width, type);
        cam_image.data = cam_data->img_buffer;

        const double percent = CalcObstacleCoverage(cam_image);

        if (percent >= THRESHOLD)
        {
            ocPacket s(ocMessageId::Object_Found);
            s.clear_and_edit().write(percent);
            socket->send_packet(s);
            logger->warn((std::string("Obstacle detected: ") + std::to_string(percent)).c_str());
        }
        if (verbose)
        {
            logger->warn(std::to_string(percent).c_str());
        }

    }

    logger->warn("Obstacle-Detection: Process Shutdown.");

    return 0;
}
