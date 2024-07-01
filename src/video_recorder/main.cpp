#include "../common/ocArgumentParser.h"
#include "../common/ocMember.h"
#include "../common/ocCommon.h"

#include <iostream>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

#include <cstring> // strerror()
#include <csignal>

static bool running = true;

static void signal_handler(int)
{
    running = false;
}

cv::Mat static convert(cv::Mat cam_image, ocPixelFormat pixel_format, bool gray, ocLogger *logger)
{
    if (gray)
    {
        cv::Mat converted(cam_image.rows, cam_image.cols, CV_8UC1);
        if (!convert_to_gray_u8(
                pixel_format,
                cam_image.data,
                (uint32_t)cam_image.cols,
                (uint32_t)cam_image.rows,
                converted.data,
                (uint32_t)converted.cols,
                (uint32_t)converted.rows))
        {
            logger->warn("Unsupported pixel format: %s (%i)", to_string(pixel_format), (int)pixel_format);
        }
        return converted;
    }
    else
    {
        cv::Mat converted(cam_image.rows, cam_image.cols, CV_8UC3);
        if (!convert_to_bgr_u8(
                pixel_format,
                cam_image.data,
                (uint32_t)cam_image.cols,
                (uint32_t)cam_image.rows,
                converted.data,
                (uint32_t)converted.cols,
                (uint32_t)converted.rows))
        {
            logger->warn("Unsupported pixel format: %s (%i)", to_string(pixel_format), (int)pixel_format);
        }
        return converted;
    }
}

int main(int argc, const char **argv)
{
    ocMember member(ocMemberId::Video_Recorder, "Video Recorder");
    ocLogger *logger = member.get_logger();
    ocArgumentParser arg_parser(argc, argv);

    if (arg_parser.has_key("-?") ||
        arg_parser.has_key("-h") ||
        arg_parser.has_key("-help"))
    {
        logger->log("");
        logger->log("Usage: ./video_recorder [args] -o <filename>");
        logger->log("Additional arguments:");
        logger->log("  -?, -h, -help: show this help");
        logger->log("  -cx <x> -cw <width> : crop the video horizontally");
        logger->log("  -cy <y> -ch <height>: crop the video vertically");
        logger->log("  -ow <width> : scale the video to the given width");
        logger->log("  -oh <height>: scale the video to the given height");
        logger->log("-gray: force the video to be gray-scale");
        logger->log("");
    }

    auto filename = arg_parser.get_value("-o");
    if (filename.empty())
    {
        logger->error("Missing output file argument (-o path/to/file.avi)");
        return -1;
    }

    int crop_x = -1;
    int crop_y = -1;
    int crop_w = -1;
    int crop_h = -1;

    bool crop_hor = false;
    bool crop_ver = false;

    crop_hor |= arg_parser.get_int32("-cx", &crop_x);
    crop_ver |= arg_parser.get_int32("-cy", &crop_y);
    crop_hor |= arg_parser.get_int32("-cw", &crop_w);
    crop_ver |= arg_parser.get_int32("-ch", &crop_h);

    if (-1 == crop_w && -1 != crop_x)
    {
        logger->error("Crop x (-cx) is set, but not the width (-cw).");
        return -1;
    }

    if (-1 == crop_x && -1 != crop_w)
    {
        logger->error("Crop width  (-cw) is set, but not the x (-cx).");
        return -1;
    }

    if (-1 == crop_h && -1 != crop_y)
    {
        logger->error("Crop y (-cy) is set, but not the height (-ch).");
        return -1;
    }

    if (-1 == crop_y && -1 != crop_h)
    {
        logger->error("Crop height (-ch) is set, but not the y (-cy).");
        return -1;
    }

    cv::Rect crop(crop_x, crop_y, crop_w, crop_h);
    if (!crop_hor) crop.x = 0;
    if (!crop_ver) crop.y = 0;

    bool gray  = arg_parser.has_key("-gray");
    bool bevRecording = arg_parser.has_key("-bev");

    int new_w = -1;
    int new_h = -1;

    arg_parser.get_int32("-ow", &new_w);
    arg_parser.get_int32("-oh", &new_h);

    cv::Size new_size(new_w, new_h);

    member.attach();
    ocIpcSocket *socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();

    if (filename.ends_with(".png") ||
        filename.ends_with(".bmp") ||
        filename.ends_with(".jpg") ||
        filename.ends_with(".jpeg"))
    {
        ocCamData *cam_data = &shared_memory->cam_data[shared_memory->last_written_cam_data_index];

        int type = CV_8UC1;
        if (3 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC3;
        if (4 == bytes_per_pixel(cam_data->pixel_format)) type = CV_8UC4;
        if (12 == bytes_per_pixel(cam_data->pixel_format)) type = CV_32FC3;

        cv::Mat cam_image((int)cam_data->height, (int)cam_data->width, type);
        cam_image.data = cam_data->img_buffer;

        if (crop_hor || crop_ver)
        {
            if (!crop_hor) crop.width  = (int)cam_data->width;
            if (!crop_ver) crop.height = (int)cam_data->height;
            if ((int)cam_data->width < crop.x + crop.width || (int)cam_data->height < crop.y + crop.height)
            {
                logger->error("Crop size (x: %i, y: %i, w: %i, h: %i) does not fit source size (w: %i, h: %i).", crop.x, crop.y, crop.width, crop.height, cam_data->width, cam_data->height);
                return -1;
            }
            cam_image = cam_image(crop);
        }

        if (-1 != new_w || -1 != new_h)
        {
            if (-1 == new_w) new_size.width  = crop.width;
            if (-1 == new_h) new_size.height = crop.height;
            cv::resize(cam_image, cam_image, new_size);
        }

        cam_image = convert(cam_image, cam_data->pixel_format, gray, logger);

        cv::imwrite(filename.data(), cam_image);
        return 0;
    }

    ocPacket s(ocMessageId::Subscribe_To_Messages);
    s.clear_and_edit()
        .write(bevRecording ? ocMessageId::Birdseye_Image_Available : ocMessageId::Camera_Image_Available);
    socket->send_packet(s);

    ocPacket recv_packet;

    // catch some signals to make sure the video file is properly closed
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    cv::VideoWriter video_writer;

    while (running)
    {
        int32_t status = socket->read_packet(recv_packet);
        if (0 < status)
        {
            switch (recv_packet.get_message_id())
            {
            case ocMessageId::Camera_Image_Available:
            case ocMessageId::Birdseye_Image_Available:
            {
                uint32_t newest_frame_index = shared_memory->last_written_cam_data_index;
                ocCamData *cam_data = &shared_memory->cam_data[newest_frame_index];
                uint32_t newest_frame_number = cam_data->frame_number;

                uint32_t frame_number;
                recv_packet.read_from_start()
                    .skip<ocTime>()
                    .read(&frame_number);
                if (frame_number != newest_frame_number)
                {
                    logger->warn("Not keeping up with video stream. Processing frame %i while newest is %i", frame_number, newest_frame_number);
                }
                else
                {
                    int32_t width    = (int32_t)cam_data->width;
                    int32_t height   = (int32_t)cam_data->height;
                    ocPixelFormat pf = cam_data->pixel_format;
                    bool is_color = !gray && (pf != ocPixelFormat::Gray_U8);

                    if (!video_writer.isOpened())
                    {
                        if (!crop_hor) crop.width  = width;
                        if (!crop_ver) crop.height = height;

                        if (width < crop.x + crop.width || height < crop.y + crop.height)
                        {
                            logger->error("Crop size (x: %i, y: %i, w: %i, h: %i) does not fit source size (w: %i, h: %i).", crop.x, crop.y, crop.width, crop.height, width, height);
                            return -1;
                        }

                        if (-1 == new_w) new_size.width  = crop.width;
                        if (-1 == new_h) new_size.height = crop.height;
                        video_writer.open(filename.data(), cv::CAP_FFMPEG, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 30, new_size, is_color);
                        if (!video_writer.isOpened())
                        {
                            logger->error("Opening the video writer didn't work. size: (w: %i h: %i) color: %i", new_size.width, new_size.height, is_color);
                            running = false;
                        }
                    }

                    int type = CV_8UC1;
                    if (3 == bytes_per_pixel(pf)) type = CV_8UC3;
                    if (4 == bytes_per_pixel(pf)) type = CV_8UC4;
                    if (12 == bytes_per_pixel(pf)) type = CV_32FC3;

                    cv::Mat cam_image(height, width, type);
                    cam_image.data = cam_data->img_buffer;

                    cam_image = cam_image(crop);

                    if (-1 != new_w || -1 != new_h)
                    {
                        if (-1 == new_w) new_size.width  = crop.width;
                        if (-1 == new_h) new_size.height = crop.height;
                        cv::resize(cam_image, cam_image, new_size);
                    }

                    cam_image = convert(cam_image, cam_data->pixel_format, gray, logger);

                    video_writer << cam_image;
                }
            } break;
            default:
            {
                ocMessageId msg_id = recv_packet.get_message_id();
                ocMemberId  mbr_id = recv_packet.get_sender();
                logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
            } break;
            } // End switch
        }

        if (status < 0)
        {
            logger->error("Error while reading IPC socket: (%i) %s", errno, strerror(errno));
            running = false;
        }
    } // End while

    socket->send(ocMessageId::Disconnect_Me);
    return 0;
}
