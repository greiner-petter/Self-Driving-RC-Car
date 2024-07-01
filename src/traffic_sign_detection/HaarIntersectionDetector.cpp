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


std::filesystem::path HaarIntersectionDetector::GetCrossingLeftXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_crossing_left.xml";
}
std::filesystem::path HaarIntersectionDetector::GetCrossingRightXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_crossing_right.xml";
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
    //s_Instances.push_back(std::make_shared<ClassifierInstance>(GetCrossingLeftXML().string(), "Crossing Left", 0b110));
    s_Instances.push_back(std::make_shared<ClassifierInstance>(GetCrossingRightXML().string(), "Crossing Right", 0b110));
}

bool HaarIntersectionDetector::Tick()
{
    cv::Mat image(400, 400, CV_8UC1, s_SharedMemory->bev_data[0].img_buffer);

    // Iterate over all the XML Classifier Instances and detect the signs
    for (auto& signClassifier : s_Instances)
    {
        std::vector<cv::Rect> sign_scaled;
        signClassifier->classifier.detectMultiScale(image, sign_scaled, 1.3, 5);

        // Detect the sign, x,y = origin points, w = width, h = height
        for (size_t i = 0; i < sign_scaled.size(); i++)
        {
            cv::Rect roi = sign_scaled[i];
            const uint32_t distance = std::max(200 - roi.y, 0);

            if (s_SupportGUI)
            {
                cv::rectangle(image, cv::Point(roi.x, roi.y),
                            cv::Point(roi.x + roi.width, roi.y + roi.height),
                            cv::Scalar(0, 255, 0), 3);
                cv::putText(image, "Found " + signClassifier->label + " Intersection", cv::Point(roi.x, roi.y + roi.height + 30),
                        cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_4);
            }
            s_Logger->log("Found Traffic Intersection %s in distance: %03d  seenLastFrame:%d", signClassifier->label.c_str(), distance, (int)signClassifier->seenLastFrame);
            if (signClassifier->seenLastFrame)
            {
                
            }
        }
        signClassifier->seenLastFrame = sign_scaled.size() != 0;
    }

    if (s_SupportGUI) 
    {
        cv::imshow("Traffic Intersection Detection", image);
        char key = cv::waitKey(30);
        if (key == 'q')
        {
            cv::destroyAllWindows();
            return false;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
        
    return true;
}