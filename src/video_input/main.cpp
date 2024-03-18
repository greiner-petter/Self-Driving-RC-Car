#include "../common/ocAlarm.h"
#include "../common/ocConst.h"
#include "../common/ocMember.h"
#include "../common/ocPacket.h"
#include "../common/ocSdfRenderer.h"
#include "../common/ocWindow.h"

#include <string>
#include <iostream>

#include <cstdint>
#include <unistd.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

static cv::VideoCapture video_input;
static cv::Mat image_input;

int main(int argc, char** argv)
{
    ocMember member(ocMemberId::Video_Input, "Video Input");
    ocLogger *logger = member.get_logger();

    if (argc != 2) {
        logger->error("Expected exactly 1 argument, but got %i", argc - 1);
        return -1;
    }
    std::string filename = argv[1];

    member.attach();
    ocSharedMemory *shared_memory = member.get_shared_memory();
    ocIpcSocket *socket = member.get_socket();

    ocPacket ipc_packet;

    oc::Window *window = nullptr;

    uint32_t width, height, channels, frame_count;
    float frame_time = 1.0f / 30.0f;

    image_input = cv::imread(filename, cv::IMREAD_GRAYSCALE);
    if (image_input.data != nullptr)
    {
        logger->log("loading a single picture");
        width  = (uint32_t) image_input.cols;
        height = (uint32_t) image_input.rows;
        channels = (uint32_t) image_input.channels();
        frame_count = 1;
    }
    else
    {
        // if file seems to be a video
        video_input = cv::VideoCapture(filename);
        if (video_input.isOpened())
        {
            width  = (uint32_t) video_input.get(cv::CAP_PROP_FRAME_WIDTH);
            height = (uint32_t) video_input.get(cv::CAP_PROP_FRAME_HEIGHT);
            channels = 3;
            if (0.0 != video_input.get(cv::CAP_PROP_MONOCHROME)) channels = 1;
            frame_count = (uint32_t) video_input.get(cv::CAP_PROP_FRAME_COUNT);
            frame_time = 1.0f / (float)video_input.get(cv::CAP_PROP_FPS);
        }
        else
        {
            logger->error("File is neither image nor video or could not be opened");
            return -1;
        }

        window = new oc::Window(200, 60, "Video Input", false);
    }

    if (OC_CAM_BUFFER_SIZE < width * height * channels)
    {
        logger->error("Error: Video image is larger than the space in the shared memory!");
        return -1;
    }

    logger->log("size: { w: %i, h: %i, d: %i }", width, height, channels);

    uint32_t frame_number = 0; // for the ipc, should never decrease. 
    uint32_t index = 0;
    uint32_t video_frame_number = 0; // for displaying the progress bar, can go forward, backward, whatever 

    ocAlarm timer(ocTime::seconds_float(frame_time), ocAlarmType::Periodic);

    bool paused = false;
    bool timeline_drag = false;
    bool step = false;
    bool resend = false;
    bool redraw = false;

    while (true)
    {
        while (true)
        {
            if (nullptr == window) break; // in case we have no ui.
            auto event = window->next_event();

            if (oc::EventType::Draw == event.type) break;
            if (oc::EventType::Close == event.type) return 0;
            if (oc::EventType::Key == event.type && event.key.down)
            {
                if (oc::KeyCode::Mouse_1 == event.key.code)
                {
                    redraw = true;
                    uint32_t mx = (uint32_t)window->get_mouse_x();
                    uint32_t my = (uint32_t)window->get_mouse_y();
                    if (50 < my)
                    { // progress bar
                        timeline_drag = true;
                        video_frame_number = mx * frame_count / (uint32_t)window->get_width();
                        video_input.set(cv::CAP_PROP_POS_FRAMES, video_frame_number);
                    }
                    else if (mx < 50)
                    { // play/pause
                        paused = !paused;
                    }
                    else if (mx < 100)
                    { // reset
                        video_frame_number = 0;
                        video_input.set(cv::CAP_PROP_POS_FRAMES, 0);
                    }
                    else if (mx < 150)
                    { // step frame
                        step = true;
                    }
                    else
                    { // resend frame
                        resend = true;
                    }
                }
            }
            if (oc::EventType::Key == event.type && !event.key.down)
            {
                if (oc::KeyCode::Mouse_1 == event.key.code)
                {
                    redraw = true;
                    timeline_drag = false;
                }
            }
            if (oc::EventType::Pointer == event.type)
            {
                redraw = true;
                if (timeline_drag)
                {
                    uint32_t mx = (uint32_t)event.pointer.new_x;
                    video_frame_number = mx * frame_count / (uint32_t)window->get_width();
                    video_input.set(cv::CAP_PROP_POS_FRAMES, video_frame_number);
                }
            }
        }
        timer.await();

        if (redraw)
        {
            int mx = (int)window->get_mouse_x();
            int my = (int)window->get_mouse_y();
            {// Play/Pause
                oc::Color bg {1.0f, 1.0f, 1.0f};
                if (my < 50 && mx < 50)
                {
                    bg = {0.9f, 0.9f, 0.9f};
                    if (window->is_key_down(oc::KeyCode::Mouse_1)) bg = {0.8f, 0.8f, 0.8f};
                }
                window->fill_rect(0, 0, 50, 50, bg);
                if (paused)
                {
                    oc::render(*window, oc::polygon(10.0f, 10.0f, 40.0f, 25.0f, 10.0f, 40.0f), {/*black*/});
                }
                else
                {
                    window->fill_rect(10, 10, 20, 40, {/*black*/});
                    window->fill_rect(30, 10, 40, 40, {/*black*/});
                }
            }
            {// reset
                oc::Color bg {1.0f, 1.0f, 1.0f};
                if (my < 50 && 50 <= mx && mx < 100)
                {
                    bg = {0.9f, 0.9f, 0.9f};
                    if (window->is_key_down(oc::KeyCode::Mouse_1)) bg = {0.8f, 0.8f, 0.8f};
                }
                window->fill_rect(50, 0, 100, 50, bg);
                window->fill_rect(60, 10, 64, 40, {/*black*/});
                oc::render(*window, oc::polygon(64.0f, 25.0f, 77.0f, 10.0f, 77.0f, 40.0f), {/*black*/});
                oc::render(*window, oc::polygon(77.0f, 25.0f, 90.0f, 10.0f, 90.0f, 40.0f), {/*black*/});
            }
            {// step frame
                oc::Color bg {1.0f, 1.0f, 1.0f};
                if (my < 50 && 100 <= mx && mx < 150)
                {
                    bg = {0.9f, 0.9f, 0.9f};
                    if (window->is_key_down(oc::KeyCode::Mouse_1)) bg = {0.8f, 0.8f, 0.8f};
                }
                window->fill_rect(100, 0, 150, 50, bg);
                window->fill_rect(110, 10, 116, 40, {/*black*/});
                oc::render(*window, oc::polygon(118.0f, 10.0f, 140.0f, 25.0f, 118.0f, 40.0f), {/*black*/});

            }
            {// resend
                oc::Color bg {1.0f, 1.0f, 1.0f};
                if (my < 50 && 150 <= mx)
                {
                    bg = {0.9f, 0.9f, 0.9f};
                    if (window->is_key_down(oc::KeyCode::Mouse_1)) bg = {0.8f, 0.8f, 0.8f};
                }
                window->fill_rect(150, 0, 200, 50, bg);
                oc::render(
                    *window,
                    oc::polyline(
                        2.0f,
                        155.0f, 10.0f,
                        195.0f, 10.0f,
                        195.0f, 40.0f,
                        155.0f, 40.0f,
                        155.0f, 10.0f,
                        175.0f, 27.0f,
                        195.0f, 10.0f),
                    {/*black*/});
                oc::render(*window, oc::line(155.0f, 40.0f, 168.0f, 22.0f, 2.0f), {/*black*/});
                oc::render(*window, oc::line(195.0f, 40.0f, 182.0f, 22.0f, 2.0f), {/*black*/});
            }
            {// progress bar
                int x = window->get_width() * (int)video_frame_number / (int)frame_count;
                window->fill_rect(0, 50, x, 60, {1.0f, 0.0f, 0.0f});
                window->fill_rect(x, 50, window->get_width(), 60, {1.0f, 1.0f, 1.0f});
            }

            window->commit();
        }

        if (paused && !step && !resend) continue;

        ocTime frame_time = ocTime::now();

        // Grab the frame from the shared memory that we want to write to and
        // write all the frame info into it
        ocCamData *cam_data = &shared_memory->cam_data[index];
        cam_data->frame_time   = frame_time;
        cam_data->frame_number = frame_number;
        cam_data->width        = width;
        cam_data->height       = height;
        switch (channels)
        {
            case 1:
            {
                cam_data->pixel_format = ocPixelFormat::Gray_U8;
            } break;
            case 3:
            {
                cam_data->pixel_format = ocPixelFormat::Bgr_U8;
            } break;
            case 4:
            {
                cam_data->pixel_format = ocPixelFormat::Bgra_U8;
            } break;
        }

        if (image_input.data != nullptr)
        {
            // If our input is just an image, copy it over.
            memcpy(cam_data->img_buffer, image_input.data, width * height * channels);
        }
        else
        {
            // Otherwise decode the next video frame
            int type = CV_8UC1;
            if (1 < channels) type = CV_8UC3;
            cv::Mat video_frame((int)width, (int)height, type);

            if (video_input.read(video_frame))
            {
                memcpy(cam_data->img_buffer, video_frame.data, width * height * channels);
            }
            else
            {
                // If the video is over, restart form the beginning
                logger->log("Video finished - rewinding to start");
                video_input.set(cv::CAP_PROP_POS_FRAMES, 0);
            }
        }

        shared_memory->last_written_cam_data_index = index;

        // Announce, that the image is now ready in the shared memory
        ipc_packet.set_message_id(ocMessageId::Camera_Image_Available);
        ipc_packet.clear_and_edit()
            .write<ocTime>(frame_time)
            .write<uint32_t>(frame_number)
            .write<ptrdiff_t>((std::byte *)cam_data - (std::byte *)shared_memory)
            .write<size_t>(sizeof(*cam_data));
        socket->send_packet(ipc_packet);

        frame_number += 1;
        index = (index + 1) % OC_NUM_CAM_BUFFERS;
        if (!resend)
        {
            video_frame_number += 1;
        }
        else
        {
            // the frame number advances on its own, so we need to undo that.
            video_input.set(cv::CAP_PROP_POS_FRAMES, video_frame_number);
        }
        step   = false;
        resend = false;
    }
}
