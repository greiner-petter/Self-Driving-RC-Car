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
}

// Converts into an estimated distance
float SignDetector::ConvertRectSizeToEstimatedDistance(float rectSize, double sizeFactor)
{
    return std::max(SignDetector::Remap<float>(rectSize, 0.0f, sizeFactor, 100.0f, 0.0f), 0.0f);
}
uint32_t SignDetector::ConvertRectToDistanceInCM(const cv::Rect& rect, const int cam_width, const int cam_height, double sizeFactor)
{
    return static_cast<uint32_t>(ConvertRectSizeToEstimatedDistance((static_cast<float>(rect.width) / (float)cam_width + static_cast<float>(rect.height) / (float)cam_height) / 2.0f, sizeFactor));
}

void SignDetector::SendPacket(TrafficSign sign)
{
    ocPacket s(ocMessageId::Traffic_Sign_Detected);
    s.clear_and_edit().write((uint16_t)sign.type).write(sign.distanceCM);
    s_Socket->send_packet(s);
}