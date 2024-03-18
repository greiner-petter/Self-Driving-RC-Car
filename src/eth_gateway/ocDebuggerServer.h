#pragma once

#include "ocConstGateway.h"
#include "../common/ocBuffer.h"
#include "../common/ocBufferReader.h"
#include "../common/ocBufferWriter.h"
#include "../common/ocCanFrame.h"
#include "../common/ocConst.h"
#include "../common/ocLogger.h"
#include "../common/ocPacket.h"
#include "../common/ocPollEngine.h"
#include "../common/ocProfiler.h"
#include "../common/ocIpcSocket.h"
#include "../common/ocTypes.h"

#include <map>
#include <netinet/in.h>
#include <ostream>
#include <queue>
#include <vector>

#define DEFAULT_JPG_QUALITY 30

struct ocPacketNet
{
    uint32_t id;
    ocTcpId  opcode;
    uint32_t clients;
    ocBuffer payload;

    ocPacketNet() : payload(OC_NET_PACKET_PAYLOAD_SIZE_MAX)
    {
        static uint32_t counter = 1;
        id = counter;
        counter += 1;
    }

    ocPacketNet(ocTcpId p_opcode) : ocPacketNet()
    {
        opcode = p_opcode;
    }

    ocBufferWriter clear_and_edit()
    {
        return payload.clear_and_edit();
    }
    ocBufferWriter edit_from_end()
    {
        return payload.edit_from_end();
    }
    ocBufferReader read_from_start() const
    {
        return payload.read_from_start();
    }
};

enum class ocClientState
{
    Disconnected,
    Connected,
    Authed
};

enum class ocFragmentationState
{
    Header,
    Payload
};

struct ocFragmentationContext
{
    ocFragmentationState frag_state = ocFragmentationState::Header;
    uint32_t recv_payload_len;
    ocPacketNet recv_packet;
    ocBufferWriter writer = ocBufferWriter(&recv_packet.payload);
};

enum class ocDebugDataKind : uint32_t
{
    None        = 0x0000,
    Cam         = 0x0001,
    Bev         = 0x0002,
    Images      = 0x0004,
    Shapes      = 0x0008,
    Timing_Data = 0x0010,
    Ipc_Header  = 0x0020,
    //Ipc_Payload = 0x0040, // not implemented
    Can_Frames  = 0x0080,
    Kv_Data     = 0x0100,
    Ai_States   = 0x0200,
    Processes   = 0x0400,
    Objects     = 0x0800,
    Bv_Data     = 0x1000,
    Imu_Quat    = 0x2000
};

struct ocClient
{
    int32_t                fd;
    uint32_t               id;
    ocClientState          state = ocClientState::Disconnected;
    int32_t                protocol_version;
    ocFragmentationContext fragmentation_context;

    uint32_t subscribed_data = (uint32_t)ocDebugDataKind::Images
                             | (uint32_t)ocDebugDataKind::Can_Frames
                             | (uint32_t)ocDebugDataKind::Kv_Data
                             | (uint32_t)ocDebugDataKind::Ai_States
                             | (uint32_t)ocDebugDataKind::Processes
                             | (uint32_t)ocDebugDataKind::Objects
                             | (uint32_t)ocDebugDataKind::Bv_Data;
};

struct ocDbgTimingSite
{
    const char *userstring;
    const char *filename;
    const char *functionname;
    uint16_t linenumber;
    uint16_t index;
    uint16_t process_id;
    uint16_t characters;
};

class ocDebuggerServer
{
private:
    std::vector<ocClient*> _clients;

    int _fd_listen;
    sockaddr_storage _endpoint;

    uint8_t _receive_buffer[OC_NET_PACKET_SIZE_MAX];

    ocBuffer _send_buffer;
    ocBufferReader _send_reader;

    std::queue<ocPacketNet*> _outgoing_packets;
    bool _refill_send_buffer = true;
    size_t _receiving_client_index = 0;
    ocMemberId _last_cam_shapes_packet_proc_id = ocMemberId::None;
    ocMemberId _last_bev_shapes_packet_proc_id = ocMemberId::None;

    uint32_t _last_sent_packet_id       = 0;
    uint32_t _last_cam_image_packet_id  = 0;
    uint32_t _last_bev_image_packet_id  = 0;
    uint32_t _last_timing_packet_id     = 0;
    uint32_t _last_key_value_packet_id  = 0;
    uint32_t _last_states_packet_id     = 0;
    uint32_t _last_can_packet_id        = 0;
    uint32_t _last_ipc_packet_id        = 0;
    uint32_t _last_cam_shapes_packet_id = 0;
    uint32_t _last_bev_shapes_packet_id = 0;
    uint32_t _last_imu_quat_packet_id   = 0;

    ocPacketNet *_timing_packet     = nullptr;
    ocPacketNet *_key_value_packet  = nullptr;
    ocPacketNet *_states_packet     = nullptr;
    ocPacketNet *_can_packet        = nullptr;
    ocPacketNet *_ipc_packet        = nullptr;
    ocPacketNet *_cam_shapes_packet = nullptr;
    ocPacketNet *_bev_shapes_packet = nullptr;
    ocPacketNet *_imu_quat_packet   = nullptr;

    std::vector<int32_t> _jpg_params;

    std::vector<ocDbgTimingSite> _timing_sites;

    uint32_t _camera_frame_number = 0;

    ocIpcSocket *_socket = nullptr;
    ocLogger _logger;

    uint32_t _interested_clients_mask(uint32_t data_kind);

    void _on_recv(ocClient *client, const ocPacketNet *packet);
    std::vector<uint8_t> compress_image(uint32_t width, uint32_t height, ocPixelFormat pixel_format, const void *image_data);

public:
    ocPollEngine pe;

    bool init_server();
    void process_server();
    int client_count();

    ocClient *connect_client();
    void disconnect_client(ocClient *c);

    ocDebuggerServer(const sockaddr *lan_addr, ocIpcSocket *socket);

    void log_ipc(const ocPacket *packet);
    void log_kv(int key, int value);
    void log_timing_site(const ocDbgTimingSite *site);
    void log_timing_event(const ocTimingEvent *event);
    void log_image(ocImageType image_type, const uint8_t *image_data, uint32_t width, uint32_t height, ocPixelFormat pixel_format, uint32_t frame_number);
    void log_state_change(ocAiStateChange sci);
    void log_processes(uint16_t processes);
    void log_can_frame(ocCanFrame can_frame);
    void log_detected_object(ocObjectType object_type);
    void log_shapes(ocImageType image_type, ocMemberId proc_id, uint32_t frame_number, ocShape shape);
    void log_imu_quat(float w, float i, float j, float k);
};
