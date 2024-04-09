#include "../common/ocAlarm.h"
#include "../common/ocArgumentParser.h"
#include "../common/ocArray.h"
#include "../common/ocAssert.h"
#include "../common/ocCar.h"
#include "../common/ocCarConfig.h"
#include "../common/ocConfigFileReader.h"
#include "../common/ocFileWatcher.h"
#include "../common/ocMember.h"
#include "../common/ocPollEngine.h"
#include "../common/ocProfiler.h"
#include "../common/ocVec.h"
#include "../common/ocWindow.h"
#include "config.h"
#include "ocOverviewMap.h"
#include "ocRenderCamera.h"
#include "ocSimCar.h"
#include "ocSimulationWorld.h"
#include "ocTrackStore.h"

#include <cerrno> // errno
#include <cmath>

#include "detections/detection.h"
#include "detections/crosswalk_detection.h"
#include "detections/lane_detection.h"
#include "detections/obstacle_detection.h"
#include "detections/parking_space_detection.h"
#include "detections/pedestrian_detection.h"
#include "detections/road_marking_detection.h"
#include "detections/sign_detection.h"
#include "detections/stop_line_detection.h"

#define OC_USE_OPENCL 1

#if OC_USE_OPENCL
#include "ocOclRenderer.h"
#else
#include "ocCpuRenderer.h"
#endif

#define CAR_CONFIG_FILE "../car_properties.conf"

#define PI 3.14159265358979f

enum class ocVirtualizationMode
{
    None,
    Virtual_Camera,
    Virtual_Detections
};

Aab3 get_car_bounds(const ocCarState& state)
{
    float padding    = 2.0f;
    float height     = state.properties->cam.pose.pos.z;
    Vec3 corners[8] = {
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_fl().xy(), height)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_fl().xy(), 0.0f)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_fr().xy(), height)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_fr().xy(), 0.0f)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_rl().xy(), height)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_rl().xy(), 0.0f)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_rr().xy(), height)),
        state.pose.generalize_pos(Vec3(state.properties->wheel_center_rr().xy(), 0.0f)),
    };
    Aab3 result = {corners[0], corners[0]};
    for (int i = 1; i < 8; ++i)
    {
        result.min.x = std::min(result.min.x, corners[i].x);
        result.min.y = std::min(result.min.y, corners[i].y);
        result.min.z = std::min(result.min.z, corners[i].z);
        result.max.x = std::max(result.max.x, corners[i].x);
        result.max.y = std::max(result.max.y, corners[i].y);
        result.max.z = std::max(result.max.z, corners[i].z);
    }
    result.min -= Vec3(padding, padding, padding);
    result.max += Vec3(padding, padding, padding);
    return result;
}

int main(int argc, const char** argv)
{
    ocMember member(ocMemberId::Virtual_Car, "Virtual Car");
    ocLogger *logger = member.get_logger();
    ocArgumentParser arg_parser(argc, argv);

#if OC_USE_OPENCL
    int platform_number = -1;
    arg_parser.get_int32("-p", &platform_number);
    int device_number = -1;
    arg_parser.get_int32("-d", &device_number);

    ocOclRenderer renderer;
    renderer.init(platform_number, device_number) || die();
#else
    ocCpuRenderer renderer;
    renderer.init() || die();
#endif

    member.attach();

    ocIpcSocket *socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();

    ocTime reaction_time = ocTime::milliseconds(50);
    ocTime frame_time    = ocTime::hertz(30.0f);
    ocTime odo_time      = ocTime::hertz(100.0f);

    ocCarProperties car_properties;
    read_config_file(CAR_CONFIG_FILE, car_properties, *logger);

    ocSimulationSettings sim_settings = {
        .brightness     = 1.0f,
        .noise_strength = 0.2f,
        .initial_pos    = {0.0f, 25.0f, 0.0f},
        .initial_yaw    = 0.0f,
        .initial_pitch  = 0.0f,
        .initial_roll   = 0.0f,
    };
    read_config_file(SIM_CONFIG_FILE, &sim_settings, *logger);

    ocFileWatcher file_watcher(2);
    auto car_file = file_watcher.add_file(CAR_CONFIG_FILE);
    if (!car_file)
    {
        logger->warn("Could not watch file: %s", CAR_CONFIG_FILE);
    }
    auto sim_file = file_watcher.add_file(SIM_CONFIG_FILE);
    if (!sim_file)
    {
        logger->warn("Could not watch file: %s", SIM_CONFIG_FILE);
    }

    ocTime car_state_times[10] = {};
    ocCarState car_states[10] = {};
    ocCarAction car_actions[10] = {};
    int32_t task_number = 0;
    for (int i = 9; 0 <= i; --i)
    {
        car_state_times[i] = ocTime::now();
        car_states[i].properties = &car_properties;
        car_states[i].pose.pos   = sim_settings.initial_pos;
        car_states[i].pose.yaw   = sim_settings.initial_yaw;
        car_states[i].pose.pitch = sim_settings.initial_pitch;
        car_states[i].pose.roll  = sim_settings.initial_roll;
        car_actions[i].speed          = 0.0f;
        car_actions[i].steering_front = 0.0f;
        car_actions[i].steering_rear  = 0.0f;
    }

    int32_t image_width        = (int)car_properties.cam.image_width;
    int32_t image_height       = (int)car_properties.cam.image_height;
    ocPixelFormat pixel_format = car_properties.cam.pixel_format;
    int32_t image_channels     = (int)bytes_per_pixel(pixel_format);
    if (1 != image_channels)
    {
        image_channels = 4;
        pixel_format = ocPixelFormat::Bgra_U8;
    }

    bool show_ui     = true;
    bool show_bounds = false;
    bool show_fov    = false;
    bool show_grid   = false;
    bool restart_on_error = false;
    bool car_selected = false;
    ocVirtualObject *selected_object = nullptr;
    ocRoadTile      *selected_tile   = nullptr;
    bool show_manipulator   = false;
    ocPose manipulator_pose = {};
    ManipulatorState manipulator_state = ManipulatorState::Default;

    ocVirtualizationMode virtualization_mode = ocVirtualizationMode::None;

    if (arg_parser.has_key_with_value("-vm", "cam"))
    {
        virtualization_mode = ocVirtualizationMode::Virtual_Camera;
    }
    else
    {
        virtualization_mode = ocVirtualizationMode::Virtual_Detections;
    }

    if (ocVirtualizationMode::Virtual_Camera == virtualization_mode)
    {
        arg_parser.get_int32("-cw", &image_width);
        if (image_width <= 0)
        {
            logger->error("Camera width can't be less or equal to 0.");
            return -1;
        }
        car_properties.cam.image_width = (uint32_t)image_width;

        arg_parser.get_int32("-ch", &image_height);
        if (image_height <= 0)
        {
            logger->error("Camera height can't be less or equal to 0.");
            return -1;
        }
        car_properties.cam.image_height = (uint32_t)image_height;

        if (arg_parser.has_key("-cpf"))
        {
            const char* pixel_formats[2] = {
                "gray",
                "bgra",
            };
            size_t pfi;
            if (arg_parser.get_index("-cpf", pixel_formats, 2, &pfi))
            {
                if (0 == pfi)
                {
                    pixel_format = ocPixelFormat::Gray_U8;
                    image_channels = 1;
                }
                else
                {
                    pixel_format = ocPixelFormat::Bgra_U8;
                    image_channels = 4;
                }
            }
            else
            {
                auto string = arg_parser.get_value("-cpf");
                if (string.empty())
                {
                    logger->error("Pixel format argument (-pf) given, but no format specified.");
                }
                else
                {
                    logger->error("Invalid pixel format: %s", string.data());
                }
                logger->error("Valid pixel formats:");
                for (size_t i = 0; i < 2; ++i)
                {
                    logger->error("    %s", pixel_formats[i]);
                }
                return -1;
            }
        }
        car_properties.cam.pixel_format = pixel_format;
    }

    DrawContext draw_context = {
        .scale = 1.0f,
        .offset = {},
        .width  = 700,
        .height = 700
    };

    if (arg_parser.has_key("-no-ui"))
    {
        show_ui = false;
    }
    else
    {
        arg_parser.get_int32("-ow", &draw_context.width);
        if (draw_context.width <= 0)
        {
            logger->error("Overview width can't be less or equal to 0.");
            return -1;
        }

        arg_parser.get_int32("-oh", &draw_context.height);
        if (draw_context.height <= 0)
        {
            logger->error("Overview height can't be less or equal to 0.");
            return -1;
        }
    }

    const int world_width = 5;
    const int world_height = 5;
    const int num_tiles = world_width * world_height;
    ocRoadTileType world_tile_types[num_tiles] = {
        ocRoadTileType::Curve_Se,                 ocRoadTileType::Curve_Sw_No_Passing_O,    ocRoadTileType::Curve_Se_No_Passing_O, ocRoadTileType::Straight_Ew_No_Passing_S, ocRoadTileType::Curve_Sw_No_Passing_O,
        ocRoadTileType::Straight_Sn_No_Passing_E, ocRoadTileType::Straight_Ns,              ocRoadTileType::Curve_Ne_No_Passing_O, ocRoadTileType::Curve_Sw_No_Passing_O,    ocRoadTileType::Straight_Sn_No_Passing_E,
        ocRoadTileType::Parking_Ns,               ocRoadTileType::Straight_Ns,              ocRoadTileType::Intersection_Turn_Nw,  ocRoadTileType::Straight_Sn_No_Passing_E, ocRoadTileType::Crosswalk_Sn,
        ocRoadTileType::Straight_Sn_No_Passing_E, ocRoadTileType::Curve_Ne_No_Passing_O,    ocRoadTileType::Intersection_Yield_All,ocRoadTileType::Curve_Nw_No_Passing_O,    ocRoadTileType::Straight_Sn_No_Passing_E,
        ocRoadTileType::Curve_Ne_No_Passing_O,    ocRoadTileType::Barred_Area_We,           ocRoadTileType::Intersection_Stop_We,  ocRoadTileType::Straight_Ew_No_Passing_S, ocRoadTileType::Curve_Nw_No_Passing_O,
    };
    ocRoadTile world_tiles[num_tiles];
    for(int y = 0; y < world_height; ++y)
    for(int x = 0; x < world_width; ++x)
    {
        world_tiles[x + y * world_width] = {world_tile_types[x + y * world_width], x, y};
    }

    Vec3 sun_dir = normalize(Vec3(1, 0, 1));

    draw_context.center_at({-100.0f, -100.0f, (float)world_width * 200.0f - 100.0f, (float)world_height * 200.0f - 100.0f});

    // TODO: simulate the world too, and have a history of world states (moving obstacles etc)
    ocSimulationWorld sim_data = {
        .world_width   = world_width,
        .world_height  = world_height,
        .world         = world_tiles,
        .world_objects = {},
        .user_objects  = {
            ocVirtualObject {
                .type = ocObjectType::Obstacle,
                .pose = ocPose({220.0f, 200.0f, 8.0f}, 0.0f, 0.0f, 0.0f),
                .size = {16.0f, 16.0f, 16.0f}
            }
        },
        .triggers        = {},
        .triggers_active = false
    };
    ocTime trigger_timer = ocTime::null();

    bool update_overview = true;
    Rect overview_dirty = draw_context.get_visible_world_rect();

    auto schedule_redraw = [&](Rect rect)
    {
        if (!update_overview)
        {
            update_overview = true;
            overview_dirty = rect;
        }
        else
        {
            overview_dirty = overview_dirty.merge(rect);
        }
    };
    auto redraw_object = [&](const ocVirtualObject& object)
    {
        auto bounds = object.facing_up().bounds();
        schedule_redraw({bounds.min.xy(), bounds.max.xy()});
    };

    simCrosswalkDetection    crosswalk_detection;
    simLaneDetection         lane_detection;
    simObstacleDetection     obstacle_detection;
    simParkingSpaceDetection parking_space_detection;
    simPedestrianDetection   pedestrian_detection;
    simRoadMarkingDetection  road_marking_detection;
    simSignDetection         sign_detection;
    simStopLineDetection     stop_line_detection;
    ocArray<simDetection*> detections {
        &crosswalk_detection,
        &lane_detection,
        &obstacle_detection,
        &parking_space_detection,
        &pedestrian_detection,
        &road_marking_detection,
        &sign_detection,
        &stop_line_detection
    };

    // local function (lambda) that sends a packet to both the IPC socket and the detection objects
    auto send_packet = [&](ocPacket& packet) {
        packet.set_sender(ocMemberId::Virtual_Car);
        socket->send_packet(packet);
        for (auto det : detections) det->handle_packet(packet);
    };

    sim_data.regenerate_objects();
    sim_data.regenerate_triggers();

    ocRenderCamera overview_cam = {};
    ocRenderCamera car_cam = {};

    renderer.init_perspective_camera(
        &car_cam,
        image_width,
        image_height,
        image_channels,
        car_properties.cam.sensor_offset_x,
        car_properties.cam.sensor_offset_y,
        car_properties.cam.fov,
        car_properties.cam.distortion);

    ocTime frame_car_time = ocTime::now();

    if (ocVirtualizationMode::Virtual_Camera == virtualization_mode)
    {
        ocCarState *car = &car_states[0];

        auto visible_objects = sim_data.get_objects_visible_from(*car);
        renderer.set_world(
            sim_data.world,
            (uint32_t)sim_data.world_width,
            (uint32_t)sim_data.world_height,
            visible_objects,
            sun_dir
        ) || die();

        renderer.set_camera_pose(
            ocPose::compose(car->pose, car_properties.cam.pose)
        ) || die();
        renderer.set_camera_properties(&car_cam, sim_settings.noise_strength, sim_settings.brightness, false) || die();
        renderer.start_rendering() || die();
    }

    ocPacket s(ocMessageId::Subscribe_To_Messages);
    s.clear_and_edit()
        .write(ocMessageId::Lane_Found)
        .write(ocMessageId::Object_Found)
        .write(ocMessageId::Send_Can_Frame)
        .write(ocMessageId::Set_Lights)
        .write(ocMessageId::Start_Driving_Task)
        .write(ocMessageId::Request_Timing_Sites);
    socket->send_packet(s);

    ocPacket ipc_packet;

    bool running = true;
    ocTime prev_time = ocTime::now();

    bool send_steps = true;
    bool send_speed = true;
    bool follow_car = false;

    auto reset_car_state = [&]()
    {
        car_actions[0].speed = 0.0f;
        car_actions[0].steering_front = 0.0f;
        car_actions[0].steering_rear = 0.0f;
        car_actions[0].stop_after = 0.0f;
        car_states[0].velocity.x = 0.0f;
        car_states[0].velocity.y = 0.0f;
        car_states[0].angular_velocity = 0.0f;
        car_states[0].steering_front = 0.0f;
        car_states[0].steering_rear = 0.0f;
        car_states[0].pose.pos   = sim_settings.initial_pos;
        car_states[0].pose.yaw   = sim_settings.initial_yaw;
        car_states[0].pose.pitch = sim_settings.initial_pitch;
        car_states[0].pose.roll  = sim_settings.initial_roll;
    };

    auto set_rc_mode = [&](bool is_on)
    {
        car_states[0].rc_is_active = is_on;
        ipc_packet.set_message_id(ocMessageId::Rc_State_Changed);
        ipc_packet.clear_and_edit().write<uint8_t>(is_on ? 0xFF : 0x00);
        send_packet(ipc_packet);
    };

    ocAlarm cam_timer(frame_time);
    cam_timer.start(ocAlarmType::Periodic);

    ocAlarm odo_timer(odo_time);
    odo_timer.start(ocAlarmType::Periodic);

    ocPollEngine pe(10);
    pe.add_fd(cam_timer.get_fd());
    pe.add_fd(odo_timer.get_fd());
    pe.add_fd(socket->get_fd());
    pe.add_fd(renderer.wait_fd);

    // Base image of the track that is rendered via raytracing whenever the track or camera changes.
    float *overview_base = nullptr;
    oc::Window *window   = nullptr;
    bool show_controls   = false;
    if (show_ui)
    {
        window = new oc::Window(draw_context.width, draw_context.height, "Virtual Car", true);
        overview_base = (float *)malloc((size_t)(draw_context.width * draw_context.height) * 4 * sizeof(float));
    }

    uint32_t frame_number = 0;
    uint32_t frame_index = 0;

/*
    ocTime fps_prev_time = ocTime::now();
    int fps = 0;
    int fps_counter = 0;
*/

    bool rendering_done = false;
    bool cam_timer_expired = false;

    bool was_offroad = false;
    ocTime offroad_time = ocTime::null();

    //bool parking_ok = true;

    while (running)
    {
        pe.await();

        if (file_watcher.check_for_changes())
        {
            if (file_watcher.has_changed(car_file))
            {
                logger->log("Change in car config detected, loading.");
                read_config_file(CAR_CONFIG_FILE, car_properties, *logger);
                if (ocVirtualizationMode::Virtual_Camera == virtualization_mode)
                {
                    renderer.init_perspective_camera(
                        &car_cam,
                        image_width,
                        image_height,
                        image_channels,
                        car_properties.cam.sensor_offset_x,
                        car_properties.cam.sensor_offset_y,
                        car_properties.cam.fov,
                        car_properties.cam.distortion);
                }
            }
            if (file_watcher.has_changed(sim_file))
            {
                logger->log("Change in sim config detected, loading.");
                read_config_file(SIM_CONFIG_FILE, &sim_settings, *logger);
            }
        }

        TIMED_BLOCK("work");

        ocTime now = ocTime::now();
        ocTime diff = now - prev_time;
        if (ocTime::null() < trigger_timer) // detect a zero crossing of the timer
        {
            trigger_timer -= diff;
            if (ocTime::null() < trigger_timer)
            {
                sim_data.triggers_active = false;
            }
        }
        prev_time = now;

        {
            TIMED_BLOCK("simulate car");
            for (int i = 9; 0 < i; --i)
            {
                car_state_times[i] = car_state_times[i - 1];
                car_states[i] = car_states[i - 1];
                car_actions[i] = car_actions[i - 1];
            }
            car_states[0] = simulate_car_dyn(car_states[0], car_actions[0], diff.get_float_seconds(), 0.0001f);
            car_state_times[0] = now;
        }

        {
            TIMED_BLOCK("Check collisions");
            auto& car = car_states[0];
            auto& pose = car.pose;
            bool is_offroad = false;

            // Check if the car has left the whole track alltogether.
            float world_west  = -100.0f;
            float world_north = -100.0f;
            float world_east  = (float)sim_data.world_width * 200.0f - 100.0f;
            float world_south = (float)sim_data.world_height * 200.0f - 100.0f;
            if (car.pose.pos.x < world_west  || world_east < car.pose.pos.x ||
                car.pose.pos.y < world_north || world_south < car.pose.pos.y)
            {
                is_offroad = true;
            }

            // If the car is still on the track, check against bounds provided by the tiles.
            // We're checking just the four center points of the wheels.
            Vec2 front_left  = pose.generalize_pos(car.properties->wheel_center_fl()).xy();
            Vec2 front_right = pose.generalize_pos(car.properties->wheel_center_fr()).xy();
            Vec2 rear_left   = pose.generalize_pos(car.properties->wheel_center_rl()).xy();
            Vec2 rear_right  = pose.generalize_pos(car.properties->wheel_center_rr()).xy();

            for (auto &trigger : sim_data.triggers)
            {
                if (trigger.contains(front_left)  ||
                    trigger.contains(front_right) ||
                    trigger.contains(rear_left)   ||
                    trigger.contains(rear_right))
                {
                    if (trigger.no_driving && (!trigger.triggerable || sim_data.triggers_active))
                    {
                        is_offroad = true;
                        break;
                    }
                    if (trigger.trigger_on)  sim_data.triggers_active = true;
                    if (trigger.trigger_off) sim_data.triggers_active = false;
                    if (ocTime::null() != trigger.trigger_time) trigger_timer = trigger.trigger_time;

/*
                    // only allow parking if all wheels are inside the trigger
                    if (trigger.contains(front_left) &&
                        trigger.contains(front_right) &&
                        trigger.contains(rear_left) &&
                        trigger.contains(rear_right))
                    {
                        if (trigger.parking_ok)  parking_ok = true;
                    }
*/
                }
            }
            if (is_offroad)
            {
                if (was_offroad)
                {
                    offroad_time += diff;
                }
                was_offroad = true;
            }
            else
            {
                offroad_time = ocTime::null();
                was_offroad = false;
            }

            if (restart_on_error && ocTime::seconds(1) <= offroad_time)
            {
                logger->warn("Car was off the road for too long, resetting!");
                reset_car_state();
            }
        }

        if (pe.was_triggered(socket->get_fd()))
        {
            TIMED_BLOCK("handle IPC");
            int status;
            while (0 < (status = socket->read_packet(ipc_packet, false)))
            {
                switch (ipc_packet.get_message_id())
                {
                case ocMessageId::Lane_Found:   break;
                case ocMessageId::Object_Found: break;
                case ocMessageId::Send_Can_Frame:
                {
                    // TODO: check if can frame is relevant to simulation
                } break;
                case ocMessageId::Set_Lights:
                {
                    car_states[0].lights = ipc_packet.read_from_start().read<ocCarLights>();
                } break;
                case ocMessageId::Start_Driving_Task:
                {
                    auto reader = ipc_packet.read_from_start();
                    int16_t speed = reader.read<int16_t>();
                    int8_t sf = reader.read<int8_t>();
                    int8_t sr = reader.read<int8_t>();
                    uint8_t nr = reader.read<uint8_t>();
                    int32_t steps = reader.read<int32_t>();
                    if (!car_states[0].rc_is_active)
                    {
                        car_actions[0].speed = speed;
                        car_actions[0].steering_front = car_properties.byte_to_front_steering_angle(sf);
                        car_actions[0].steering_rear  = car_properties.byte_to_rear_steering_angle(sr);
                    }
                    car_actions[0].stop_after = (nr & 0x80) ? true : false;
                    car_actions[0].distance = car_properties.steps_to_cm((float)steps);
                    task_number = nr;
                    oc_assert(0 != task_number);
                } break;
                case ocMessageId::Request_Timing_Sites:
                {
                    ipc_packet.set_sender(ocMemberId::Virtual_Car);
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
                } // End switch

                // Send packets to the detections. Ignore those that come
                // from the simulation, because they were already passed
                // to the detections internally.
                if (ocMemberId::Virtual_Car != ipc_packet.get_sender())
                {
                    for (auto det : detections) det->handle_packet(ipc_packet);
                }
            }
            if (status < 0)
            {
                logger->error("Error reading the IPC socket: (%i) %s", errno, strerror(errno));
                running = false;
            }
        }

        Vec2 mouse_in_world = draw_context.screen_to_world(window->get_mouse_x(), window->get_mouse_y()).xy();

        if (odo_timer.is_expired())
        {
            TIMED_BLOCK("send odometry");
            ocCarState&  state  = car_states[0];
            ocCarAction& action = car_actions[0];
            if (send_speed)
            {
                //Vec2 fx = angle_to_vector(car_states[0].pose.rot_z + car_states[0].steering_front);
                ipc_packet.set_message_id(ocMessageId::Received_Current_Speed);
                ipc_packet.clear_and_edit()
                    .write<int16_t>((int16_t)((int)state.speed() / 6 * 6))
                    .write<ocTime>(car_state_times[0]);
                send_packet(ipc_packet);
            }
            if (send_steps)
            {
                ipc_packet.set_message_id(ocMessageId::Received_Odo_Steps);
                ipc_packet.clear_and_edit()
                    .write<int32_t>((int32_t)state.odo_steps())
                    .write<ocTime>(car_state_times[0]);
                send_packet(ipc_packet);
            }
            if (task_number && // we have a task
                ((action.speed < 0.0f && state.milage() < action.distance) || // driven backward far enough
                 (0.0f < action.speed && action.distance < state.milage())))  // driven forward far enough
            {
                ipc_packet.set_message_id(ocMessageId::Driving_Task_Finished);
                ipc_packet.clear_and_edit().write<uint8_t>((uint8_t)task_number);
                send_packet(ipc_packet);
                task_number = 0;
            }
        }

        if (pe.was_triggered(renderer.wait_fd))
        {
            TIMED_BLOCK("Rendering done");
            oc_assert(ocVirtualizationMode::Virtual_Camera == virtualization_mode);
            rendering_done = true;

            ocCamData *cam_data = &shared_memory->cam_data[frame_index];
            uint8_t *cam_buffer = cam_data->img_buffer;

            renderer.get_rendered_image(cam_buffer, (size_t)image_width * bytes_per_pixel(pixel_format));

            cam_data->width        = (uint32_t)image_width;
            cam_data->height       = (uint32_t)image_height;
            cam_data->pixel_format = pixel_format;
            cam_data->frame_number = frame_number;
            cam_data->frame_time   = frame_car_time;
            shared_memory->last_written_cam_data_index = frame_index;

            ipc_packet.set_message_id(ocMessageId::Camera_Image_Available);
            ipc_packet.clear_and_edit()
                .write<ocTime>(frame_car_time)
                .write<uint32_t>(frame_number)
                .write<ptrdiff_t>((std::byte *)cam_data - (std::byte *)shared_memory)
                .write<size_t>(sizeof(*cam_data));
            send_packet(ipc_packet);

            frame_number += 1;
            frame_index = (frame_index + 1) % OC_NUM_CAM_BUFFERS;
        }

        if (cam_timer.is_expired())
        {
            TIMED_BLOCK("Cam Timer expired");
            cam_timer_expired = true;
        }

        if ((virtualization_mode != ocVirtualizationMode::Virtual_Camera || rendering_done) && cam_timer_expired)
        {
            rendering_done = false;
            cam_timer_expired = false;
/*
            ocTime fps_now = ocTime::now();
            if (fps_prev_time.get_seconds() < fps_now.get_seconds())
            {
                fps = fps_counter;
                fps_counter = 0;
            }
            fps_prev_time = fps_now;
            fps_counter++;
*/
            ocTime car_state_age = reaction_time;
            if (ocVirtualizationMode::Virtual_Camera == virtualization_mode) car_state_age -= frame_time;
            ocCarState  *car    = nullptr;
            ocCarAction *action = nullptr;

            for (int i = 0; i < 10; ++i)
            {
                if (car_state_age <= now - car_state_times[i])
                {
                    car    = &car_states[i];
                    action = &car_actions[i];
                    frame_car_time = car_state_times[i];
                    break;
                }
            }
            oc_assert(car);
            oc_assert(action);

            if (follow_car)
            {
                auto old_off = draw_context.offset;
                draw_context.center_at(car->pose.pos.xy());
                if (old_off != draw_context.offset)
                {
                    schedule_redraw(draw_context.get_visible_world_rect());
                }
            }

            if (show_ui && update_overview)
            {
                TIMED_BLOCK("Render overview");
                update_overview = false;

                Vec2 a = draw_context.world_to_screen(overview_dirty.top_left());
                Vec2 b = draw_context.world_to_screen(overview_dirty.bottom_right());
                int screen_x0 = std::clamp((int)std::floor(a.x), 0, draw_context.width);
                int screen_y0 = std::clamp((int)std::floor(a.y), 0, draw_context.height);
                int screen_x1 = std::clamp((int)std::ceil(b.x),  0, draw_context.width);
                int screen_y1 = std::clamp((int)std::ceil(b.y),  0, draw_context.height);

                if (screen_x0 != screen_x1 && screen_y0 != screen_y1)
                {
                    Vec2 c = draw_context.screen_to_world((float)screen_x0, (float)screen_y0).xy();
                    Vec2 d = draw_context.screen_to_world((float)screen_x1, (float)screen_y1).xy();

                    renderer.init_ortho_camera(
                        &overview_cam,
                        screen_x1 - screen_x0,
                        screen_y1 - screen_y0,
                        4,
                        d.x - c.x,
                        d.y - c.y);

                    renderer.set_camera_properties(&overview_cam, 0.0f, 1.0f, true) || die();
                    renderer.set_camera_pose(
                        ocPose(Vec3((c + d) * 0.5f, 700), 0, PI * 0.5f, 0)
                    ) || die();

                    auto objects = sim_data.get_all_objects_facing_up();
                    renderer.set_world(
                        sim_data.world,
                        (uint32_t)sim_data.world_width,
                        (uint32_t)sim_data.world_height,
                        objects,
                        sun_dir
                    ) || die();

                    renderer.start_rendering() || die();
                    renderer.get_rendered_image(
                        &overview_base[(screen_x0 + screen_y0 * draw_context.width) * 4],
                        (size_t)draw_context.width * 4 * sizeof(float)
                    ) || die();

                    if (ocVirtualizationMode::Virtual_Camera == virtualization_mode)
                    {
                        renderer.set_camera_properties(&car_cam, sim_settings.noise_strength, sim_settings.brightness, false) || die();
                    }
                }
            }

            if (ocVirtualizationMode::Virtual_Camera == virtualization_mode)
            {
                TIMED_BLOCK("start new image");

                renderer.set_camera_pose(
                    ocPose::compose(car->pose, car_properties.cam.pose)
                ) || die();

                auto visible_objects = sim_data.get_objects_visible_from(*car);
                renderer.set_world(
                    sim_data.world,
                    (uint32_t)sim_data.world_width,
                    (uint32_t)sim_data.world_height,
                    visible_objects,
                    sun_dir
                ) || die();

                renderer.set_camera_properties(&car_cam, sim_settings.noise_strength, sim_settings.brightness, false) || die();

                renderer.start_rendering() || die();
            }
            else
            {
                TIMED_BLOCK("emulate detection processes");

                for (auto det : detections)
                {
                    det->run_detection(sim_data, *car);
                }
                for (auto det : detections)
                {
                    while (det->next_object(ipc_packet))
                    {
                        send_packet(ipc_packet);
                    }
                }

                frame_number += 1;
                frame_index = (frame_index + 1) % OC_NUM_CAM_BUFFERS;
            }

            if (show_ui)
            {
                TIMED_BLOCK("draw map");
                draw_map(*window, draw_context.width, draw_context.height, overview_base);

                NEXT_TIMED_BLOCK("draw detections");
                for (auto det : detections)
                {
                    det->draw_ui(*window, draw_context, sim_data, *car);
                }

                if (show_grid) draw_grid(*window, draw_context);

                if (show_bounds)
                {
                    NEXT_TIMED_BLOCK("draw areas");
                    draw_areas(*window, sim_data, draw_context);
                }

                NEXT_TIMED_BLOCK("draw car");
                draw_car(*window, *car, draw_context);
                if (task_number) draw_action(*window, *car, *action, draw_context);
                if (show_fov) draw_fov(*window, *car, draw_context);

                NEXT_TIMED_BLOCK("draw selection");
                bool show_selection = false;
                Rect bounds;
                if (car_selected)
                {
                    show_manipulator = true;
                    show_selection   = true;
                    bounds = get_car_bounds(*car).xy();
                    manipulator_pose = car->pose;
                }
                else if (selected_object)
                {
                    show_manipulator = true;
                    show_selection   = true;
                    bounds = selected_object->facing_up().bounds().xy();
                    manipulator_pose = selected_object->pose;
                }
                else if (selected_tile)
                {
                    show_manipulator = false;
                    show_selection   = true;
                    bounds = selected_tile->bounds();
                    manipulator_pose.pos = Vec3(INFINITY, INFINITY, INFINITY);
                }
                else
                {
                    show_manipulator = false;
                    manipulator_pose.pos = Vec3(INFINITY, INFINITY, INFINITY);
                }

                if (show_selection)
                {
                    auto min = draw_context.world_to_screen(Vec2(bounds.min_x, bounds.min_y));
                    auto max = draw_context.world_to_screen(Vec2(bounds.max_x, bounds.max_y));
                    oc::render(
                        *window,
                        oc::outline(oc::box(min, max), 1.0f),
                        {1.0f, 0.0f, 0.0f});
                }
                if (show_manipulator)
                {
                    Vec2 pos = draw_context.world_to_screen(manipulator_pose.pos);
                    float angle = manipulator_pose.heading;
                    draw_manipulator(*window, pos, angle, manipulator_state);
                }

                NEXT_TIMED_BLOCK("draw help text");
                draw_help_text(*window);

                NEXT_TIMED_BLOCK("commit");
                window->commit();
            }
        }

        while (true)
        {
            TIMED_BLOCK("handle user input");

            auto event = window->next_event();
            if (event.type == oc::EventType::Draw) break;
            if (event.type == oc::EventType::Close) return 0;

            if (event.type == oc::EventType::Key) {
                if(event.key.code == oc::KeyCode::Mouse_1)
                {
                    Vec2 pos = draw_context.world_to_screen(manipulator_pose.pos);
                    float angle = manipulator_pose.heading;
                    Vec2 mouse_pos = Vec2(window->get_mouse_x(), window->get_mouse_y());
                    manipulator_state = get_manipulator_state(pos, angle, mouse_pos, event.key.down);
                }

                if(event.key.down)
                {
                    switch (event.key.code)
                    {
                        case oc::KeyCode::Mouse_1:
                        {   
                            if(manipulator_state == ManipulatorState::Default) {
                                if(is_point_in_car(car_states[0], mouse_in_world)) {
                                    car_selected    = true;
                                    selected_object = nullptr;
                                    selected_tile   = nullptr;
                                } else {
                                    car_selected    = false;
                                    selected_object = sim_data.get_object_at(mouse_in_world);
                                    if (nullptr == selected_object)
                                    {
                                        selected_tile = sim_data.get_tile_at(mouse_in_world);
                                    }
                                }
                            }
                        } break;
                        case oc::KeyCode::Key_1: // blue button: free drive
                        {
                            ipc_packet.set_message_id(ocMessageId::Received_Button_Press);
                            ipc_packet.clear_and_edit().write<int32_t>(1);
                            send_packet(ipc_packet);
                        } break;
                        case oc::KeyCode::Key_2: // red button: obstacle drive
                        {
                            ipc_packet.set_message_id(ocMessageId::Received_Button_Press);
                            ipc_packet.clear_and_edit().write<int32_t>(2);
                            send_packet(ipc_packet);
                        } break;
                        case oc::KeyCode::Key_Space: // toggle remote control
                        {
                            set_rc_mode(!car_states[0].rc_is_active);
                            car_actions[0].speed = 0.0f;
                            car_actions[0].steering_front = 0.0f;
                            car_actions[0].steering_rear = 0.0f;
                        } break;
                        // TODO: implement remote controlling
                        case oc::KeyCode::Key_R: // reset car speed, position and orientation
                        {
                            set_rc_mode(true);
                            reset_car_state();
                            draw_context.center_at({-100.0f, -100.0f, (float)world_width * 200.0f - 100.0f, (float)world_height * 200.0f - 100.0f});
                            schedule_redraw(draw_context.get_visible_world_rect());
                        } break;
                        case oc::KeyCode::Key_L:
                        {
                            auto result = load_track("../sim_track.bin", sim_data);
                            if (result != ocTrackStoreReport::Success)
                            {
                                logger->warn("Could not load track. Error: %s", to_string(result)); 
                            }
                            schedule_redraw(draw_context.get_visible_world_rect());
                        } break;
                        case oc::KeyCode::Key_S:
                        {
                            auto result = save_track("../sim_track.bin", sim_data);
                            if (result != ocTrackStoreReport::Success)
                            {
                                logger->warn("Could not save track. Error: %s", to_string(result)); 
                            }
                        } break;
                        case oc::KeyCode::Key_B:
                        {
                            show_bounds = !show_bounds;
                        } break;
                        case oc::KeyCode::Key_C:
                        {
                            show_controls = !show_controls;
                        } break;
                        case oc::KeyCode::Key_E:
                        {
                            restart_on_error = !restart_on_error;
                        } break;
                        case oc::KeyCode::Key_F:
                        {
                            follow_car = !follow_car;
                        } break;
                        case oc::KeyCode::Key_G:
                        {
                            show_grid = !show_grid;
                        } break;
                        case oc::KeyCode::Key_V:
                        {
                            show_fov = !show_fov;
                        } break;
                        case oc::KeyCode::Key_O:
                        {
                            ocVirtualObject box = {
                                .type = ocObjectType::Obstacle,
                                .pose = ocPose({mouse_in_world.x, mouse_in_world.y, 8.0f}, 0.0f, 0.0f, 0.0f),
                                .size = {20.0f, 20.0f, 16.0f}
                            };
                            sim_data.add_object(box);
                            redraw_object(box);
                        } break;
                        case oc::KeyCode::Key_P:
                        {
                            ocVirtualObject ped = {
                                .type = ocObjectType::Pedestrian,
                                .pose = ocPose({mouse_in_world.x, mouse_in_world.y, 7.5f}, 0.0f, 0.0f, 0.0f),
                                .size = {5.0f, 10.0f, 15.0f}
                            };
                            sim_data.add_object(ped);
                            redraw_object(ped);
                        } break;
                        case oc::KeyCode::Key_Escape:
                        {
                            car_selected    = false;
                            selected_object = nullptr;
                            selected_tile   = nullptr;
                        } break;
                        case oc::KeyCode::Key_Delete:
                        {
                            if (selected_object)
                            {
                                redraw_object(*selected_object);
                                sim_data.remove_object(selected_object);
                            }
                            car_selected    = false;
                            selected_object = nullptr;
                            selected_tile   = nullptr;
                        }
                        default: break;
                        case oc::KeyCode::Key_Arrow_Left:
                        {
                            if (selected_tile)
                            {
                                int old_rot = (int)selected_tile->type % 4;
                                int new_rot = (old_rot + 1) % 4;
                                selected_tile->type = (ocRoadTileType)((int)selected_tile->type - old_rot + new_rot);
                                sim_data.regenerate_objects();
                                sim_data.regenerate_triggers();
                                schedule_redraw(draw_context.get_visible_world_rect()); // Unfortunately we need to redraw everything, due to the regenerated objects
                            }
                        } break;
                        case oc::KeyCode::Key_Arrow_Right:
                        {
                            if (selected_tile)
                            {
                                int old_rot = (int)selected_tile->type % 4;
                                int new_rot = (old_rot + 3) % 4;
                                selected_tile->type = (ocRoadTileType)((int)selected_tile->type - old_rot + new_rot);
                                sim_data.regenerate_objects();
                                sim_data.regenerate_triggers();
                                schedule_redraw(draw_context.get_visible_world_rect()); // Unfortunately we need to redraw everything, due to the regenerated objects
                            }
                        } break;
                        case oc::KeyCode::Key_Arrow_Up:
                        {
                            if (selected_tile)
                            {
                                int old_type = (int)selected_tile->type;
                                int new_type = (old_type + 4) % (int)ocRoadTileType::Count;
                                selected_tile->type = (ocRoadTileType)(new_type);
                                sim_data.regenerate_objects();
                                sim_data.regenerate_triggers();
                                schedule_redraw(draw_context.get_visible_world_rect()); // Unfortunately we need to redraw everything, due to the regenerated objects
                            }
                        } break;
                        case oc::KeyCode::Key_Arrow_Down:
                        {
                            if (selected_tile)
                            {
                                int old_type = (int)selected_tile->type;
                                int new_type = (old_type + (int)ocRoadTileType::Count - 4) % (int)ocRoadTileType::Count;
                                selected_tile->type = (ocRoadTileType)(new_type);
                                sim_data.regenerate_objects();
                                sim_data.regenerate_triggers();
                                schedule_redraw(draw_context.get_visible_world_rect()); // Unfortunately we need to redraw everything, due to the regenerated objects
                            }
                        } break;
                    }
                } 
            }
            if (event.type == oc::EventType::Pointer)
            {
                if (manipulator_state == ManipulatorState::XY_Active)
                {
                    float dx = (event.pointer.new_x - event.pointer.old_x) / draw_context.scale;
                    float dy = (event.pointer.new_y - event.pointer.old_y) / draw_context.scale;

                    if(car_selected) {
                        for (int i = 0; i < 10; ++i)
                        {
                            car_states[i].pose.pos.x += dx;
                            car_states[i].pose.pos.y += dy;
                        }
                    } else {
                        redraw_object(*selected_object);
                        selected_object->move(dx, dy);
                        redraw_object(*selected_object);
                    }
                    
                }
                if (manipulator_state == ManipulatorState::X_Active)
                {
                    float dx = (event.pointer.new_x - event.pointer.old_x) / draw_context.scale;
                    if (car_selected)
                    {
                        for (int i = 0; i < 10; ++i)
                            car_states[i].pose.pos.x += dx;
                    }
                    else
                    {
                        redraw_object(*selected_object);
                        selected_object->move(dx, 0.0f);
                        redraw_object(*selected_object);
                    }
                }
                if (manipulator_state == ManipulatorState::Y_Active)
                {
                    float dy = (event.pointer.new_y - event.pointer.old_y) / draw_context.scale;
                    if (car_selected)
                    {
                        for (int i = 0; i < 10; ++i)
                            car_states[i].pose.pos.y += dy;
                    }
                    else
                    {
                        redraw_object(*selected_object);
                        selected_object->move(0.0f, dy);
                        redraw_object(*selected_object);
                    }
                }
                if (manipulator_state == ManipulatorState::R_Active)
                {
                    Vec2 mouse_pos = Vec2(event.pointer.new_x, event.pointer.new_y);
                    Vec2 manip_pos = draw_context.world_to_screen(manipulator_pose.pos);
                    float angle = vector_to_angle(mouse_pos - manip_pos);
                    if (car_selected)
                    {
                        for (int i = 0; i < 10; ++i)
                            car_states[i].pose.heading = angle;
                    }
                    else if (selected_object)
                    {
                        redraw_object(*selected_object);
                        selected_object->pose.heading = angle;
                        redraw_object(*selected_object);
                    }
                }
                if (static_cast<int>(manipulator_state) % 2 == 1 || manipulator_state == ManipulatorState::Default) // Hover or Default
                {
                    Vec2 pos = draw_context.world_to_screen(manipulator_pose.pos);
                    float angle = manipulator_pose.heading;
                    Vec2 mouse_pos = Vec2(event.pointer.new_x, event.pointer.new_y);
                    manipulator_state = get_manipulator_state(pos, angle, mouse_pos, false);
                }
            }
            if (event.type == oc::EventType::Zoom)
            {
                draw_context.offset.x = (draw_context.offset.x - event.zoom.center_x) * event.zoom.factor + event.zoom.center_x;
                draw_context.offset.y = (draw_context.offset.y - event.zoom.center_y) * event.zoom.factor + event.zoom.center_y;
                draw_context.scale *= event.zoom.factor;
                schedule_redraw(draw_context.get_visible_world_rect());
            }
            if (event.type == oc::EventType::Scroll)
            {
                draw_context.offset.x += event.scroll.x;
                draw_context.offset.y += event.scroll.y;
                schedule_redraw(draw_context.get_visible_world_rect());
            }
            if (event.type == oc::EventType::Resize)
            {
                draw_context.width  = (int)((float)event.resize.new_width  * event.resize.new_scaling);
                draw_context.height = (int)((float)event.resize.new_height * event.resize.new_scaling);
                overview_base = (float *)realloc(overview_base, (size_t)(draw_context.width * draw_context.height) * 4 * sizeof(float));
                schedule_redraw(draw_context.get_visible_world_rect());
            }
        }

        if (40 < timing_event_count())
        {
            TIMED_BLOCK("Send timing data");
            ipc_packet.set_sender(ocMemberId::Virtual_Car);
            ipc_packet.set_message_id(ocMessageId::Timing_Events);
            if (write_timing_events_to_buffer(ipc_packet.get_payload()))
            {
                socket->send_packet(ipc_packet);
            }
        }
    } // End while
    return 0;
}
