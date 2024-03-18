#include "../common/ocAlarm.h"
#include "../common/ocHistoryBuffer.h"
#include "../common/ocMember.h"
#include "../common/ocPollEngine.h"
#include "../common/ocTime.h"

#include <cstring> // strerror()

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

int main()
{
    ocMember member(ocMemberId::Tachometer, "Tachometer");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocLogger *logger = member.get_logger();

    ocPacket s(ocMessageId::Subscribe_To_Messages);
    s.clear_and_edit()
        .write(ocMessageId::Ipc_Stats)
        .write(ocMessageId::Start_Driving_Task)
        .write(ocMessageId::Received_Odo_Steps)
        .write(ocMessageId::Received_Current_Speed);
    socket->send_packet(s);

    ocPacket recv_packet;

    ocHistoryBuffer<ocTime, uint32_t> sent_packets_history(12);
    ocHistoryBuffer<ocTime, uint32_t> read_packets_history(12);
    ocHistoryBuffer<ocTime, uint32_t> sent_bytes_history(12);
    ocHistoryBuffer<ocTime, uint32_t> read_bytes_history(12);
    ocHistoryBuffer<ocTime, int16_t> speed_history(1000);
    ocHistoryBuffer<ocTime, uint32_t> steps_history(1000);
    ocHistoryBuffer<ocTime, int16_t> target_speed_history(1000);
    ocHistoryBuffer<ocTime, int8_t> steering_front_history(1000);
    ocHistoryBuffer<ocTime, int8_t> steering_rear_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_front_left_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_front_center_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_front_right_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_side_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_rear_left_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_rear_right_history(1000);
    ocHistoryBuffer<ocTime, uint8_t> ir_rear_center_history(1000);

    float sent_packets_scale    = 0.5f;
    float read_packets_scale    = 0.5f;
    float sent_bytes_scale      = 0.002f;
    float read_bytes_scale      = 0.002f;
    float speed_scale           = 1.0f;
    float steps_scale           = 1.0f;
    float target_speed_scale    = 1.0f;
    float steering_front_scale  = 1.0f;
    float steering_rear_scale   = 1.0f;
    float ir_front_left_scale   = 1.0f;
    float ir_front_center_scale = 1.0f;
    float ir_front_right_scale  = 1.0f;
    float ir_side_scale         = 1.0f;
    float ir_rear_left_scale    = 1.0f;
    float ir_rear_right_scale   = 1.0f;
    float ir_rear_center_scale  = 1.0f;

    float sent_packets_offset    = 0.0f;
    float read_packets_offset    = 0.0f;
    float sent_bytes_offset      = 0.0f;
    float read_bytes_offset      = 0.0f;
    float speed_offset           = 200.0f;
    float steps_offset           = 1.0f;
    float target_speed_offset    = 200.0f;
    float steering_front_offset  = 200.0f;
    float steering_rear_offset   = 200.0f;
    float ir_front_left_offset   = 1.0f;
    float ir_front_center_offset = 1.0f;
    float ir_front_right_offset  = 1.0f;
    float ir_side_offset         = 1.0f;
    float ir_rear_left_offset    = 1.0f;
    float ir_rear_right_offset   = 1.0f;
    float ir_rear_center_offset  = 1.0f;

    bool running = true;
    ocTime window_length  = ocTime::seconds(10);
    ocTime time_per_pixel = ocTime::milliseconds(10);
    ocTime latest = ocTime::now();

    ocAlarm draw_timer(ocTime::hertz(10));
    draw_timer.start(ocAlarmType::Periodic);

    int display_height = 400;
    int display_width = (int)(window_length / time_per_pixel);
    cv::Mat display(display_height, display_width, CV_8UC3);
    cv::namedWindow("Graphs", cv::WINDOW_AUTOSIZE);

    ocPollEngine pe(2);
    pe.add_fd(socket->get_fd());
    pe.add_fd(draw_timer.get_fd());

    while (running)
    {
        pe.await();

        if (pe.was_triggered(socket->get_fd()))
        {
            int32_t status = socket->read_packet(recv_packet);
            ocTime now = ocTime::now();
            latest = now;
            while (0 < status)
            {
                auto reader = recv_packet.read_from_start();
                switch (recv_packet.get_message_id())
                {
                case ocMessageId::Ipc_Stats:
                {
                    sent_packets_history.push(now, reader.read<uint32_t>());
                    read_packets_history.push(now, reader.read<uint32_t>());
                    sent_bytes_history.push(now, reader.read<uint32_t>());
                    read_bytes_history.push(now, reader.read<uint32_t>());
                } break;
                case ocMessageId::Start_Driving_Task:
                {
                    target_speed_history.push(now, reader.read<int16_t>());
                    steering_front_history.push(now, reader.read<int8_t>());
                    steering_rear_history.push(now, reader.read<int8_t>());
                } break;
                case ocMessageId::Received_Odo_Steps:
                {
                    steps_history.push(now, reader.read<uint32_t>());
                } break;
                case ocMessageId::Received_Current_Speed:
                {
                    speed_history.push(now, reader.read<int16_t>());
                } break;
                default:
                {
                    ocMessageId msg_id = recv_packet.get_message_id();
                    ocMemberId  mbr_id = recv_packet.get_sender();
                    logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                } break;
                } // End switch

                // read next packet (if there is one)
                status = socket->read_packet(recv_packet, false);
            }

            if (status < 0)
            {
                logger->error("Error while reading IPC socket: (%i) %s", errno, strerror(errno));
                running = false;
            }
        }

        if (draw_timer.is_expired())
        {
            display = cv::Scalar(0.0, 0.0, 0.0);

            ocTime oldest = latest - window_length;
            for (int x0 = 0, y0 = 0; auto &[time, value] : sent_packets_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * sent_packets_scale + sent_packets_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(127.0, 16.0, 16.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : read_packets_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * read_packets_scale + read_packets_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(16.0, 16.0, 127.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : sent_bytes_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * sent_bytes_scale + sent_bytes_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 16.0, 16.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : read_bytes_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * read_bytes_scale + read_bytes_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(16.0, 16.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : speed_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * speed_scale + speed_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 64.0, 64.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : steps_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * steps_scale + steps_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(64.0, 255.0, 64.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : target_speed_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * target_speed_scale + target_speed_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x1, y0), cv::Point(x0, y0), cv::Scalar(64.0, 64.0, 255.0), 2);
                    cv::line(display, cv::Point(x1, y1), cv::Point(x1, y0), cv::Scalar(64.0, 64.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : steering_front_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * steering_front_scale + steering_front_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x1, y0), cv::Point(x0, y0), cv::Scalar(255.0, 64.0, 255.0), 2);
                    cv::line(display, cv::Point(x1, y1), cv::Point(x1, y0), cv::Scalar(255.0, 64.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : steering_rear_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * steering_rear_scale + steering_rear_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x1, y0), cv::Point(x0, y0), cv::Scalar(255.0, 255.0, 64.0), 2);
                    cv::line(display, cv::Point(x1, y1), cv::Point(x1, y0), cv::Scalar(255.0, 255.0, 64.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_front_left_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_front_left_scale + ir_front_left_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_front_center_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_front_center_scale + ir_front_center_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_front_right_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_front_right_scale + ir_front_right_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_side_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_side_scale + ir_side_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_rear_left_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_rear_left_scale + ir_rear_left_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_rear_center_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_rear_center_scale + ir_rear_center_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            for (int x0 = 0, y0 = 0; auto &[time, value] : ir_rear_right_history)
            {
                int x1 = (int)((float)display_width * ((time - oldest) / window_length));
                int y1 = display_height - (int)((float)value * ir_rear_right_scale + ir_rear_right_offset);
                if (0 != x0)
                {
                    cv::line(display, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(255.0, 255.0, 255.0), 2);
                }
                if (x1 < 0) break;
                x0 = x1;
                y0 = y1;
            }
            ocTime mid_time = latest - window_length * 0.1f;
            int row = 0;
            int left = display_width - 120;
            if (sent_packets_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "sent_packets", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(127.0, 16.0, 16.0), 1);
            }
            if (read_packets_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "read_packets", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(16.0, 16.0, 127.0), 1);
            }
            if (sent_bytes_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "sent_bytes", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 16.0, 16.0), 1);
            }
            if (read_bytes_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "read_bytes", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(16.0, 16.0, 255.0), 1);
            }
            if (speed_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "speed", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 64.0, 64.0), 1);
            }
            if (steps_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "steps", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(64.0, 255.0, 64.0), 1);
            }
            if (target_speed_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "target_speed", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(64.0, 64.0, 255.0), 1);
            }
            if (steering_front_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "steering_front", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 64.0, 255.0), 1);
            }
            if (steering_rear_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "steering_rear", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 64.0), 1);
            }
            if (ir_front_left_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_front_left", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }
            if (ir_front_center_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_front_center", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }
            if (ir_front_right_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_front_right", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }
            if (ir_side_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_side", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }
            if (ir_rear_left_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_rear_left", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }
            if (ir_rear_right_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_rear_right", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }
            if (ir_rear_center_history.contains(mid_time))
            {
                cv::rectangle(display, cv::Rect(left, row * 20 + 20, 150, 20), cv::Scalar(0.0, 0.0, 0.0), cv::FILLED);
                cv::putText(display, "ir_rear_center", cv::Point(left, row++ * 20 + 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255.0, 255.0, 255.0), 1);
            }

            cv::imshow("Graphs", display);
            cv::waitKey(1);
        }
    } // End while

    cv::destroyWindow("Graphs");

    return 0;
}
