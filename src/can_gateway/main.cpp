#include "ocCanGateway.h"
#include "../common/ocLogger.h"
#include "../common/ocMember.h"
#include "../common/ocPollEngine.h"
#include "../common/ocProfiler.h"
#include "../common/ocTypes.h"

int32_t main()
{
    // create and init the ocMember we use to communicate with other processes
    ocMember member(ocMemberId::Can_Gateway, "CAN Gateway");
    member.attach();

    ocIpcSocket *ipc_socket = member.get_socket();
    ocLogger *logger = member.get_logger();

    // create and init the CAN interface which we use for sending and receiving CAN frames
    ocCanGateway gateway(member.get_logger());
    if (!gateway.init())
    {
        logger->error("Error while initializing the CAN gateway!");
        return -1;
    }

    // create, fill out and send subscription packet.
    ocPacket s(ocMessageId::Subscribe_To_Messages);
    s.clear_and_edit()
        .write(ocMessageId::Send_Can_Frame)
        .write(ocMessageId::Set_Lights)
        .write(ocMessageId::Start_Driving_Task)
        .write(ocMessageId::Request_Timing_Sites);
    ipc_socket->send_packet(s);

    ocPacket time_packet(ocMessageId::Timing_Events);

    // create a poll engine with which we can wait for any communication to occur
    ocPollEngine pe(2);
    pe.add_fd(gateway.get_socket());
    pe.add_fd(ipc_socket->get_fd());

    // create a packet and a frame once here and use it for all communications in the future
    ocCanFrame can_frame[10];
    ocPacket ipc_packet[10];

    // create, send and log the initial can frame that tells SAM that we're ready
    can_frame[0].clear();
    can_frame[0].id = ocCanId::Boot_Complete;
    can_frame[0].write<uint8_t>(0x03);
    gateway.send_frame(&can_frame[0]);
    ipc_socket->send(ocMessageId::Can_Frame_Transmitted, can_frame[0]);

    // turn on the headlights
    can_frame[0].clear();
    can_frame[0].id = ocCanId::Light_Absolute;
    can_frame[0].write<uint8_t>(0x00); // brake
    can_frame[0].write<uint8_t>(0xFF); // headlights
    can_frame[0].write<uint8_t>(0x00); // blink left
    can_frame[0].write<uint8_t>(0x00); // blink right
    can_frame[0].write<uint8_t>(0x00); // rc indicator
    gateway.send_frame(&can_frame[0]);
    ipc_socket->send(ocMessageId::Can_Frame_Transmitted, can_frame[0]);

    while (1)
    {
        pe.await();

        TIMED_BLOCK("work");
        if (pe.was_triggered(gateway.get_socket()))
        {
            TIMED_BLOCK("can to ipc");
            while (gateway.read_frame(&can_frame[0]))
            {
                int num_packets = gateway.can_to_ipc(&can_frame[0], &ipc_packet[0]);
                if (num_packets < 0)
                {
                    logger->warn("Problem when receiving CAN Frame with identifier: %i", can_frame[0].id);
                }
                else
                {
                    for (int i = 0; i < num_packets; ++i)
                    {
                        ipc_packet[i].set_sender(ocMemberId::Can_Gateway);
                        ipc_socket->send_packet(ipc_packet[i]);
                    }
                }
                ipc_socket->send(ocMessageId::Can_Frame_Received, can_frame[0]);
            }
        }

        if (pe.was_triggered(ipc_socket->get_fd()))
        {
            TIMED_BLOCK("ipc to can");
            while (ipc_socket->read_packet(ipc_packet[0], false) > 0)
            {
                int num_frames = gateway.ipc_to_can(&ipc_packet[0], &can_frame[0]);
                if (num_frames < 0)
                {
                    logger->warn("Problem when sending IPC Packet with identifier: %i", ipc_packet[0].get_message_id());
                }
                else
                {
                    for (int i = 0; i < num_frames; ++i)
                    {
                        gateway.send_frame(&can_frame[i]);
                        ipc_socket->send(ocMessageId::Can_Frame_Transmitted, can_frame[i]);
                    }
                }
            }
            if (ipc_packet[0].get_message_id() == ocMessageId::Request_Timing_Sites)
            {
                time_packet.set_message_id(ocMessageId::Timing_Sites);
                if (write_timing_sites_to_buffer(time_packet.get_payload()))
                {
                    ipc_socket->send_packet(time_packet);
                }
            }
        }

        if (40 < timing_event_count())
        {
            TIMED_BLOCK("send timing data");
            time_packet.set_message_id(ocMessageId::Timing_Events);
            if (write_timing_events_to_buffer(time_packet.get_payload()))
            {
                ipc_socket->send_packet(time_packet);
            }
        }
    }
}
