#include "../common/ocPacket.h"
#include "../common/ocMember.h"
#include <signal.h>
#include <vector>
#include <opencv2/opencv.hpp>

struct LineVectorData {
    double slope;
    double length;
    std::pair<double, double> closest_point;
    double distance;
};

ocLogger *logger;

static bool running = true;

static void signal_handler(int)
{
    running = false;
}

std::pair<double, double> linearRegression(int (data)[NUMBER_OF_POLYGONS_PER_CONTOUR][2], int count) {
    double x_mean = 0;
    double y_mean = 0;

    for(int j = 0; j < count; j++) {
        x_mean += data[j][0];
        y_mean += data[j][1];
    } 

    x_mean = x_mean / count;
    y_mean = y_mean / count;

    double numerator = 0;
    double denominator = 1;

    for(int j = 0; j < count; j++) {
        numerator += (data[j][0] - x_mean) * (data[j][1] - y_mean);
        denominator += (data[j][1] - y_mean) * (data[j][1] - y_mean);
    } 

    double slope = numerator/denominator;

    double y_intercept = x_mean - slope * y_mean;  

    return {slope, y_intercept};
};

double calcDist(std::pair<double, double> p1, std::pair<double, double> p2) {
    return std::sqrt(std::pow(std::get<0>(p1) - std::get<0>(p2), 2) + std::pow(std::get<1>(p1) - std::get<1>(p2), 2));
}

int image_i = 0;

int main()
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
                    //Find Lane
                    static struct ocBevLines lines;
                    ipc_packet.read_from_start().read(&lines);

                    //TODO summarize small dots to a bigger line for better vectorization

                    

                    LineVectorData vectors[lines.contour_num];
                    int vector_counter = 0;

                    for(int i = 0; i < lines.contour_num; i++) {
                        if(lines.poly_num[i] < 2) continue;
                        //do for every shape a linear regression to prepare a vector 

                        std::vector<double> vector;

                        std::pair<double, double> linReg = linearRegression((lines.lines[i]), (lines.poly_num[i]));

                        double slope = (atan(std::get<0>(linReg))*-180/M_PI);

                        if(slope != slope || abs(slope) > 80) continue;

                        std::pair<double, double> closest_point = {0, 0}; 
                        std::pair<double, double> furthest_point = {199, 399};

                        double dist = 0;
                        double longest_dist = 0;
                        double shortest_dist = 0;

                        for(int j = 0; j < lines.poly_num[i]; j++) {
                            dist = calcDist(((std::pair<double, double>){lines.lines[i][j][0], lines.lines[i][j][1]}), 
                                ((std::pair<double,double>){199, 399}));

                            if(dist < shortest_dist || j == 0) {
                                shortest_dist = dist;
                                closest_point = {lines.lines[i][j][0], lines.lines[i][j][1]};
                            }

                            if(dist > longest_dist || j == 0) {
                                longest_dist = dist;
                                furthest_point = {lines.lines[i][j][0], lines.lines[i][j][1]};
                            }
                        } 
                        
                        vectors[vector_counter++] = {
                            .slope = slope,
                            .length = calcDist(closest_point, furthest_point),
                            .closest_point = closest_point,
                            .distance = shortest_dist,
                        };
                    }

                    double normalized_length = 0;
                    
                    //normalizing them according to how importend the vectorized object is.
                    for(LineVectorData vector : vectors) {
                        vector.length =  1 / (vector.distance + 1) * vector.length;
                        logger->log("%f", vector.length);
                        normalized_length += vector.length;
                    } 

                    normalized_length /= vector_counter;

                    //scalar products for overall direction
                    double avg_slope = 0;
                    double total_distance = 0;

                    for(LineVectorData vector : vectors) {
                        avg_slope += (vector.distance + 1) * vector.slope;
                        total_distance += vector.distance+1;
                    } 

                    avg_slope /= total_distance;
                    // TODO: problem detected if lanes are too long and not linear --> lane dectection object has to be split into smaller pieces in the processing of the BEV

                    double xStart = 200;
                    double yStart = 400;

                    double factor = avg_slope / abs(avg_slope);
                    double xDest = xStart + cos(factor * 90 + avg_slope) * normalized_length * 100;
                    double yDest = yStart - abs(sin(factor * 90 + avg_slope)) * normalized_length * 200;

                    logger->log("x: %f, y: %f, slope: %f, length: %f", xDest, yDest, avg_slope, normalized_length);

                    cv::line(cv::Mat(400,400,CV_8UC1, shared_memory->bev_data->img_buffer), cv::Point(xStart, yStart), cv::Point(xDest, yDest), cv::Scalar(255,255,255,1), 10);

                    /*ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Lane_Found);
                    ipc_packet.clear_and_edit().write(values);
                    socket->send_packet(ipc_packet);*/

                    if(image_i == 100) {
                        logger->log("image saved");
                        cv::imwrite("test.jpg", *shared_memory->bev_data->img_buffer);

                        return 0;
                    }

                    image_i++;

                    

                    ipc_packet.set_sender(ocMemberId::Lane_Detection);
                    ipc_packet.set_message_id(ocMessageId::Birdseye_Image_Available);
                    socket->send_packet(ipc_packet);
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