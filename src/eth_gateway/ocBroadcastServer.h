#pragma once

#include "ocConstGateway.h"
#include "../common/ocBuffer.h"
#include "../common/ocLogger.h"

#include <cstddef>
#include <netinet/in.h> // sockaddr_in

class ocBroadcastServer
{
public:
    int fd_listen;

    ocBroadcastServer(const sockaddr *lan_addr);

    bool init_server();
    void process_server();

private:
    sockaddr_storage _broadcast_addr;
    std::byte        _receive_buffer[OC_NET_PACKET_SIZE_MAX];
    ocBuffer         _send_buffer;
    ocLogger         _logger;
};
