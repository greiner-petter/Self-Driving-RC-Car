#include "HaarIntersectionDetector.h"

#include <chrono>
#include <thread>

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

static ocIpcSocket* s_Socket = nullptr;
static ocSharedMemory* s_SharedMemory = nullptr;
static ocLogger* s_Logger = nullptr;
static bool s_SupportGUI = false;


std::filesystem::path HaarSignDetector::GetCrossingLeftXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_stop_sign.xml";
}


struct ClassifierInstance
{
    cv::CascadeClassifier classifier;
    int8_t result;
    std::string label;
    bool seenLastFrame = false;

    ClassifierInstance(const std::string& path, const std::string& label, int8_t result)
    {
        classifier.load(path);
        label = signLabel;
        type = signType;
        this->label = label;
        this->result = result;
    }
};

static std::vector<std::shared_ptr<ClassifierInstance>> s_Instances;

void HaarIntersectionDetector::Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger, bool supportGUI)
{
    SignDetector::Init(socket, shared_memory, logger, supportGUI);
    logger->log("SignDetector::Init()");
    s_Socket = socket;
    s_SharedMemory = shared_memory;
    s_Logger = logger;
    s_SupportGUI = supportGUI;

    // Load sign cascade classifiers
    s_Instances.push_back(std::make_shared<ClassifierInstance>(GetCrossingLeftXML().string(), "Crossing Left", 0b110));
}

void HaarIntersectionDetector::Tick()
{
    // Fetch Camera Data
    ocCamData *cam_data = &s_SharedMemory->cam_data[s_SharedMemory->last_written_cam_data_index];

    int type = CV_8UC1;
    if (3 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC3;
    if (4 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC4;
    if (12 == bytes_per_pixel(cam_data->pixel_format)) type = CV_32FC3;

    cv::Mat cam_image((int)cam_data->height, (int)cam_data->width, type);
    cam_image.data = cam_data->img_buffer;

    cv::Mat gray;
    cv::cvtColor(cam_image, gray, cv::COLOR_BGR2GRAY);

    // Iterate over all the XML Classifier Instances and detect the signs
    for (auto& signClassifier : s_Instances)
    {
        std::vector<cv::Rect> sign_scaled;
        signClassifier->classifier.detectMultiScale(gray, sign_scaled, 1.3, 5);

        // Detect the sign, x,y = origin points, w = width, h = height
        for (size_t i = 0; i < sign_scaled.size(); i++)
        {
            cv::Rect roi = sign_scaled[i];
            const uint32_t distance = y;

            if (distance <= 8) continue;

            if (s_SupportGUI)
            {
                cv::rectangle(cam_image, cv::Point(roi.x, roi.y),
                            cv::Point(roi.x + roi.width, roi.y + roi.height),
                            cv::Scalar(0, 255, 0), 3);
                cv::putText(cam_image, "Found " + signClassifier->label + " Sign", cv::Point(roi.x, roi.y + roi.height + 30),
                        cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_4);
            }
            s_Logger->log("Found Traffic Intersection in distance: %03d  seenLastFrame:%d", distance, (int)signClassifier->seenLastFrame);
            if (signClassifier->seenLastFrame)
            {
                
            }
        }
        signClassifier->seenLastFrame = sign_scaled.size() != 0;
    }

    if (s_SupportGUI) 
    {
        cv::imshow("Traffic Intersection Detection", cam_image);
        char key = cv::waitKey(30);
        if (key == 'q')
        {
            cv::destroyAllWindows();
            break;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
        
    

}