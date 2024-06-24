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

static double CalcObstacleCoverage(const cv::Mat& camData, int32_t width, int32_t height)
{
    int totalCount = 0, obstaclePixelCount = 0;

    for (int row = 0; row < camData.rows; row++) {
        for (int col = 0; col < camData.cols; col++) {
            cv::Vec3b pixel = camData.at<cv::Vec3b>(row, col);

            uchar B = pixel[0];
            uchar G = pixel[1];
            uchar R = pixel[2];

            if (G > R + B) {
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
    bool supportGUI = true;
    std::vector<std::string> params{argv, argv+argc};
    for (auto i : params)
    {
        if (i == "--nogui")
        {
            supportGUI = false;
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

        logger->warn(std::to_string(CalcObstacleCoverage(cam_image, cam_data->width, cam_data->height)).c_str());
    }

    logger->warn("Obstacle-Detection: Process Shutdown.");

    return 0;
}
