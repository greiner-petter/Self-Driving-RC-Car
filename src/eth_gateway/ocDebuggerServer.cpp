#include "ocDebuggerServer.h"

#include "../common/ocCanFrame.h"
#include "../common/ocProfiler.h"
#include "../common/ocQoiFormat.h"

#include <algorithm> // remove()
#include <cstring> // strerror()
#include <netinet/in.h> // sockaddr_in
#include <netinet/tcp.h> // TCP_NODELAY
#include <sys/socket.h>
#include <unistd.h> // close()

void ocDebuggerServer::_on_recv(ocClient *client, const ocPacketNet *packet)
{
    TIMED_BLOCK("handle net packet");

    auto reader = packet->read_from_start();

    switch (packet->opcode)
    {
        case ocTcpId::Auth_Response:
        {
            uint16_t proto_version = reader.read<uint16_t>();
            client->protocol_version = proto_version;

            ocPacketNet *p = new ocPacketNet();
            p->opcode = ocTcpId::Auth_Status;
            p->clear_and_edit().write<uint8_t>(OC_NET_TCP_AUTH_STATUS_OK);
            p->clients = client->id; // only this one client should receive this packet
            _outgoing_packets.push(p);

            client->state = ocClientState::Authed;
            break;
        }
        case ocTcpId::Params:
        {
            ocTcpParamId param_type = reader.read<ocTcpParamId>();
            switch(param_type)
            {
                case ocTcpParamId::Camera_Quality:
                {
                    _jpg_params[1] = reader.read<uint8_t>();
                } break;
                case ocTcpParamId::Image_Mode:
                {
                    switch (reader.read<uint32_t>())
                    {
                    case 1:
                        client->subscribed_data |=  (uint32_t)ocDebugDataKind::Cam;
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Bev;
                        break;
                    case 2:
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Cam;
                        client->subscribed_data |=  (uint32_t)ocDebugDataKind::Bev;
                        break;
                    case 3:
                        client->subscribed_data |=  (uint32_t)ocDebugDataKind::Cam;
                        client->subscribed_data |=  (uint32_t)ocDebugDataKind::Bev;
                        break;
                    default:
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Cam;
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Bev;
                        break;
                    }
                } break;
                case ocTcpParamId::Image_Params:
                {
                    /* TODO: implement better camera configuration via debugger
                    uint32_t param_id = reader.read<uint32_t>();
                    double param1 = reader.read<double>();
                    double param2 = reader.read<double>();
                    */
                } break;
                case ocTcpParamId::Timing_Enabled:
                {
                    if (reader.read<uint8_t>())
                        client->subscribed_data |= (uint32_t)ocDebugDataKind::Timing_Data;
                    else
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Timing_Data;
                } break;
                case ocTcpParamId::Image_Enabled:
                {
                    if (reader.read<uint8_t>())
                        client->subscribed_data |= (uint32_t)ocDebugDataKind::Images;
                    else
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Images;
                } break;
                case ocTcpParamId::Ipc_Enabled:
                {
                    if (reader.read<uint8_t>())
                        client->subscribed_data |= (uint32_t)ocDebugDataKind::Ipc_Header;
                    else
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Ipc_Header;
                } break;
                case ocTcpParamId::Shapes_Enabled:
                {
                    if (reader.read<uint8_t>())
                        client->subscribed_data |= (uint32_t)ocDebugDataKind::Shapes;
                    else
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Shapes;
                } break;
                case ocTcpParamId::Imu_Quat_Enabled:
                {
                    if (reader.read<uint8_t>())
                        client->subscribed_data |= (uint32_t)ocDebugDataKind::Imu_Quat;
                    else
                        client->subscribed_data &= ~(uint32_t)ocDebugDataKind::Imu_Quat;
                } break;
                default:
                {
                    _logger.warn("received unknown parameter type %i", param_type);
                } break;
            }
            break;
        }
        case ocTcpId::Adapter:
        {
            ocTcpDataId data_type = reader.read<ocTcpDataId>();
            switch (data_type)
            {
                case ocTcpDataId::Can_Frame:
                {
                    uint32_t cf_id = reader.read<uint32_t>();
                    uint8_t cf_pl_len = reader.read<uint8_t>();
                    if (cf_pl_len <= 8)
                    {
                        ocCanFrame frame = {};
                        frame.id = (ocCanId)cf_id;
                        frame.write(reader.read(cf_pl_len), cf_pl_len);
                        _socket->send(ocMessageId::Send_Can_Frame, frame);
                        _logger.log("on_recv() : Sent can frame with id 0x%x", cf_id);
                    }
                    else
                    {
                        _logger.error("on_recv() : Received CAN frame with wrong length: %i", cf_pl_len);
                    }
                } break;
                case ocTcpDataId::Ipc_Packet:
                {
                    ocMessageId message_id = reader.read<ocMessageId>();
                    size_t length = packet->payload.get_length() - 2;
                    _socket->send(ocMemberId::Eth_Gateway, message_id, reader.read(length), length, true);
                    _logger.log("on_recv() : Sent IPC packet with message_id 0x%x", message_id);
                } break;
                case ocTcpDataId::Timing_Sites:
                {
                    uint32_t index = 0;
                    while (index < _timing_sites.size())
                    {
                        ocPacketNet *p = new ocPacketNet();
                        p->opcode = ocTcpId::Data;
                        auto editor = p->clear_and_edit();
                        editor
                            .write<ocTcpDataId>(ocTcpDataId::Timing_Sites)
                            .write<uint32_t>((uint32_t)_timing_sites.size());
                        p->clients = client->id; // only this one client should receive this packet
                        while (index < _timing_sites.size())
                        {
                            auto site = _timing_sites[index];
                            if (!editor.can_write(_timing_sites[index].characters
                                                + sizeof(uint32_t) * 3
                                                + sizeof(uint16_t) * 3))
                            {
                                break;
                            }
                            editor.write<uint16_t>(site.process_id);
                            editor.write<uint16_t>(site.index);
                            editor.write<uint16_t>(site.linenumber);
                            editor.write_string(site.userstring);
                            editor.write_string(site.filename);
                            editor.write_string(site.functionname);
                            index++;
                        }
                        _outgoing_packets.push(p);
                    }
                } break;
                default:
                {
                    _logger.warn("on_recv() : unknown data_type: %i", data_type);
                } break;
            }
        } break;
        case ocTcpId::Keepalive:
        {
            _logger.log("on_recv() : Keep-alive received from client");
        } break;
        default:
        {
            _logger.warn("on_recv() : Unknown opcode: 0x%x length: %i:", packet->opcode, packet->payload.get_length());
        } break;
    }

    if (reader.can_read())
    {
        _logger.warn("Not all bytes of received packet were read!");
    }
}

ocDebuggerServer::ocDebuggerServer(
    const sockaddr *lan_addr,
    ocIpcSocket *socket) :
    _send_buffer(OC_NET_PACKET_SIZE_MAX),
    _send_reader(&_send_buffer),
    _logger("Debugger Server"),
    pe(33)
{
    memset(&_endpoint, 0, sizeof(sockaddr_storage));
    switch (lan_addr->sa_family) {
        case AF_INET: {
            memcpy(&_endpoint, lan_addr, sizeof(sockaddr_in));
            sockaddr_in *lip4 = (sockaddr_in *)&_endpoint;
            lip4->sin_port = htons(OC_NET_TCP_ADAPTER_PORT);
            lip4->sin_addr.s_addr = INADDR_ANY;
        } break;
        case AF_INET6: {
            memcpy(&_endpoint, lan_addr, sizeof(sockaddr_in6));
            sockaddr_in6 *lip6 = (sockaddr_in6 *)&_endpoint;
            lip6->sin6_port = htons(OC_NET_TCP_ADAPTER_PORT);
            lip6->sin6_addr = in6addr_any;
        } break;
        default:
            _logger.error("Unsupported address type: %i", lan_addr->sa_family);
    }

    _socket = socket;

    _send_buffer.set_length(OC_NET_PACKET_SIZE_MAX);
}

uint32_t ocDebuggerServer::_interested_clients_mask(uint32_t data_kind)
{
    uint32_t mask = 0;
    for (auto *client : _clients)
    {
        if (data_kind == (client->subscribed_data & data_kind))
        {
            mask |= client->id;
        }
    }
    return mask;
}

void ocDebuggerServer::log_ipc(const ocPacket *packet)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Ipc_Header);
    if (clients)
    {
        if (!_ipc_packet ||
            _last_ipc_packet_id <= _last_sent_packet_id)
        {
            _ipc_packet = new ocPacketNet(ocTcpId::Data);
            _last_ipc_packet_id = _ipc_packet->id;
            _ipc_packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Ipc_Packet)
                .write<uint32_t>(0); // size field, overwritten later
            _outgoing_packets.push(_ipc_packet);
        }
        _ipc_packet->clients = clients;
        auto editor = _ipc_packet->edit_from_end();
        editor.write<ocMessageId>(packet->get_message_id());
        if ((false) /*_send_ipc_payload*/)
        {
            uint32_t length = (uint32_t)packet->get_payload()->get_length();
            auto payload = packet->get_payload()->get_space(length);
            editor.write<uint32_t>(length);
            editor.write(payload, length);
        }
        else
        {
            editor.write<int32_t>(0);
        }
        (*(uint32_t*)(&_ipc_packet->payload[1]))++; // increase the 4 size bytes
    }
}

void ocDebuggerServer::log_image(ocImageType image_type, const uint8_t *image_data, uint32_t width, uint32_t height, ocPixelFormat pixel_format, uint32_t frame_number)
{
    uint32_t clients = 0;
    switch (image_type)
    {
    case ocImageType::Cam:
        clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Images | (uint32_t)ocDebugDataKind::Cam);
        break;
    case ocImageType::Bev:
        clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Images | (uint32_t)ocDebugDataKind::Bev);
        break;
    default:
        _logger.warn("Unhandled image type: %i", image_type);
        return;
    }

    // We only want to create packets for the image if there are clients listening for them and there
    // are no pending packets for previous images.
    if (clients &&
        ((ocImageType::Cam == image_type && _last_cam_image_packet_id <= _last_sent_packet_id) ||
         (ocImageType::Bev == image_type && _last_bev_image_packet_id <= _last_sent_packet_id)))
    {
        _camera_frame_number = frame_number;

        // encode the image as jpeg to reduce its size
        std::vector<uint8_t> compressed_data = compress_image(width, height, pixel_format, image_data);

        // split the image data into as many packets as needed and add them to the client send list
        uint32_t read_pos = 0;
        while (read_pos < compressed_data.size())
        {
            ocPacketNet *p = new ocPacketNet(ocTcpId::Data);
            p->clients = clients;

            if (ocImageType::Cam == image_type)
                _last_cam_image_packet_id = p->id;
            else
                _last_bev_image_packet_id = p->id;

            auto editor = p->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Camera)
                .write<uint32_t>(frame_number); // index of camera frame

            if (read_pos == 0)
            {
                //first time sending chunks of this frame
                editor.write<uint8_t>(1); // yeah, we have some frame infos.
                editor.write<uint8_t>(8); // color depth in bit
                editor.write<uint8_t>(uint8_t(image_type)); // image type
                // due to legacy reasons the width and height fields together need to give the buffer size
                editor.write<uint32_t>((uint32_t)compressed_data.size()); // used to be width
                editor.write<uint32_t>(1);               // used to be height
                editor.write<uint32_t>((uint32_t)compressed_data.size());
            }
            else if (compressed_data.size() <= read_pos + OC_NET_CAMERA_MAX_CHUNK_SIZE)
            {
                editor.write<uint8_t>(2); // This is the last data frame for this image.
            }
            else
            {
                editor.write<uint8_t>(0); // no frame infos.
            }

            editor.write<uint32_t>(read_pos); // starting position of buffer sent in this frame
            uint32_t img_chunk_size = (uint32_t)compressed_data.size() - read_pos;
            if (OC_NET_CAMERA_MAX_CHUNK_SIZE < img_chunk_size) img_chunk_size = OC_NET_CAMERA_MAX_CHUNK_SIZE;
            editor.write<uint32_t>(img_chunk_size); // size of chunk being sent here
            editor.write(&compressed_data[read_pos], img_chunk_size);
            read_pos += img_chunk_size;
            _outgoing_packets.push(p);
        }
    }
}

void ocDebuggerServer::log_kv(int32_t key, int32_t value)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Kv_Data);
    if (clients)
    {
        if (!_key_value_packet ||
            _last_key_value_packet_id <= _last_sent_packet_id ||
            OC_NET_PACKET_PAYLOAD_CUTOFF <= _key_value_packet->payload.get_length())
        {
            _key_value_packet = new ocPacketNet(ocTcpId::Data);
            _last_key_value_packet_id = _key_value_packet->id;
            _key_value_packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Key_Value)
                .write<uint32_t>(0); // size field, overwritten later
            _outgoing_packets.push(_key_value_packet);
        }
        _key_value_packet->clients = clients;
        _key_value_packet->edit_from_end().write<int32_t>(key).write<int32_t>(value);
        (*(uint32_t*)(&_key_value_packet->payload[1]))++; // increase the 4 size bytes
    }
}

void ocDebuggerServer::log_timing_site(const ocDbgTimingSite *site)
{
    _timing_sites.push_back(*site);
}

void ocDebuggerServer::log_timing_event(const ocTimingEvent *event)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Timing_Data);
    if (clients)
    {
        if (!_timing_packet ||
            _last_timing_packet_id <= _last_sent_packet_id ||
            OC_NET_PACKET_PAYLOAD_CUTOFF <= _timing_packet->payload.get_length())
        {
            _timing_packet = new ocPacketNet(ocTcpId::Data);
            _last_timing_packet_id = _timing_packet->id;
            _timing_packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Timing_Events);
            _outgoing_packets.push(_timing_packet);
        }
        _timing_packet->clients = clients;
        _timing_packet->edit_from_end().write<ocTimingEvent>(*event);
    }
}

void ocDebuggerServer::log_state_change(ocAiStateChange state_change)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Ai_States);
    if (clients)
    {
        if (!_states_packet ||
            _last_states_packet_id <= _last_sent_packet_id ||
            OC_NET_PACKET_PAYLOAD_CUTOFF <= _states_packet->payload.get_length())
        {
            _states_packet = new ocPacketNet(ocTcpId::Data);
            _last_states_packet_id = _states_packet->id;
            _states_packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Bv_State_Change)
                .write<uint32_t>(0); // size field, overwritten later
            _outgoing_packets.push(_states_packet);
        }
        _states_packet->clients = clients;
        auto editor = _states_packet->edit_from_end();
        // write the new state name length and characters
        editor.write<uint8_t>(state_change.state_name_len);
        for (size_t i = 0; i < state_change.state_name_len; ++i)
        {
            editor.write<char>(state_change.state_name[i]);
        }

        // write the number of parameters and parameter ids
        editor.write<uint8_t>(state_change.state_change_param_len);
        for (size_t i = 0; i < state_change.state_change_param_len; ++i)
        {
            editor.write<char>(state_change.state_change_params[i]);
        }

        // write boolean to indicate what state machine was changed
        editor.write<bool>(state_change.is_main);

        // increase the 4 size bytes
        (*(uint32_t*)(&_states_packet->payload[1]))++;
    }
}

void ocDebuggerServer::log_processes(uint16_t processes)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Processes);
    if (clients)
    {
        ocPacketNet *p = new ocPacketNet(ocTcpId::Member_Online_State);
        p->clients = clients;
        auto editor = p->clear_and_edit();
        editor.write<uint8_t>(0); // overwrite at the end
        uint8_t process_count = 0;
        for (size_t i = 0; i < 16; ++i)
        {
            uint16_t process_id = (uint16_t)(1 << i);
            if (processes & process_id)
            {
                editor.write<uint16_t>(process_id);
                process_count++;
            }
        }
        (*(uint8_t*)(&p->payload[0])) = process_count;
        _outgoing_packets.push(p);
    }
}

void ocDebuggerServer::log_can_frame(ocCanFrame can_frame)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Can_Frames);
    if (clients)
    {
        if (!_can_packet ||
            _last_can_packet_id <= _last_sent_packet_id ||
            OC_NET_PACKET_PAYLOAD_CUTOFF <= _can_packet->payload.get_length())
        {
            _can_packet = new ocPacketNet(ocTcpId::Data);
            _last_can_packet_id = _can_packet->id;
            _can_packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Can_Frame)
                .write<uint32_t>(0); // size field, overwritten later
            _outgoing_packets.push(_can_packet);
        }
        _can_packet->clients = clients;
        _can_packet->edit_from_end()
            .write<int32_t>((int32_t)can_frame.timestamp.get_nanoseconds())
            .write<ocCanId>(can_frame.id)
            .write<uint8_t>((uint8_t)can_frame.length)
            .write(&can_frame.data[0], can_frame.length);

        // increase the 4 size bytes
        (*(uint32_t*)(&_can_packet->payload[1]))++;
    }
}

void ocDebuggerServer::log_detected_object(ocObjectType object_type)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Objects);
    if (clients)
    {
        ocPacketNet *packet = new ocPacketNet(ocTcpId::Data);
        packet->clients = clients;
        packet->clear_and_edit()
            .write<ocTcpDataId>(ocTcpDataId::Object_Detected)
            .write<ocObjectType>(object_type);
        _outgoing_packets.push(packet);
    }
}

void ocDebuggerServer::log_shapes(ocImageType image_type, ocMemberId proc_id, uint32_t frame_number, ocShape shape)
{
    uint32_t clients = 0;
    switch (image_type)
    {
    case ocImageType::Cam:
        clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Cam | (uint32_t)ocDebugDataKind::Shapes);
        break;
    case ocImageType::Bev:
        clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Bev | (uint32_t)ocDebugDataKind::Shapes);
        break;
    default:
        _logger.warn("Unhandled image type: %i", image_type);
        return;
    }
    if (clients && _camera_frame_number == frame_number)
    {
        ocPacketNet *packet;
        if (ocImageType::Cam == image_type)
        {
            if (!_cam_shapes_packet ||
                _last_cam_shapes_packet_id <= _last_sent_packet_id ||
                _last_cam_shapes_packet_proc_id != proc_id ||
                OC_NET_PACKET_PAYLOAD_CUTOFF <= _cam_shapes_packet->payload.get_length())
            {
                _cam_shapes_packet = new ocPacketNet(ocTcpId::Data);
                _last_cam_shapes_packet_id = _cam_shapes_packet->id;
                _last_cam_shapes_packet_proc_id = proc_id;
            }
            packet = _cam_shapes_packet;
        }
        else
        {
            if (!_bev_shapes_packet ||
                _last_bev_shapes_packet_id <= _last_sent_packet_id ||
                _last_bev_shapes_packet_proc_id != proc_id ||
                OC_NET_PACKET_PAYLOAD_CUTOFF <= _bev_shapes_packet->payload.get_length())
            {
                _bev_shapes_packet = new ocPacketNet(ocTcpId::Data);
                _last_bev_shapes_packet_id = _bev_shapes_packet->id;
                _last_bev_shapes_packet_proc_id = proc_id;
            }
            packet = _bev_shapes_packet;
        }

        if (0 == packet->payload.get_length())
        {
            packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Shapes)
                .write<uint32_t>(frame_number)
                .write<ocImageType>(image_type)
                .write<ocMemberId>(proc_id);
            _outgoing_packets.push(packet);
        }
        packet->clients = clients;
        auto editor = packet->edit_from_end();
        editor.write<ocShapeType>(shape.type);
        switch (shape.type)
        {
        case ocShapeType::Point:     editor.write<ocPoint>(shape.point); break;
        case ocShapeType::Circle:    editor.write<ocCircle>(shape.circle); break;
        case ocShapeType::Line:      editor.write<ocLine>(shape.line); break;
        case ocShapeType::Rectangle: editor.write<ocRectangle>(shape.rectangle); break;
        default:
        {
            _logger.warn("Unhandled shape type: %i", shape.type);
        } break;
        }
    }
}

void ocDebuggerServer::log_imu_quat(float w, float i, float j, float k)
{
    uint32_t clients = _interested_clients_mask((uint32_t)ocDebugDataKind::Imu_Quat);
    if (clients && _last_imu_quat_packet_id <= _last_sent_packet_id)
    {
        if (!_imu_quat_packet ||
            _last_imu_quat_packet_id <= _last_sent_packet_id ||
            OC_NET_PACKET_PAYLOAD_CUTOFF <= _imu_quat_packet->payload.get_length())
        {
            _imu_quat_packet = new ocPacketNet(ocTcpId::Data);
            _last_imu_quat_packet_id = _imu_quat_packet->id;
            _imu_quat_packet->clear_and_edit()
                .write<ocTcpDataId>(ocTcpDataId::Imu_Quat);
            _outgoing_packets.push(_imu_quat_packet);
        }

        _imu_quat_packet->clients = clients;
        _imu_quat_packet->edit_from_end()
            .write<float>(w)
            .write<float>(i)
            .write<float>(j)
            .write<float>(k);
    }
}

bool ocDebuggerServer::init_server()
{
    _fd_listen = socket(_endpoint.ss_family, SOCK_STREAM, 0);
    if (_fd_listen < 0)
    {
        _logger.error("init_server() : Failed to create listen socket: (%i) %s", errno, strerror(errno));
        return false;
    }

    int setsockopt_1 = 1; // needed because setsockopt needs something to point to
    if (setsockopt(_fd_listen, SOL_SOCKET, SO_REUSEADDR, (char *)&setsockopt_1, sizeof(setsockopt_1)) < 0)
    {
        _logger.error("init_server() : setsockopt failed: (%i) %s", errno, strerror(errno));
        return false;
    }

    if (bind(_fd_listen, (struct sockaddr*) &_endpoint, sizeof(_endpoint)) < 0)
    {
        _logger.error("init_server() : bind() failed: (%i) %s", errno, strerror(errno));
        return false;
    }

    if (listen(_fd_listen, OC_GATEWAY_ACCEPT_QUEUE_SIZE) < 0)
    {
        _logger.error("init_server() : listen() failed: (%i) %s", errno, strerror(errno));
        return false;
    }

    pe.add_fd(_fd_listen);

    _logger.log("Debug server is listening.");

    return true;
}

void ocDebuggerServer::process_server()
{
    TIMED_BLOCK("debug server");
    pe.update(); // returns immediately but refreshes the event buffer so that pe.was_triggered(fd) is up to date.

    if (pe.was_triggered(_fd_listen))
    {
        connect_client();
    }

    bool socket_full = false;

    // first handle all sending. To be fair to all clients we let everyone only send one packet before moving to the next client.
    // This way the first doesn't fill up the whole send buffer and leaves no room for the others.
    // We need two nested loops here to make sure that we always loop the full list of clients to see in anyone wants to send a packet.
    NEXT_TIMED_BLOCK("send packets");
    while (!_outgoing_packets.empty() && !socket_full)
    {
        ocPacketNet *packet = _outgoing_packets.front();

        if (_refill_send_buffer)
        {
            _refill_send_buffer = false;
            _last_sent_packet_id = packet->id;
            auto reader = packet->read_from_start();
            auto writer = _send_buffer.clear_and_edit();
            uint32_t length = (uint32_t)packet->payload.get_length();
            writer.write<ocTcpId>(packet->opcode);
            writer.write<uint32_t>(length);
            if (0 < length) writer.write(reader.read(length), length);
            _send_reader = _send_buffer.read_from_start();
        }

        while (_receiving_client_index < _clients.size() && !socket_full)
        {
            ocClient *client = _clients[_receiving_client_index];
            if (0 == (client->id & packet->clients))
            {
                // the packet isn't for this client. skip it.
                _receiving_client_index += 1;
                continue;
            }

            size_t len_rest = _send_reader.available_read_space();
            auto mem_pos = _send_reader.peek(len_rest);
            ssize_t send_res = send(client->fd, mem_pos, len_rest, MSG_DONTWAIT);

            if (0 <= send_res)
            {
                _send_reader.inc_pos((size_t)send_res);

                // reset the buffer and continue to the next client to send it to them too.
                if (!_send_reader.can_read())
                {
                    _send_reader.set_pos(0);
                    _receiving_client_index += 1;
                }
            }
            else if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                socket_full = true;
            }
            else
            {
                disconnect_client(client);
            }
        }
        // if we exited the loop and the socket wasn't full, it means we have sent the payload to all clients and can begin with the next one.
        if (!socket_full)
        {
            _refill_send_buffer = true;
            _receiving_client_index = 0;
            _outgoing_packets.pop();
            delete packet;
        }
    }

    for (size_t j = 0; j < _clients.size(); ++j)
    {
        ocClient *sel_client = _clients[j];
        // check if the client received anything
        if (sel_client && pe.was_triggered(sel_client->fd))
        {
            TIMED_BLOCK("receive packet");
            ocFragmentationContext *fc = &(sel_client->fragmentation_context);

            if (fc->frag_state == ocFragmentationState::Header)
            {
                // TODO: don't kick the client if we don't receive a full header all at once. wait for more data.
                ssize_t received = recv(sel_client->fd, &_receive_buffer[0], OC_NET_PACKET_HEADER_SIZE, MSG_DONTWAIT);
                if (OC_NET_PACKET_HEADER_SIZE == received)
                {
                    //there are at least 5 bytes in recv buffer, we can construct a header from this!

                    fc->recv_packet.opcode = (ocTcpId)_receive_buffer[0]; // 1 byte opcode
                    memcpy(&fc->recv_payload_len, &_receive_buffer[1], sizeof(uint32_t)); // 4 byte len in container

                    if (OC_NET_PACKET_PAYLOAD_SIZE_MAX < fc->recv_payload_len)
                    {
                        disconnect_client(sel_client);
                        continue;
                    }

                    // if the message has no payload, we stay in the header state and wait for the next one.
                    if (0 == fc->recv_payload_len)
                    {
                        _on_recv(sel_client, &fc->recv_packet);
                    }
                    else
                    {
                        fc->frag_state = ocFragmentationState::Payload;
                    }

                    fc->recv_packet.payload.set_length(fc->recv_payload_len);
                    fc->writer = fc->recv_packet.clear_and_edit();
                }
                else if (0 == received || (errno != EWOULDBLOCK && errno != EAGAIN))
                {
                    disconnect_client(sel_client);
                    continue;
                }
            }

            if (fc->frag_state == ocFragmentationState::Payload)
            {
                size_t length_remaining = fc->recv_payload_len - fc->writer.get_pos();
                auto payload = fc->writer.get_writable_space(length_remaining);

                ssize_t received = recv(sel_client->fd, payload, length_remaining, MSG_DONTWAIT);
                if (0 < received)
                {
                    size_t still_remaining = length_remaining - (size_t)received;
                    if (0 == still_remaining)
                    {
                        //payload complete
                        fc->frag_state = ocFragmentationState::Header;

                        _on_recv(sel_client, &fc->recv_packet);

                        _logger.log("process_server() : Payload received, length %i", fc->recv_payload_len);
                    }
                    else
                    {
                        fc->writer.dec_pos(still_remaining);
                    }
                }
                else if (0 == received || (errno != EWOULDBLOCK && errno != EAGAIN))
                {
                    disconnect_client(sel_client);
                    continue;
                }
            }
        }
    }
}

ocClient *ocDebuggerServer::connect_client()
{
    if (32 <= _clients.size())
    {
        _logger.warn("Too many clients, can't connect another one.");
        return nullptr;
    }

    sockaddr_storage new_addr;
    int addr_len = sizeof(new_addr);
    int new_sock = accept(_fd_listen, (struct sockaddr*) &new_addr, (socklen_t*) &addr_len);

    if (new_sock < 0)
    {
        _logger.warn("connect_client() : Failed accepting new socket: (%i) %s", errno, strerror(errno));
        return nullptr;
    }

    int one = 1;
    setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    ocClient *new_client = new ocClient();
    new_client->fd    = new_sock;
    new_client->state = ocClientState::Connected;

    // find a free client id
    for (uint32_t bit = 0; bit < 32; ++bit)
    {
        new_client->id = 1 << bit;
        bool used = false;
        for (auto *client : _clients)
        {
            if (client->id == new_client->id)
            {
                used = true;
                break;
            }
        }
        if (!used) break;
    }

    _clients.push_back(new_client);
    pe.add_fd(new_sock);

    //Send authentication challenge
    ocPacketNet *p = new ocPacketNet();
    p->opcode  = ocTcpId::Auth_Challenge;
    p->clients = new_client->id;
    _outgoing_packets.push(p);

    _logger.log("New client at socket: %i", new_sock);

    return new_client;
}

void ocDebuggerServer::disconnect_client(ocClient *client)
{
    int fd = client->fd;
    pe.delete_fd(fd);
    //disconnect event!
    close(fd);
    //erase idiom
    _clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());

    _logger.log("Client disconnected: %i", fd);

    delete client;
}

std::vector<uint8_t> ocDebuggerServer::compress_image(uint32_t width, uint32_t height, ocPixelFormat pixel_format, const void *image_data)
{
    TIMED_BLOCK("Compress Image");
    std::vector<uint8_t> temp_buffer(width * height * 4);

    oc::qoi::EncodeResult result = oc::qoi::encode(
      (const std::byte *)image_data, width, height, pixel_format,
      (std::byte *)temp_buffer.data(), temp_buffer.size());

    if (result.status != oc::qoi::EncodeStatus::Success)
    {
        _logger.warn("Couldn't encode image:\nsize: %i, %i\nformat: %s (%i)\nstatus: %s (%i)\nbuffer size: %i",
            width, height,
            to_string(pixel_format), int(pixel_format),
            to_string(result.status), int(result.status),
            temp_buffer.size());
        temp_buffer.clear();
    }
    else
    {
        _logger.log("Encoded %i pixels in %i bytes.", width * height, result.output_length);
        temp_buffer.resize(result.output_length);
    }
    return temp_buffer;
}
