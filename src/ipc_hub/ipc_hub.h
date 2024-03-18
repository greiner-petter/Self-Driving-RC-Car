#pragma once

#include "../common/ocConst.h"
#include "../common/ocLogger.h"
#include "../common/ocPacket.h"
#include "../common/ocPollEngine.h"
#include "../common/ocIpcSocket.h"
#include "../common/ocTime.h"
#include "../common/ocTypes.h"

#include "ocCanary.h"

#include <map>

struct IpcMember
{
    ocIpcSocket socket;
    ocMemberId id = ocMemberId::None;
    bool deaf = true;
    bool mute = true;
    uint64_t packets_sent     = 0;
    uint64_t packets_received = 0;
    ocTime   last_active_time;
};

class IpcHub
{
public:
    IpcHub();

    // init itself
    int start_server();

    // init shared memory
    int create_shared_memory();

    // check the canaries in the shred memory for modifications
    void check_shared_memory();

    // read at all connected sockets and do the sending/receiving
    void process_clients();

private:
    // Shared Memory Key
    int _shmid;

    // the shared memory itself
    ocSharedMemory *_shared_memory;

    ocPacket _packet;

    // Socket-Management
    int _listener;
    ocPollEngine _pe;

    ocLogger _logger;

    ocTime   _last_stat_time = ocTime::now();
    uint32_t _packets_sent = 0;
    uint32_t _packets_read = 0;
    uint32_t _bytes_sent = 0;
    uint32_t _bytes_read = 0;

    ocCanary<uint64_t> _canaries[8];

    // list of all connected clients
    std::map<ocMemberId, IpcMember*> _members_by_id;

    // list of subscribers for every message_id
    std::map<ocMessageId, ocArray<ocMemberId>> _subscribers_by_message_id;

    /* Private Functions */
    void _add_stats(uint32_t sent_packets, uint32_t read_packets, uint32_t sent_bytes, uint32_t read_bytes);

    // create a new client, add it to the list and send it the shared memory key
    int _handle_auth(int newConnection);

    // send a packet to all clients that should receive it
    void _distribute(const ocPacket& packet);

    // remove a client and clear all the message_ids it was subscribed to
    std::map<ocMemberId, IpcMember*>::iterator _disconnect_client(ocMemberId client_id);

    // send a packet to everyone who cares about newly connected and disconnected members
    void _notify_members_changed(ocMemberId member_id, bool came_online);
};
