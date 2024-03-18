#include "../common/ocMember.h"
#include "../common/ocProfiler.h"
#include "../common/ocSdfRenderer.h"
#include "../common/ocWindow.h"

#include <cstring> // strerror()
#include <iostream>
#include <map>
#include <set>
#include <vector>

static const std::map<ocMemberId, oc::Color> colors = {
    { ocMemberId::Lane_Detection    , {1.0f, 0.0f, 0.0f} },
    { ocMemberId::Ai                , {0.0f, 1.0f, 0.0f} },
    { ocMemberId::Sign_Detection    , {0.0f, 0.0f, 1.0f} },
    { ocMemberId::Obstacle_Detection, {0.0f, 1.0f, 1.0f} },
    { ocMemberId::Qr_Detection      , {1.0f, 0.0f, 1.0f} }
};

int main()
{
    ocMember member(ocMemberId::Video_Viewer, "Video Viewer");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();
    ocLogger *logger = member.get_logger();

    ocPacket s(ocMessageId::Subscribe_To_Messages);
    s.clear_and_edit()
        .write(ocMessageId::Camera_Image_Available)
        .write(ocMessageId::Binary_Image_Available)
        .write(ocMessageId::Birdseye_Image_Available)
        .write(ocMessageId::Shapes)
        .write(ocMessageId::Request_Timing_Sites);
    socket->send_packet(s);

    ocPacket recv_packet;
    ocPacket timing_packet;

    std::map<ocMemberId, uint32_t> processes;

    std::map<ocMemberId, std::vector<ocPoint>>     cam_points;
    std::map<ocMemberId, std::vector<ocCircle>>    cam_circles;
    std::map<ocMemberId, std::vector<ocLine>>      cam_lines;
    std::map<ocMemberId, std::vector<ocRectangle>> cam_rects;

    std::map<ocMemberId, std::vector<ocPoint>>     bev_points;
    std::map<ocMemberId, std::vector<ocCircle>>    bev_circles;
    std::map<ocMemberId, std::vector<ocLine>>      bev_lines;
    std::map<ocMemberId, std::vector<ocRectangle>> bev_rects;

    oc::Window cam_window(640, 512, "Camera", true);
    oc::Window bin_window(640, 512, "Binary", true);
    oc::Window bev_window(640, 512, "Birdseye", true);

    float *cam_buffer = nullptr;
    float *bin_buffer = nullptr;
    float *bev_buffer = nullptr;

    uint32_t cam_width = 0, cam_height = 0;
    uint32_t bin_width = 0, bin_height = 0;
    uint32_t bev_width = 0, bev_height = 0;

    bool show_shapes = true;
    bool running = true;
    bool update_cam = false;
    bool update_bin = false;
    bool update_bev = false;
    bool draw_cam = false;
    bool draw_bin = false;
    bool draw_bev = false;

    while (running)
    {
        TIMED_BLOCK("Handle Packets");
        int32_t status = socket->read_packet(recv_packet);
        while (0 < status)
        {
            switch (recv_packet.get_message_id())
            {
            case ocMessageId::Camera_Image_Available:
            {
                update_cam = true;
                draw_cam = true;
            } break;
            case ocMessageId::Binary_Image_Available:
            {
                update_bin = true;
                draw_bin = true;
            } break;
            case ocMessageId::Birdseye_Image_Available:
            {
                update_bev = true;
                draw_bev = true;
            } break;
            case ocMessageId::Shapes:
            {
                ocMemberId sender = recv_packet.get_sender();
                auto reader = recv_packet.read_from_start();
                uint32_t frame_num = reader.read<uint32_t>();
                ocImageType image_type = reader.read<ocImageType>();

                std::vector<ocPoint>     *points_vec;
                std::vector<ocCircle>    *circles_vec;
                std::vector<ocLine>      *lines_vec;
                std::vector<ocRectangle> *rects_vec;

                if (ocImageType::Cam == image_type)
                {
                    points_vec  = &cam_points[sender];
                    circles_vec = &cam_circles[sender];
                    lines_vec   = &cam_lines[sender];
                    rects_vec   = &cam_rects[sender];
                    draw_cam = true;
                }
                else if (ocImageType::Bev == image_type)
                {
                    points_vec  = &bev_points[sender];
                    circles_vec = &bev_circles[sender];
                    lines_vec   = &bev_lines[sender];
                    rects_vec   = &bev_rects[sender];
                    draw_bev = true;
                }
                else
                {
                    logger->error("Unhandled image type: %s (%i)", to_string(image_type), image_type);
                    continue;
                }

                processes[sender] = frame_num;
                points_vec->clear();
                circles_vec->clear();
                lines_vec->clear();
                rects_vec->clear();

                while (reader.can_read<ocShapeType>())
                {
                    ocShapeType type = reader.read<ocShapeType>();
                    switch (type)
                    {
                        case ocShapeType::Point:
                        {
                            ocPoint point = reader.read<ocPoint>();
                            points_vec->push_back(point);
                        } break;
                        case ocShapeType::Circle:
                        {
                            ocCircle circle = reader.read<ocCircle>();
                            circles_vec->push_back(circle);
                        } break;
                        case ocShapeType::Line:
                        {
                            ocLine line = reader.read<ocLine>();
                            lines_vec->push_back(line);
                        } break;
                        case ocShapeType::Rectangle:
                        {
                            ocRectangle rectangle = reader.read<ocRectangle>();
                            rects_vec->push_back(rectangle);
                        } break;
                        default:
                        {
                            logger->warn("Unhandled shape type: %s (%i)", to_string(type), (int) type);
                        } break;
                    }
                }
            } break;
            case ocMessageId::Request_Timing_Sites:
            {
                timing_packet.set_sender(ocMemberId::Video_Viewer);
                timing_packet.set_message_id(ocMessageId::Timing_Sites);
                if (write_timing_sites_to_buffer(timing_packet.get_payload()))
                {
                    socket->send_packet(timing_packet);
                }
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

        NEXT_TIMED_BLOCK("Update Images");
        if (update_cam)
        {
            TIMED_BLOCK("Update Camera Image");
            update_cam = false;
            const ocCamData *cam_data = &shared_memory->cam_data[shared_memory->last_written_cam_data_index];
            cam_width  = cam_data->width;
            cam_height = cam_data->height;
            cam_buffer = (float *)realloc((void *)cam_buffer, cam_width * cam_height * bytes_per_pixel(ocPixelFormat::Rgb_F32));
            convert_to_rgb_f32(cam_data->pixel_format, cam_data->img_buffer, cam_width, cam_height, cam_buffer, cam_width, cam_height);
        }
        if (draw_cam)
        {
            TIMED_BLOCK("Draw Camera Image");
            draw_cam = false;
            cam_window.set_size((int)cam_width, (int)cam_height);
            for (uint32_t y = 0; y < cam_height; ++y)
            for (uint32_t x = 0; x < cam_width;  ++x)
            {
                cam_window.draw_pixel((int)x, (int)y, {cam_buffer[(y * cam_width + x) * 3 + 0], cam_buffer[(y * cam_width + x) * 3 + 1], cam_buffer[(y * cam_width + x) * 3 + 2]});
            }
            if (show_shapes)
            {
                NEXT_TIMED_BLOCK("Draw Camera Image Shapes");
                for (auto &it : processes)
                {
                    auto process_id = it.first;
                    auto color = colors.at(process_id);
                    for (auto &point  : cam_points[process_id])  oc::render(cam_window, oc::circle(point.x, point.y, 2.0f), color);
                    for (auto &circle : cam_circles[process_id]) oc::render(cam_window, oc::outline(oc::circle(circle.x, circle.y, circle.radius), 1.0f), color);
                    for (auto &line   : cam_lines[process_id])   oc::render(cam_window, oc::line(line.x1, line.y1, line.x2, line.y2, 1.0f), color);
                    for (auto &rect   : cam_rects[process_id])   oc::render(cam_window, oc::outline(oc::box(rect.left, rect.top, rect.right, rect.bottom), 1.0f), color);
                }
            }
            cam_window.commit();
        }
        if (update_bin)
        {
            update_bin = false;
            TIMED_BLOCK("Update Binary Image");
            const ocBinData *bin_data = &shared_memory->bin_data[shared_memory->last_written_bin_data_index];
            bin_width = bin_data->width;
            bin_height = bin_data->height;
            bin_buffer = (float *)realloc((void *)bin_buffer, bin_width * bin_height * bytes_per_pixel(ocPixelFormat::Rgb_F32));
            convert_to_rgb_f32(ocPixelFormat::Gray_U8, bin_data->img_buffer, bin_width, bin_height, bin_buffer, bin_width, bin_height);
        }
        if (draw_bin)
        {
            NEXT_TIMED_BLOCK("Draw Binary Image");
            draw_bin = false;
            bin_window.set_size((int)bin_width, (int)bin_height);
            for (uint32_t y = 0; y < bin_height; ++y)
            for (uint32_t x = 0; x < bin_width; ++x)
            {
                bin_window.draw_pixel((int)x, (int)y, {bin_buffer[(y * bin_width + x) * 3 + 0], bin_buffer[(y * bin_width + x) * 3 + 1], bin_buffer[(y * bin_width + x) * 3 + 2]});
            }
            bin_window.commit();
        }
        if (update_bev)
        {
            TIMED_BLOCK("Update BEV Image");
            update_bev = false;
            const ocBevData *bev_data = &shared_memory->bev_data[shared_memory->last_written_bev_data_index];
            bev_width  = (uint32_t)(bev_data->max_map_x - bev_data->min_map_x);
            bev_height = (uint32_t)(bev_data->max_map_y - bev_data->min_map_y);
            bev_buffer = (float *)realloc((void *)bev_buffer, bev_width * bev_height * bytes_per_pixel(ocPixelFormat::Rgb_F32));
            convert_to_rgb_f32(ocPixelFormat::Gray_U8, bev_data->img_buffer, bev_width, bev_height, bev_buffer, bev_width, bev_height);
        }
        if (draw_bev)
        {
            TIMED_BLOCK("Draw BEV Image");
            draw_bev = false;
            bev_window.set_size((int)bev_width, (int)bev_height);
            for (uint32_t y = 0; y < bev_height; ++y)
            for (uint32_t x = 0; x < bev_width; ++x)
            {
                bev_window.draw_pixel((int)x, (int)y, {bev_buffer[(y * bev_width + x) * 3 + 0], bev_buffer[(y * bev_width + x) * 3 + 1], bev_buffer[(y * bev_width + x) * 3 + 2]});
            }
            if (show_shapes)
            {
                NEXT_TIMED_BLOCK("Draw BEV Image Shapes");
                for (auto &it : processes)
                {
                    auto process_id = it.first;
                    auto color = colors.at(process_id);
                    for (auto &point  : bev_points[process_id])  oc::render(bev_window, oc::circle(point.x, point.y, 2.0f), color);
                    for (auto &circle : bev_circles[process_id]) oc::render(bev_window, oc::outline(oc::circle(circle.x, circle.y, circle.radius), 1.0f), color);
                    for (auto &line   : bev_lines[process_id])   oc::render(bev_window, oc::line(line.x1, line.y1, line.x2, line.y2, 1.0f), color);
                    for (auto &rect   : bev_rects[process_id])   oc::render(bev_window, oc::outline(oc::box(rect.left, rect.top, rect.right, rect.bottom), 1.0f), color);
                }
            }

            bev_window.commit();
        }

        if (40 < timing_event_count())
        {
            TIMED_BLOCK("Send timing data");
            timing_packet.set_sender(ocMemberId::Video_Viewer);
            timing_packet.set_message_id(ocMessageId::Timing_Events);
            if (write_timing_events_to_buffer(timing_packet.get_payload()))
            {
                socket->send_packet(timing_packet);
            }
        }
        NEXT_TIMED_BLOCK("Handle Events");
        while (true)
        {
            auto event = cam_window.next_event();
            if (oc::EventType::Draw == event.type) event = bin_window.next_event();
            if (oc::EventType::Draw == event.type) event = bev_window.next_event();
            if (oc::EventType::Draw == event.type) break;

            if (oc::EventType::Close == event.type) return 0;
            if (oc::EventType::Key == event.type && event.key.down)
            {
                if (oc::KeyCode::Key_S == event.key.code) show_shapes = !show_shapes;
            }
        }

    } // End while

    return 0;
}
