#pragma once

#include "../common/ocCanFrame.h"
#include "../common/ocLogger.h"
#include "../common/ocPacket.h"
#include "../common/ocTypes.h"

#define OC_CAN_MAN_IF_NAME "can0"

class ocCanGateway
{
public:
    ocCanGateway(ocLogger *logger);

    bool init();

    bool read_frame(ocCanFrame *frame);
    bool send_frame(const ocCanFrame *frame);

    int can_to_ipc(/*const*/ ocCanFrame *frame, ocPacket *packet);
    int ipc_to_can(const ocPacket *packet, ocCanFrame *frame);

    int32_t get_socket() const { return _socket_fd; }
private:
    int32_t _socket_fd;
    ocLogger *_logger;

    bool _rc_active = false;
    int32_t _steps_times_4 = 0;
};
