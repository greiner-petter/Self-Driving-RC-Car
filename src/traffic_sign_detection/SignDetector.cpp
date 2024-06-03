#include "SignDetector.h"

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

void SignDetector::Init(ocIpcSocket* socket, ocSharedMemory* shared_memory, ocLogger* logger, bool supportGUI)
{
    logger->log("SignDetector::Init()");
    s_Socket = socket;
    s_SharedMemory = shared_memory;
    s_Logger = logger;
    s_SupportGUI = supportGUI;
    Run();
}

std::filesystem::path SignDetector::GetStopSignXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_stop_sign.xml";
}
std::filesystem::path SignDetector::GetLeftSignXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_left_sign.xml";
}
std::filesystem::path SignDetector::GetRightSignXML()
{
    return std::filesystem::current_path().parent_path() / "res" / "cascade_right_sign.xml";
}


struct ClassifierInstance
{
    cv::CascadeClassifier classifier;
    std::string label;
    TrafficSignType type;
    std::function<float(float)> convertToDistanceLambda;

    ClassifierInstance(const std::string& path, const std::string& signLabel, TrafficSignType signType, double sizeFactor)
    {
        classifier.load(path);
        label = signLabel;
        type = signType;
        convertToDistanceLambda = [sizeFactor](float rectSize) {
            return std::max(SignDetector::Remap<float>(rectSize, 0.0f, sizeFactor, 100.0f, 0.0f), 0.0f);
        };
    }
};

// Converts into an estimated distance
// static float ConvertRectSizeToEstimatedDistance(float rectSize)
// {
//     return std::max(SignDetector::Remap<float>(rectSize, 0.0f, 0.3f, 100.0f, 0.0f), 0.0f);
// }
static uint32_t ConvertRectToDistanceInCM(const cv::Rect& rect, const int cam_width, const int cam_height, const std::function<float(float)>& convertToDistanceLambda)
{
    return static_cast<uint32_t>(convertToDistanceLambda((static_cast<float>(rect.width) / (float)cam_width + static_cast<float>(rect.height) / (float)cam_height) / 2.0f));
}

static std::vector<std::shared_ptr<ClassifierInstance>> s_Instances;

void SignDetector::Run()
{
    // Load sign cascade classifiers
    s_Instances.push_back(std::make_shared<ClassifierInstance>(GetStopSignXML().string(), "Stop", TrafficSignType::Stop, 0.3));
    s_Instances.push_back(std::make_shared<ClassifierInstance>(GetLeftSignXML().string(), "Left", TrafficSignType::Left, 0.2));
    s_Instances.push_back(std::make_shared<ClassifierInstance>(GetRightSignXML().string(), "Right", TrafficSignType::Right, 0.2));

    while (true)
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
                const uint32_t distance = ConvertRectToDistanceInCM(roi, (int)cam_data->width, (int)cam_data->height, signClassifier->convertToDistanceLambda);

                // Draw rectangle around the sign
                cv::rectangle(cam_image, cv::Point(roi.x, roi.y),
                              cv::Point(roi.x + roi.width, roi.y + roi.height),
                              cv::Scalar(0, 255, 0), 3);
                // Write Text on the bottom of the rectangle
                cv::putText(cam_image, "Found " + signClassifier->label + " Sign", cv::Point(roi.x, roi.y + roi.height + 30),
                            cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2, cv::LINE_4);

                s_Logger->log("Found %s Sign in distance: %d", signClassifier->label.c_str(), distance);
            }
        }

        if (s_SupportGUI) 
        {
            cv::imshow("Traffic Sign Detection", cam_image);
            char key = cv::waitKey(30);
            if (key == 'q')
            {
                cv::destroyAllWindows();
                break;
            }
        }
        
    }

}

void SignDetector::SendPacket(TrafficSign sign)
{
    ocPacket s(ocMessageId::Traffic_Sign_Detected);
    s.clear_and_edit().write((uint16_t)sign.type).write(sign.distanceCM);
    s_Socket->send_packet(s);
}