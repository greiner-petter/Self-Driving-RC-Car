#include "ocBroadcastServer.h"

#include "../common/ocProfiler.h"

#include <arpa/inet.h> // inet_ntoa()
#include <cstring> // strerror()
#include <ifaddrs.h>
#include <sys/socket.h>

bool ocBroadcastServer::init_server()
{
    int setsockopt_1 = 1; // needed because setsockopt needs something to point to

    fd_listen = socket(_broadcast_addr.ss_family, SOCK_DGRAM, 0);
    if (fd_listen < 0)
    {
        _logger.error("Failed to create listen socket: (%i) %s", errno, strerror(errno));
        return false;
    }

    if (setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, (uint8_t*) &setsockopt_1, sizeof(setsockopt_1)) < 0)
    {
        _logger.error("setsockopt SO_REUSEADDR failed: (%i) %s", errno, strerror(errno));
        return false;
    }

    if (bind(fd_listen, (sockaddr *)&_broadcast_addr, sizeof(sockaddr_storage)) < 0)
    {
        _logger.error("bind() failed: (%i) %s", errno, strerror(errno));
        return false;
    }
    _logger.log("Broadcast Server is listening.");

    return true;
}

ocBroadcastServer::ocBroadcastServer(const sockaddr *lan_addr) : _logger("Broadcast Server")
{
    memset(&_broadcast_addr, 0, sizeof(sockaddr_storage));
    switch (lan_addr->sa_family) {
        case AF_INET: {
            memcpy(&_broadcast_addr, lan_addr, sizeof(sockaddr_in));
            sockaddr_in *lip4 = (sockaddr_in *)&_broadcast_addr;
            _send_buffer.clear_and_edit()
                .write<ocUdpId>(ocUdpId::Beacon)
                .write<uint32_t>(15)
                .write<uint16_t>((uint16_t) OC_NET_UDP_BEACON_MAGIC)
                .write<uint16_t>(OC_NET_UDP_PROTOCOL_VERSION)
                .write<uint32_t>(lip4->sin_addr.s_addr)
                .write<uint32_t>(lip4->sin_addr.s_addr)
                .write<uint16_t>(OC_NET_TCP_ADAPTER_PORT)
                .write<uint8_t>(0);
            lip4->sin_addr.s_addr = INADDR_ANY;
            lip4->sin_port = htons(OC_NET_UDP_ADAPTER_PORT);
        } break;
//        case AF_INET6: {
//            TODO: implement ipv6
//
//            memcpy(&_broadcast_addr, lan_addr, sizeof(sockaddr_in6));
//            const sockaddr_in6 *lip6 = (const sockaddr_in6 *)&_broadcast_addr;
//            lip6->sin6_addr = in6addr_any;
//            lip6->sin6_port = htons(OC_NET_UDP_ADAPTER_PORT);
//        } break;
        default:
            _logger.warn("Unsupported address type: %i", lan_addr->sa_family);
    }
}

void ocBroadcastServer::process_server()
{
    TIMED_BLOCK("broadcasting");

    sockaddr_storage client_addr = {};
    socklen_t addr_size = (socklen_t)sizeof(client_addr);
    ssize_t recv_ret = recvfrom(
        fd_listen,
        &_receive_buffer[0],
        OC_NET_PACKET_SIZE_MAX,
        MSG_DONTWAIT,
        (sockaddr *)&client_addr,
        &addr_size);

    if (0 < recv_ret)
    {
        ocUdpId opcode = (ocUdpId)_receive_buffer[0]; // 1 byte opcode
        int32_t payload_len; // 4 byte len in container
        memcpy(&payload_len, &_receive_buffer[1], sizeof(int32_t));
        if ((payload_len + OC_NET_PACKET_HEADER_SIZE) != recv_ret)
        {
            _logger.warn("received size mismatch");
            // TODO the server might be in a bad state now, what can we do about that?
        }

        switch(opcode)
        {
            case ocUdpId::Beacon_Request:
            {
                ssize_t send_ret = sendto(
                    fd_listen,
                    _send_buffer.get_space(_send_buffer.get_length()),
                    _send_buffer.get_length(),
                    MSG_DONTWAIT,
                    (sockaddr *)&client_addr,
                    addr_size);

                if (send_ret < 0 && errno != EWOULDBLOCK)
                {
                    _logger.warn("UDP sendto error");
                }
            } break;
            case ocUdpId::Ping:
            {
            } break;
            default:
            {
                _logger.warn("Received unknown opcode: 0x%x length: %i", opcode, payload_len);
            } break;
        }
    }
    else if (errno != EWOULDBLOCK)
    {
        _logger.warn("UDP recvfrom error: (%i) %s", errno, strerror(errno));
    }
}
