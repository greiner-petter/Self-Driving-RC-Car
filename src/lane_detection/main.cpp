#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <vector>
#include <opencv2/opencv.hpp>

struct LineVectorData {
    float slope;
    float length;
    std::pair<float, float> closest_point;
    float distance;
    float weight;
};

ocLogger *logger;

static bool running = true;

static void signal_handler(int)
{
    running = false;
}

std::pair<float, float> linearRegression(int (data)[NUMBER_OF_POLYGONS_PER_CONTOUR][2], int count) {
    float x_mean;
    float y_mean;

    for(int j = 0; j < count; j++) {
        x_mean += data[j][0];
        y_mean += data[j][1];
    } 

    x_mean = x_mean / count;
    y_mean = y_mean / count;

    logger->log("test4.2");

    float numerator;
    float denominator;

    for(int j = 0; j < count; j++) {
        numerator += (data[j][0] - x_mean) * (data[j][1] - y_mean);
        denominator += (data[j][0] - x_mean) * (data[j][0] - x_mean);
    } 

    float slope = numerator/denominator;

    logger->log("test4.3");

    float y_intercept = y_mean - slope * x_mean;  

    return {slope, y_intercept};
};

float calcDist(std::pair<float, float> p1, std::pair<float, float> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
}

int main(int argc, const char **argv)
{
    // Catch some signals to allow us to gracefully shut down the process
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    ocMember member(ocMemberId::Lane_Detection, "Lane Detection");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();
    logger = member.get_logger();

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Lines_Available);
    socket->send_packet(ipc_packet);

    while(running) {
        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Lines_Available:
                {
                    logger->log("test");
                    //Find Lane
                    static struct ocBevLines lines;
                    ipc_packet.read_from_start().read(&lines);

                    LineVectorData vectors[lines.contour_num];

                    logger->log("test1");

                    for(int i = 0; i < lines.contour_num; i++) {
                        //do for every shape a linear regression to prepare a vector 

                        std::vector<float> vector;

                        std::pair<float, float> linReg = linearRegression((lines.lines[i]), (lines.poly_num[i]));

                        std::pair<float, float> closest_point = {0, 0}; 
                        std::pair<float, float> furthest_point = {199, 399};

                        float dist;
                        float longest_dist = 0;
                        float shortest_dist = 0;

                        for(int j = 0; j < lines.poly_num[i]; j++) {
                            dist = calcDist(((std::pair<float, float>){lines.lines[i][j][0], lines.lines[i][j][1]}), 
                                ((std::pair<float,float>){199, 399}));

                            if(dist < shortest_dist || j == 0) {
                                shortest_dist = dist;
                                closest_point = {lines.lines[i][j][0], lines.lines[i][j][1]};
                            }

                            if(dist > longest_dist || j == 0) {
                                longest_dist = dist;
                                furthest_point = {lines.lines[i][j][0], lines.lines[i][j][1]};
                            }
                        } 
                        
                        vectors[i] = {
                            .slope = std::get<0>(linReg)-90,
                            .distance = shortest_dist,
                            .closest_point = closest_point,
                            .length = calcDist(closest_point, furthest_point)
                        };
                    }

                    logger->log("test2");

                    float item_count = (sizeof(vectors) / sizeof(LineVectorData));
                    float normalized_length = 0;
                    
                    //normalizing them according to how importend the vectorized object is.
                    for(LineVectorData vector : vectors) {
                        vector.length =  1 / (vector.distance + 1) * vector.length;
                        normalized_length += vector.length;
                    } 

                    logger->log("test3");

                    normalized_length /= item_count;

                    //scalar products for overall direction
                    float avg_slope;

                    for(LineVectorData vector : vectors) {
                        avg_slope += 1 / (vector.distance + 1) * vector.slope;
                    } 

                    avg_slope /= item_count;

                    // TODO: problem detected if lanes are too long and not linear --> lane dectection object has to be split into smaller pieces in the processing of the BEV

                    float values[] = {normalized_length, avg_slope};

                    float xStart = 200;
                    float yStart = 400;

                    //normalized_length = 100;

                    float xDest = xStart + sin(avg_slope) * normalized_length * 400;
                    float yDest = yStart - cos(avg_slope) * normalized_length * 400;

                    logger->log("x: %f, y: %f, slope: %f, lnengtj: %f", xDest, yDest, avg_slope, normalized_length);

                    cv::line(cv::InputOutputArray(shared_memory->bev_data->img_buffer), cv::Point(xStart, yStart), cv::Point(xDest, yDest), cv::Scalar(180), 3);

                    /*ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Lane_Found);
                    ipc_packet.clear_and_edit().write(values);
                    socket->send_packet(ipc_packet);*/

                    ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Birdseye_Image_Available);
                    socket->send_packet(ipc_packet);
                } break;
            }
        }
    }
}