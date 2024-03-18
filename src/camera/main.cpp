#include "../common/ocArgumentParser.h"
#include "../common/ocCommon.h"
#include "../common/ocConfigFileReader.h"
#include "../common/ocIpcSocket.h"
#include "../common/ocLogger.h"
#include "../common/ocMember.h"
#include "../common/ocProfiler.h"
#include "../common/ocTime.h"
#include "../common/ocTypes.h"

#include "ocCamera.h"

#include <csignal>

#define CAMERA_CONFIG_FILE "../camera.conf"
#define NUM_CAM_PIXEL_FORMATS 3

static bool running = true;

static const char* pixel_format_strings[NUM_CAM_PIXEL_FORMATS] = {
    "gray",
    "bgr",
    "bgra"
};
static const ocPixelFormat pixel_formats[NUM_CAM_PIXEL_FORMATS] = {
    ocPixelFormat::Gray_U8,
    ocPixelFormat::Bgr_U8,
    ocPixelFormat::Bgra_U8
};

static bool read_config_file(ocCameraSettings *settings, ocLogger *logger)
{
    ocConfigFileReader config;
    ocConfigReadReport result = config.read_file(CAMERA_CONFIG_FILE);
    if (ocConfigReadReport::Success != result)
    {
        if (ocConfigReadReport::File_Not_Accessible == result)
        {
            logger->log("No config file found.");
        }
        else
        {
            logger->error("Error while reading config file: %s (%i)", to_string(result), (int32_t)result);
            return false;
        }
    }

    config.get_uint32("binning_h", &settings->binning_h);
    config.get_uint32("binning_v", &settings->binning_v);
    config.get_uint32("subsampling_h", &settings->subsampling_h);
    config.get_uint32("subsampling_v", &settings->subsampling_v);
    size_t pfi;
    result = config.get_index("-pf", pixel_format_strings, NUM_CAM_PIXEL_FORMATS, &pfi);
    if (ocConfigReadReport::Success == result)
    {
        settings->pixel_format = pixel_formats[pfi];
    }
    else if (ocConfigReadReport::Range_Mismatch == result)
    {
        auto string = config.get_value("-pf");
        logger->error("Invalid pixel format: %s", string.data());
        logger->error("Valid pixel formats:");
        for (size_t i = 0; i < NUM_CAM_PIXEL_FORMATS; ++i)
        {
            logger->error("    %s", pixel_format_strings[i]);
        }
        return false;
    }
    config.get_int32("gain_percent", &settings->gain_percent);
    config.get_float32("framerate", &settings->frame_rate);
    config.get_float32("exposure", &settings->exposure_time_ms);
    config.get_bool("flip_lr", &settings->flip_left_right);
    config.get_bool("flip_ud", &settings->flip_up_down);
    config.get_bool("auto_shutter", &settings->auto_shutter);
    config.get_bool("gain_boost", &settings->gain_boost);
    return true;
}

static bool read_cmd_params(int argc, const char **argv, ocCameraSettings *settings, ocLogger *logger)
{
    ocArgumentParser arg_parser(argc, argv);
    arg_parser.get_uint32("-bh", &settings->binning_h);
    arg_parser.get_uint32("-bv", &settings->binning_v);
    arg_parser.get_uint32("-sh", &settings->subsampling_h);
    arg_parser.get_uint32("-sv", &settings->subsampling_v);
    size_t pfi;
    if (arg_parser.has_key("-pf"))
    {
        if (arg_parser.get_index("-pf", pixel_format_strings, NUM_CAM_PIXEL_FORMATS, &pfi))
        {
            settings->pixel_format = pixel_formats[pfi];
        }
        else
        {
            auto string = arg_parser.get_value("-pf");
            if (string.empty())
            {
                logger->error("Pixel format argument (-pf) given, but no format specified.");
            }
            else
            {
                logger->error("Invalid pixel format: %s", string.data());
            }
            logger->error("Valid pixel formats:");
            for (size_t i = 0; i < NUM_CAM_PIXEL_FORMATS; ++i)
            {
                logger->error("    %s", pixel_format_strings[i]);
            }
            return false;
        }
    }
    arg_parser.get_int32("-gp",     &settings->gain_percent);
    arg_parser.get_float32("-fps",  &settings->frame_rate);
    arg_parser.get_float32("-exp",  &settings->exposure_time_ms);
    if (arg_parser.has_key("-flr"))  settings->flip_left_right = true;
    if (arg_parser.has_key("-fud"))  settings->flip_up_down    = true;
    if (arg_parser.has_key("-as"))   settings->auto_shutter    = true;
    if (arg_parser.has_key("-gb"))   settings->gain_boost      = true;
    if (arg_parser.has_key("-!flr")) settings->flip_left_right = false;
    if (arg_parser.has_key("-!fud")) settings->flip_up_down    = false;
    if (arg_parser.has_key("-!as"))  settings->auto_shutter    = false;
    if (arg_parser.has_key("-!gb"))  settings->gain_boost      = false;
    return true;
}

static void signal_handler(int)
{
    running = false;
}

int main(int argc, const char **argv)
{
    ocCameraSettings settings = {};

    settings.binning_h = 1;
    settings.binning_v = 1;
    settings.subsampling_h = 1;
    settings.subsampling_v = 1;
    settings.pixel_format = ocPixelFormat::Bgr_U8;
    settings.gain_percent = 50;
    settings.frame_rate   = 30;
    settings.exposure_time_ms = 4.0f;
    settings.flip_left_right  = false;
    settings.flip_up_down     = false;
    settings.auto_shutter     = true;
    settings.white_balance    = true;
    settings.gain_boost       = false;

    ocMember member(ocMemberId::Camera, "Camera");
    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();
    ocLogger *logger = member.get_logger();

    read_config_file(&settings, logger) || die();
    read_cmd_params(argc, argv, &settings, logger) || die();

    logger->log(
        "{bh: %i bv: %i, sh: %i, sv: %i pf: %s, fps: %.2f, flr: %i, fud: %i, as: %i}",
        settings.binning_h,
        settings.binning_v,
        settings.subsampling_h,
        settings.subsampling_v,
        to_string(settings.pixel_format),
        settings.frame_rate,
        settings.flip_left_right,
        settings.flip_up_down,
        settings.auto_shutter);

    ocPacket ipc_packet;
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Set_Camera_Parameter)
        .write(ocMessageId::Request_Timing_Sites);
    socket->send_packet(ipc_packet);

    ocCamera cam;
    while (!cam.init(settings, shared_memory->cam_data, OC_NUM_CAM_BUFFERS))
    {
        logger->warn("main(): Retrying in 3 Seconds...");
        usleep(1000 * 1000 * 3);
    }

    // Do not kill yourself if send/recv fails
    signal(SIGPIPE, SIG_IGN);

    // Catch some signals to allow us to gracefully shut down the camera
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto current_second = ocTime::now().get_seconds();
    int32_t fps_count = 0;
    float frame_divergence = 0.0f;
    float expected_framerate = cam.get_expected_framerate();
    while (running)
    {
        uint32_t index = cam.get_image_index();
        shared_memory->cam_data[index].frame_number = 0;

        bool ret_camera = cam.read_frame();
        TIMED_BLOCK("check frame");

        ocTime now = ocTime::now();

        if (now.get_seconds() != current_second)
        {
            frame_divergence += (float)fps_count - expected_framerate;
            int int_divergence = (int)frame_divergence;
            if (1 <= frame_divergence)
            {
                logger->warn("main(): warning, gained %i frame%s.", int_divergence, 1 < int_divergence ? "s" : "");
                frame_divergence -= (float)int_divergence;
            }
            else if (frame_divergence <= -1)
            {
                logger->warn("main(): warning, missed %i frame%s.", -int_divergence, int_divergence < -1 ? "s" : "");
                frame_divergence -= (float)int_divergence;
            }
            fps_count = 0;
            current_second = now.get_seconds();
        }

        if (ret_camera)
        {
            ocTime   frame_time   = cam.get_image_time();
            uint32_t frame_number = cam.get_image_number();

            shared_memory->cam_data[index].frame_time   = frame_time;
            shared_memory->cam_data[index].frame_number = frame_number;
            shared_memory->last_written_cam_data_index  = index;

            ipc_packet.set_message_id(ocMessageId::Camera_Image_Available);
            ipc_packet.clear_and_edit()
                .write<ocTime>(frame_time)
                .write<uint32_t>(frame_number)
                .write<ptrdiff_t>((std::byte *)&shared_memory->cam_data[index] - (std::byte *)shared_memory) // offsetof doesn't work here, because the index isn't constexpr.
                .write<size_t>(sizeof(shared_memory->cam_data[index]));
            socket->send_packet(ipc_packet);

            ++fps_count;
        }

        NEXT_TIMED_BLOCK("process packets");

        int32_t socket_status;
        while (0 < (socket_status = socket->read_packet(ipc_packet, false)))
        {
            TIMED_BLOCK();
            switch(ipc_packet.get_message_id())
            {
                case ocMessageId::Set_Camera_Parameter:
                {
                    int32_t param_id;
                    double val1, val2;
                    ipc_packet.read_from_start()
                        .read<int>(&param_id)
                        .read<double>(&val1)
                        .read<double>(&val2);
                    if (!cam.set_parameter(param_id, val1, val2))
                    {
                        logger->error("Error while setting parameter %i, %f, %f", param_id, val1, val2);
                    }
                } break;
                case ocMessageId::Request_Timing_Sites:
                {
                    ipc_packet.set_sender(ocMemberId::Camera);
                    ipc_packet.set_message_id(ocMessageId::Timing_Sites);
                    if (write_timing_sites_to_buffer(ipc_packet.get_payload()))
                    {
                        socket->send_packet(ipc_packet);
                    }
                } break;
                default:
                {
                    ocMessageId msg_id = ipc_packet.get_message_id();
                    ocMemberId  mbr_id = ipc_packet.get_sender();
                    logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                } break;
            }
        }
        if (socket_status < 0)
        {
            logger->error("Error while reading the IPC socket: (%i) %s", errno, strerror(errno));
            running = false;
        }

        if (40 < timing_event_count())
        {
            TIMED_BLOCK("send timing data");
            ipc_packet.set_sender(ocMemberId::Camera);
            ipc_packet.set_message_id(ocMessageId::Timing_Events);
            while (write_timing_events_to_buffer(ipc_packet.get_payload()))
            {
                socket->send_packet(ipc_packet);
            }
        }
    }

    cam.exit();
}
