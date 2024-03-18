#pragma once

#include <cstdint>

//general switches
#define OC_DEBUG                                    1
#define OC_GATEWAY_SOFTWARE_VERSION                 1
#define OC_GATEWAY_ACCEPT_QUEUE_SIZE                5


//ports
#define OC_NET_UDP_ADAPTER_PORT                     31337
#define OC_NET_TCP_ADAPTER_PORT                     11000


#define OC_NET_PACKET_SIZE_MAX                      7500 // MTU with jumboframes is 9k
#define OC_NET_PACKET_HEADER_SIZE                   5
#define OC_NET_PACKET_PAYLOAD_SIZE_MAX (OC_NET_PACKET_SIZE_MAX - OC_NET_PACKET_HEADER_SIZE)
#define OC_NET_PACKET_PAYLOAD_CUTOFF (OC_NET_PACKET_PAYLOAD_SIZE_MAX - 40) // leave some extra space so we don't accidentally write over the max size

#define OC_NET_TCP_MAX_USERS                        4

//opcodes
enum class ocUdpId : uint8_t
{
    Beacon         = 0xB0,
    Beacon_Request = 0xB1,
    Ping           = 0xB2,
    Ping_Response  = 0xB3
};

#define OC_NET_UDP_BEACON_MAGIC                     0xBEAC
#define OC_NET_UDP_PROTOCOL_VERSION                 0x01

enum class ocTcpId : uint8_t
{
    Auth_Challenge      = 0xC0, //sent to client (pc)
    Auth_Response       = 0xC1, //sent to adapter
    Auth_Status         = 0xC2, //sent to client

    Data                = 0xC3, //sent to client
    Error               = 0xC4, //sent to client - C4, like, boom, amirite
    Keepalive           = 0xC5,
    Adapter             = 0xC6, //send to adapter
    Params              = 0xC7, //parameters for various things
    Member_Online_State = 0xC8  //which members are online?
};

//subtypes for  OC_NET_TCP_OPCODES_AUTH_STATUS
#define OC_NET_TCP_AUTH_STATUS_OK                   0x01 //sent to client
#define OC_NET_TCP_AUTH_STATUS_FAIL                 0x00 //sent to client


//subtypes for  OC_NET_TCP_OPCODES_DATA
enum class ocTcpDataId : uint8_t
{
    Can_Frame       = 0x01,
    Camera          = 0x02,
    Key_Value       = 0x03,
    Bv_Debug        = 0x04,
    Bv_State_Change = 0x05,
    Object_Detected = 0x06,
    Ipc_Packet      = 0x07,
    Shapes          = 0x08,
    Timing_Events   = 0x09,
    Timing_Sites    = 0x0A,
    Imu_Quat        = 0x0B
};

//subtypes for OC_NET_TCP_OPCODES_PARAMS
enum class ocTcpParamId : uint8_t
{
    Camera_Quality = 0x01,
    Image_Mode     = 0x02,
    Image_Params   = 0x03,
    Timing_Enabled = 0x04,
    Image_Enabled  = 0x05,
    Ipc_Enabled    = 0x06,
    Shapes_Enabled = 0x07,
    Imu_Quat_Enabled = 0x08
};

enum class ocTcpKvId : uint8_t
{
    Steering         = 1,
    Speed            = 2,
    Camera_Quality   = 3,
    Ai_Curve         = 4,
    Ai_Curve_X_Shift = 5,
    Button_Pressed   = 6,
    Img_Mode         = 7
};


//camera stuff
#define OC_NET_CAMERA_MAX_CHUNK_SIZE                6000 // size dividable by 3

#define OC_NET_TCP_PROTOCOL_VERSION                 0x01
