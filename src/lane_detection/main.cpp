#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include "../common/ocCar.h"
#include "../common/ocCarConfig.h"
#include <signal.h>
#include <vector>
#include <unistd.h>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <tensorflow/lite/op_resolver.h>
#include <tensorflow/lite/string_util.h>
#include <tensorflow/lite/tools/gen_op_registration.h>

#include "../common/ocWindow.h"

#include <opencv2/opencv.hpp>

#define CAR_CONFIG_FILE "../car_properties.conf"

#define DRAW_LINE_SAMPLES

#define DEBUG_WINDOW

#include <tensorflow/lite/model.h>
#include <tensorflow/lite/optional_debug_tools.h>

class Lanes {
public:
    std::vector<cv::Mat> recent_fit;
    cv::Mat avg_fit;

    Lanes() {
        avg_fit = cv::Mat::zeros(cv::Size(160, 80), CV_32FC3);
    }
};

Lanes lanes;

cv::Mat road_lines(const cv::Mat &image, tflite::Interpreter* interpreter) {
    // Resize image
    cv::Mat small_img;
    cv::resize(image, small_img, cv::Size(160, 80));

    // Prepare the input data
    small_img.convertTo(small_img, CV_32FC3);
    std::vector<float> input_data(small_img.reshape(1, 1));

    // Get input and output tensors
    int input_index = interpreter->inputs()[0];
    int output_index = interpreter->outputs()[0];

    // Set the tensor to point to the input data to be inferred
    float* input_tensor_data = interpreter->typed_tensor<float>(input_index);
    std::copy(input_data.begin(), input_data.end(), input_tensor_data);

    // Run the inference
    interpreter->Invoke();

    // Get the output data
    float* output_tensor_data = interpreter->typed_tensor<float>(output_index);
    std::vector<float> output_data(output_tensor_data, output_tensor_data + (160 * 80 * 3));
    
    cv::Mat prediction(80, 160, CV_32FC3, output_data.data());
    prediction *= 255.0;

    // Append prediction to recent_fit
    lanes.recent_fit.push_back(prediction);

    // Keep only the last 5 predictions
    if (lanes.recent_fit.size() > 5) {
        lanes.recent_fit.erase(lanes.recent_fit.begin());
    }

    // Calculate average of recent_fit
    lanes.avg_fit = cv::Mat::zeros(prediction.size(), CV_32FC3);
    for (const auto& mat : lanes.recent_fit) {
        lanes.avg_fit += mat;
    }
    lanes.avg_fit /= lanes.recent_fit.size();
    lanes.avg_fit.convertTo(lanes.avg_fit, CV_8UC3);

    // Create lane drawn image
    cv::Mat blanks = cv::Mat::zeros(lanes.avg_fit.size(), CV_8UC3);
    std::vector<cv::Mat> channels(3);
    cv::split(blanks, channels);
    channels[1] = lanes.avg_fit;
    cv::merge(channels, blanks);

    // Resize lane drawn image
    cv::Mat lane_drawn;
    cv::resize(blanks, lane_drawn, cv::Size(400, 400));

    // Add lanes to original image
    cv::Mat result;
    cv::addWeighted(image, 1, lane_drawn, 1, 0, result);

    return result;
}

ocMember member(ocMemberId::Lane_Detection_Values, "Lane Detection");

ocLogger *logger;
ocPacket ipc_packet;
ocIpcSocket *socket;
ocSharedMemory *shared_memory;
ocCarProperties car_properties;

std::list<int> last_angles; 

static bool running = true;
bool onStreet = true;
int onStreetCount = 0;

static void signal_handler(int)
{
    running = false;
}

int main()
{
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    member.attach();

    socket = member.get_socket();
    shared_memory = member.get_shared_memory();
    logger = member.get_logger();

    read_config_file(CAR_CONFIG_FILE, car_properties, *logger);

    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Birdseye_Image_Available);
    socket->send_packet(ipc_packet);

    const char* model_path = "model.tflite";
    std::unique_ptr<tflite::FlatBufferModel> model = tflite::FlatBufferModel::BuildFromFile(model_path);
    if (!model) {
        std::cerr << "Failed to load model: " << model_path << std::endl;
        return -1;
    }

    // Build the interpreter
    tflite::ops::builtin::BuiltinOpResolver resolver;
    std::unique_ptr<tflite::Interpreter> interpreter;
    if (tflite::InterpreterBuilder(*model, resolver)(&interpreter) != kTfLiteOk) {
        std::cerr << "Failed to build interpreter." << std::endl;
        return -1;
    }

    // Allocate tensor buffers
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        std::cerr << "Failed to allocate tensors." << std::endl;
        return -1;
    }

    logger->log("Lane Detection started!");

    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Birdseye_Image_Available:
                {
                    cv::Mat matrix = cv::Mat(400,400,CV_8UC1, shared_memory->cam_data[0].img_buffer);

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::imwrite("bev.jpg", matrix);
                    } 

                    if(std::getenv("CAR_ENV") != NULL) {
                        cv::Mat result = road_lines(matrix, interpreter.get());
                        cv::imwrite("bev_out.jpg", result);
                    } 
                    return;
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
}