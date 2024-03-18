#include "../common/ocAlarm.h"
#include "../common/ocAssert.h"
#include "../common/ocMember.h"
#include "../common/ocTime.h"
#include "../common/ocTypes.h"
#include "ocBroadcastServer.h"
#include "ocDebuggerServer.h"

#include <arpa/inet.h> // inet_ntoa()
#include <cstring> // strerror()
#include <csignal> // sigignore(SIGPIPE)
#include <ifaddrs.h> // ifaddrs, getifaddrs, freeifaddrs
#include <net/if.h> // IFF_LOOPBACK, IFF_BROADCAST
#include <netinet/in.h> // sockaddr_storage

static bool get_lan_addr(sockaddr_storage *lan_addr)
{
    // TODO: we could have multiple LAN IPs and should launch broadcast and debugger servers for all of them
    ifaddrs *addrs;
    if (getifaddrs(&addrs) < 0) return false;

    for (ifaddrs *tmp = addrs; tmp; tmp = tmp->ifa_next)
    {
        if (!tmp->ifa_addr) continue;
        if (tmp->ifa_addr->sa_family != AF_INET) continue; // only allows ipv4 for now
        if (tmp->ifa_flags & IFF_LOOPBACK) continue; // don't use the loopback interface
        if (!(tmp->ifa_flags & IFF_UP)) continue; // make sure the interface is enabled

        switch (tmp->ifa_addr->sa_family) {
            case AF_INET: {
                memcpy(lan_addr, tmp->ifa_addr, sizeof(sockaddr_in));
            } break;
            case AF_INET6: {
                memcpy(lan_addr, tmp->ifa_addr, sizeof(sockaddr_in6));
            } break;
        }
        return true;
    }
    freeifaddrs(addrs);
    return false;
}

int main()
{
    ocMember member(ocMemberId::Eth_Gateway, "Ethernet Gateway");
    member.attach();

    ocIpcSocket *ipc_socket = member.get_socket();
    ocSharedMemory *shared_memory = member.get_shared_memory();
    ocLogger *logger = member.get_logger();

    sockaddr_storage lan_addr = {};
    if (!get_lan_addr(&lan_addr))
    {
      logger->error("main: could not find LAN ip. Exiting...");
      return -1;
    }

    switch (lan_addr.ss_family) {
        case AF_INET: {
            char str[INET_ADDRSTRLEN];
            sockaddr_in *lip4 = (sockaddr_in *)&lan_addr;
            inet_ntop(AF_INET, &(lip4->sin_addr), str, INET_ADDRSTRLEN);
            logger->log("Using LAN address: %s TCP port: %i UDP port: %i", str, OC_NET_TCP_ADAPTER_PORT, OC_NET_UDP_ADAPTER_PORT);
        } break;
        case AF_INET6: {
            char str[INET6_ADDRSTRLEN];
            sockaddr_in6 *lip6 = (sockaddr_in6 *)&lan_addr;
            inet_ntop(AF_INET6, &(lip6->sin6_addr), str, INET6_ADDRSTRLEN);
            logger->log("Using LAN address: %s TCP port: %i UDP port: %i", str, OC_NET_TCP_ADAPTER_PORT, OC_NET_UDP_ADAPTER_PORT);
        } break;
        default:
            return -1;
    }

    ocPacket ipc_packet(ocMessageId::None);

    ocDebuggerServer  _dbs((sockaddr *)&lan_addr, ipc_socket);
    ocBroadcastServer _bcs((sockaddr *)&lan_addr);

    signal(SIGPIPE, SIG_IGN); // Do not kill yourself if send fails

    // create, fill out and send subscription packet.
    ipc_packet.set_message_id(ocMessageId::Subscribe_To_Messages);
    ipc_packet.clear_and_edit()
        .write(ocMessageId::Can_Frame_Transmitted)
        .write(ocMessageId::Can_Frame_Received)
        .write(ocMessageId::Ai_Switched_State)
        .write(ocMessageId::Birdseye_Image_Available)
        .write(ocMessageId::Camera_Image_Available)
        .write(ocMessageId::Member_List)
        .write(ocMessageId::Request_Timing_Sites)
        .write(ocMessageId::Timing_Sites)
        .write(ocMessageId::Timing_Events)
        .write(ocMessageId::Shapes)
        .write(ocMessageId::Imu_Rotation_Quaternion)
        .write(ocMessageId::Object_Found);
    ipc_socket->send_packet(ipc_packet);

    //start tcp server
    if (!_dbs.init_server()) return -1;

    //start udp server
    if (!_bcs.init_server()) return -1;

    ocAlarm timing_sites_timer(ocTime::seconds(3), ocAlarmType::Periodic);

    ocPollEngine _pe(4);
    _pe.add_fd(ipc_socket->get_fd());
    _pe.add_fd(_dbs.pe.get_fd());
    _pe.add_fd(_bcs.fd_listen);
    _pe.add_fd(timing_sites_timer.get_fd());

    while (true)
    {
        _pe.await();
        TIMED_BLOCK("work");

        if (_pe.was_triggered(ipc_socket->get_fd()))
        {
            TIMED_BLOCK("process ipc packets");
            int32_t status;
            // read non-blocking, so return 0 if no data is available
            while (0 < (status = ipc_socket->read_packet(ipc_packet, false)))
            {
                TIMED_BLOCK("packet");
                _dbs.log_ipc(&ipc_packet); // TODO: the gateway needs to register for all message_ids

                auto reader = ipc_packet.read_from_start();

                switch (ipc_packet.get_message_id())
                {
                    case ocMessageId::Camera_Image_Available:
                    {
                        const ocCamData *cam_data = &shared_memory->cam_data[shared_memory->last_written_cam_data_index];
                        uint32_t current_number = cam_data->frame_number;
                        _dbs.log_image(
                            ocImageType::Cam,
                            cam_data->img_buffer,
                            cam_data->width,
                            cam_data->height,
                            cam_data->pixel_format,
                            cam_data->frame_number);
                        oc_assert(current_number == cam_data->frame_number);
                    } break;
                    case ocMessageId::Can_Frame_Transmitted:
                    {
                        ocCanFrame can_frame = reader.read<ocCanFrame>();
                        // TODO send the direction to the debugger
                        _dbs.log_can_frame(can_frame);
                    } break;
                    case ocMessageId::Can_Frame_Received:
                    {
                        ocCanFrame can_frame = reader.read<ocCanFrame>();
                        // TODO send the direction to the debugger
                        _dbs.log_can_frame(can_frame);
                    } break;
                    case ocMessageId::Ai_Switched_State:
                    {
                        ocAiStateChange change;
                        change.state_name_len = (uint8_t) reader.peek<int32_t>();
                        reader.read_string(&change.state_name[0], 64);
                        change.state_change_param_len =  reader.read<uint8_t>();
                        oc_assert(change.state_change_param_len <= 32, change.state_change_param_len);
                        for (int i = 0; i < change.state_change_param_len; ++i)
                        {
                            change.state_change_params[i] = reader.read<char>();
                        }
                        change.is_main = reader.read<bool>();
                        _dbs.log_state_change(change);
                    } break;
                    case ocMessageId::Birdseye_Image_Available:
                    {
                        const ocBevData *bev_data = &shared_memory->bev_data[shared_memory->last_written_bev_data_index];
                        _dbs.log_image(
                            ocImageType::Bev,
                            bev_data->img_buffer,
                            (uint32_t)(bev_data->max_map_x - bev_data->min_map_x),
                            (uint32_t)(bev_data->max_map_y - bev_data->min_map_y),
                            ocPixelFormat::Gray_U8,
                            bev_data->frame_number);
                    } break;
                    case ocMessageId::Member_List:
                    {
                        _dbs.log_processes(shared_memory->online_members);
                    } break;
                    case ocMessageId::Request_Timing_Sites:
                    {
                        ipc_packet.set_message_id(ocMessageId::Timing_Sites);
                        if (write_timing_sites_to_buffer(ipc_packet.get_payload()))
                        {
                          ipc_socket->send_packet(ipc_packet);
                        }
                    } break;
                    case ocMessageId::Timing_Sites:
                    {
                        ocDbgTimingSite site;
                        while (reader.can_read())
                        {
                            site.index      = reader.read<uint16_t>();
                            site.linenumber = reader.read<uint16_t>();
                            site.process_id = (uint16_t)ipc_packet.get_sender();
                            // these mallocs are never freed which is ok because
                            // there is only a limited number of timing sites
                            // and we need to store them indefinitely so they
                            // can be used whenever a client asks for them.
                            char *userstring   = (char *)malloc(sizeof(char) * 128);
                            char *filename     = (char *)malloc(sizeof(char) * 128);
                            char *functionname = (char *)malloc(sizeof(char) * 128);
                            reader.read_string(userstring, 128);
                            reader.read_string(filename, 128);
                            reader.read_string(functionname, 128);
                            site.userstring   = (const char *)userstring;
                            site.filename     = (const char *)filename;
                            site.functionname = (const char *)functionname;
                            site.characters   = (uint16_t)(strlen(site.userstring) + strlen(site.filename) + strlen(site.functionname));
                            _dbs.log_timing_site(&site);
                        }
                    } break;
                    case ocMessageId::Timing_Events:
                    {
                        ocTimingEvent event;
                        while (reader.can_read<ocTimingEvent>())
                        {
                            event = reader.read<ocTimingEvent>();
                            event.stuff = (uint8_t)ipc_packet.get_sender();
                            _dbs.log_timing_event(&event);
                        }
                    } break;
                    case ocMessageId::Shapes:
                    {
                        uint32_t frame_number = reader.read<uint32_t>();
                        ocImageType image_kind = reader.read<ocImageType>();
                        while (reader.can_read<ocShapeType>())
                        {
                            ocShapeType type = reader.read<ocShapeType>();
                            ocShape shape;
                            shape.type = type;
                            switch (type)
                            {
                            case ocShapeType::Point:     shape.point = reader.read<ocPoint>(); break;
                            case ocShapeType::Circle:    shape.circle = reader.read<ocCircle>(); break;
                            case ocShapeType::Line:      shape.line = reader.read<ocLine>(); break;
                            case ocShapeType::Rectangle: shape.rectangle = reader.read<ocRectangle>(); break;
                            default:
                            {
                                logger->warn("Unhandled shape type: %i", type);
                            } break;
                            }
                            _dbs.log_shapes(image_kind, ipc_packet.get_sender(), frame_number, shape);
                        }
                    } break;
                    case ocMessageId::Object_Found:
                    {
                        ocDetectedObject detected_obj = reader.read<ocDetectedObject>();
                        _dbs.log_detected_object(detected_obj.object_type);
                    } break;
                    case ocMessageId::Imu_Rotation_Quaternion:
                    {
                        _dbs.log_imu_quat(
                            reader.read<float>(),
                            reader.read<float>(),
                            reader.read<float>(),
                            reader.read<float>());
                    } break;
                    default:
                    {
                        ocMessageId msg_id = ipc_packet.get_message_id();
                        ocMemberId  mbr_id = ipc_packet.get_sender();
                        logger->warn("Unhandled message_id: %s (0x%x) from sender: %s (%i)", to_string(msg_id), msg_id, to_string(mbr_id), mbr_id);
                    } break;
                }
            }
            if (status < 0)
            {
                logger->error("Error while reading the IPC socket: (%i) %s", errno, strerror(errno));
                return -1;
            }
        }
        NEXT_TIMED_BLOCK("run servers");

        _dbs.process_server();
        _bcs.process_server();

        NEXT_TIMED_BLOCK("handle timing data");

        // once every 3 seconds send out a request for timing sites
        if (timing_sites_timer.is_expired())
        {
            ipc_packet.set_message_id(ocMessageId::Request_Timing_Sites);
            ipc_packet.set_sender(ocMemberId::Eth_Gateway);
            ipc_packet.clear();
            ipc_socket->send_packet(ipc_packet);
        }

        if (40 < timing_event_count())
        {
          TIMED_BLOCK("Send timing data");
          ipc_packet.set_message_id(ocMessageId::Timing_Events);
          ipc_packet.set_sender(ocMemberId::Eth_Gateway);
          if (write_timing_events_to_buffer(ipc_packet.get_payload()))
          {
              ipc_socket->send_packet(ipc_packet);
          }
        }
    }
}
