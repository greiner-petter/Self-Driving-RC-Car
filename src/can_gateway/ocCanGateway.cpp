#include "ocCanGateway.h"

#include "../common/ocCar.h"
#include "../common/ocTime.h"

#include <cstring> // memcpy
#include <sys/socket.h>
#include <linux/can.h>
#include <sys/ioctl.h>
#include <net/if.h> // ifreq
#include <unistd.h> // close
#include <fcntl.h>

ocCanGateway::ocCanGateway(ocLogger *logger)
{
    _logger = logger;
}

bool ocCanGateway::init()
{
    _socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (_socket_fd == -1)
    {
        _logger->error("Error creating CAN socket: (%i) %s", errno, strerror(errno));
        return false;
    }

    ifreq ifr;
    strcpy(ifr.ifr_name, OC_CAN_MAN_IF_NAME);

    if (-1 == ioctl(_socket_fd, SIOCGIFINDEX, &ifr))
    {
        _logger->error("Error at ioctl: (%i) %s", errno, strerror(errno));
        return false;
    }

    sockaddr_can saddr_can;
    saddr_can.can_family = AF_CAN;
    saddr_can.can_ifindex = ifr.ifr_ifindex;

    if (-1 == bind(_socket_fd, (struct sockaddr*) &saddr_can, sizeof(saddr_can)))
    {
        _logger->error("Error at bind: (%i) %s", errno, strerror(errno));
        return false;
    }

    if (-1 == fcntl(_socket_fd, F_SETFL, O_NONBLOCK))
    {
        _logger->error("Error at fcntl: (%i) %s", errno, strerror(errno));
        return false;
    }

    return true;
}

bool ocCanGateway::send_frame(const ocCanFrame *frame)
{
    can_frame frame_temp;
    frame_temp.can_id = (uint32_t)frame->id;
    frame_temp.can_dlc = (uint8_t)frame->length;
    memcpy(&frame_temp.data[0], &frame->data[0], frame_temp.can_dlc);

    ssize_t bytes_sent = write(_socket_fd, &frame_temp, sizeof(frame_temp));

    return 0 < bytes_sent && (size_t)bytes_sent == sizeof(frame_temp);
}

bool ocCanGateway::read_frame(ocCanFrame *frame)
{
    can_frame frame_temp = {};
    ssize_t bytes_read = read(_socket_fd, &frame_temp, sizeof(frame_temp));
    if (0 < bytes_read)
    {
        if (frame_temp.can_id & CAN_ERR_FLAG)
        {
            _logger->error("CAN Error: 0x%x", frame_temp.can_id);
            frame->id = ocCanId::Com_Error;
            return true;
        }
        frame->clear();
        frame->timestamp = ocTime::now();
        frame->id = (ocCanId)(frame_temp.can_id & CAN_SFF_MASK); // mask off only the relevant bits of the id
        frame->write(&frame_temp.data[0], frame_temp.can_dlc);
        frame->reset_pos();
        return true;
    }
    return false;
}

int ocCanGateway::can_to_ipc(/*const*/ ocCanFrame *frame, ocPacket *packet)
{
    switch (frame->id)
    {
        case ocCanId::Digital_Input:
        {
            ocCanInputId id = frame->read<ocCanInputId>();
            if (id == ocCanInputId::Button1)
            { //free drive
                packet->set_message_id(ocMessageId::Received_Button_Press);
                packet->clear_and_edit().write<int32_t>(1);
            }
            else if (id == ocCanInputId::Button2)
            { // obstacle drive
                packet->set_message_id(ocMessageId::Received_Button_Press);
                packet->clear_and_edit().write<int32_t>(2);
            } else if (id == ocCanInputId::Aux)
            {
                uint8_t value = frame->read<uint8_t>();
                _rc_active = (0 != value);
                packet->set_message_id(ocMessageId::Rc_State_Changed);
                packet->clear_and_edit().write<uint8_t>(value);
            }
        } break;
        case ocCanId::Odo_Front:
        {
            int16_t steps_left  = frame->read<int16_t>();
            int16_t steps_right = frame->read<int16_t>();
            _steps_times_4 += steps_left + steps_right;
            //int16_t speed = (int16_t)(((float)steps_left + (float)steps_right) / (2.0f * 8196.0f) / (0.01f) * 21.0f);
            packet[0].set_message_id(ocMessageId::Received_Odo_Steps);
            packet[0].clear_and_edit()
                .write<int32_t>(_steps_times_4 / 4)
                .write<ocTime>(frame->timestamp);
            /*packet[1].set_message_id(ocMessageId::Received_Current_Speed);
            packet[1].clear_and_edit()
                .write<int16_t>(speed)
                .write<ocTime>(frame->timestamp);*/
            //return 2;
        } break;
        case ocCanId::Odo_Rear:
        {
            int16_t steps_left  = frame->read<int16_t>();
            int16_t steps_right = frame->read<int16_t>();
            _steps_times_4 += steps_left + steps_right;
            int16_t speed = (int16_t)(((float)steps_left + (float)steps_right) / (2.0f * 8196.0f) / (0.01f) * 21.0f);
            /*packet[0].set_message_id(ocMessageId::Received_Odo_Steps);
            packet[0].clear_and_edit()
                .write<int32_t>(_steps_times_4 / 4)
                .write<ocTime>(frame->timestamp);*/
            packet[0].set_message_id(ocMessageId::Received_Current_Speed);
            packet[0].clear_and_edit()
                .write<int16_t>(speed)
                .write<ocTime>(frame->timestamp);
            //return 2;
        } break;
        case ocCanId::Task_Report:
        {
            packet->set_message_id(ocMessageId::Driving_Task_Finished);
            packet->clear_and_edit().write<uint8_t>(frame->read<uint8_t>());
        } break;

        case ocCanId::Imu_Rotation_Euler:
        {
            packet->set_message_id(ocMessageId::Imu_Rotation_Euler);
            packet->clear_and_edit()
                .write<float>((float)frame->read<int16_t>() * 0.001090831f /* / 16.0f / 180.0f * M_PI */)
                .write<float>((float)frame->read<int16_t>() * 0.001090831f /* / 16.0f / 180.0f * M_PI */)
                .write<float>((float)frame->read<int16_t>() * 0.001090831f /* / 16.0f / 180.0f * M_PI */);
        } break;

        case ocCanId::Imu_Rotation:
        {
            packet->set_message_id(ocMessageId::Imu_Rotation_Gyro);
            packet->clear_and_edit()
                .write<float>((float)frame->read<int16_t>() * 0.001090831f /* / 16.0f / 180.0f * M_PI */)
                .write<float>((float)frame->read<int16_t>() * 0.001090831f /* / 16.0f / 180.0f * M_PI */)
                .write<float>((float)frame->read<int16_t>() * 0.001090831f /* / 16.0f / 180.0f * M_PI */);
        } break;

        case ocCanId::Imu_Acceleration:
        {
            packet->set_message_id(ocMessageId::Imu_Linear_Acceleration);
            packet->clear_and_edit()
                .write<float>((float)frame->read<int16_t>())
                .write<float>((float)frame->read<int16_t>())
                .write<float>((float)frame->read<int16_t>());
        } break;
        case ocCanId::Imu_Quaternion:
        {
            packet->set_message_id(ocMessageId::Imu_Rotation_Quaternion);
            packet->clear_and_edit()
                .write<float>((float)frame->read<int16_t>() / (float)0x4000)
                .write<float>((float)frame->read<int16_t>() / (float)0x4000)
                .write<float>((float)frame->read<int16_t>() / (float)0x4000)
                .write<float>((float)frame->read<int16_t>() / (float)0x4000);
        }break;
        default:
        {
            return 0;
        }
    }
    return 1;
}

int ocCanGateway::ipc_to_can(const ocPacket *packet, ocCanFrame *frame)
{
    auto reader = packet->read_from_start();
    switch (packet->get_message_id())
    {
        case ocMessageId::Send_Can_Frame:
        {
            *frame = reader.read<ocCanFrame>();
        } break;
        case ocMessageId::Set_Lights:
        {
            ocCarLights lights = reader.read<ocCarLights>();
            frame[0].clear();
            frame[0].id = ocCanId::Light_Absolute;
            frame[0].write<uint8_t>(lights.headlights); // headlights
            frame[0].write<uint8_t>(0x00); // brake
            frame[0].write<uint8_t>(lights.indicator_left); // blink left
            frame[0].write<uint8_t>(lights.indicator_right); // blink right
            frame[0].write<uint8_t>(_rc_active); // rc indicator
        } break;
        case ocMessageId::Start_Driving_Task:
        {
            frame->clear();
            frame->id = ocCanId::Set_Task;
            int16_t speed = reader.read<int16_t>();
            frame->write<int8_t>((int8_t)(speed / 4)); // speed
            frame->write<int8_t>(reader.read<int8_t>()); // steering front
            frame->write<int8_t>(reader.read<int8_t>()); // steering rear
            frame->write<uint8_t>(reader.read<uint8_t>()); // id
            frame->write<int32_t>(reader.read<int32_t>() - _steps_times_4 / 4); // steps // TODO: serious sam should get the absolute steps
        } break;
        default:
        {
            return 0;
        }
    }
    return 1;
}
