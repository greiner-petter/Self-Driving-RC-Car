#include "SignDetector.h"


#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

static ocIpcSocket* s_Socket = nullptr;
static ocSharedMemory* s_SharedMemory = nullptr;
static ocLogger* s_Logger = nullptr;

void SignDetector::Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger)
{
    logger->log("SignDetector::Init()");
    s_Socket = socket;
    s_SharedMemory = shared_memory;
    s_Logger = logger;
    Run();
}

std::filesystem::path SignDetector::GetStopSignXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_stop_sign.xml";
}

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

void SignDetector::Run()
{
    // Load stop sign cascade classifier
    cv::CascadeClassifier stop_sign;
    stop_sign.load(GetStopSignXML().string());

    cv::VideoCapture cap(0);

    while (cap.isOpened())
    {
        ocCamData *cam_data = &s_SharedMemory->cam_data[s_SharedMemory->last_written_cam_data_index];

        int type = CV_8UC1;
        if (3 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC3;
        if (4 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC4;
        if (12 == bytes_per_pixel(cam_data->pixel_format)) type = CV_32FC3;

        cv::Mat cam_image((int)cam_data->height, (int)cam_data->width, type);
        cam_image.data = cam_data->img_buffer;


        cv::Mat img;
        cap.read(img);
        cv::Mat gray;
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

        std::vector<cv::Rect> stop_sign_scaled;
        stop_sign.detectMultiScale(gray, stop_sign_scaled, 1.3, 5);

        // Detect the stop sign, x,y = origin points, w = width, h = height
        for (size_t i = 0; i < stop_sign_scaled.size(); i++) {
            cv::Rect roi = stop_sign_scaled[i];
            // Draw rectangle around the stop sign
            cv::rectangle(img, cv::Point(roi.x, roi.y),
                          cv::Point(roi.x + roi.width, roi.y + roi.height),
                          cv::Scalar(0, 255, 0), 3);
            // Write "Stop sign" on the bottom of the rectangle
            cv::putText(img, "Stop Sign", cv::Point(roi.x, roi.y + roi.height + 30),
                        cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_4);

            s_Logger->log("Stop Sign!");
        }
        cv::imshow("img", img);
        char key = cv::waitKey(30);
        if (key == 'q') {
            cap.release();
            cv::destroyAllWindows();
            break;
        }
    }

}