#include "ipc_hub.h"
#include "../common/ocAssert.h"
#include "../common/ocCommon.h"
#include "../common/ocProfiler.h"
#include "../common/ocTypes.h"

#include <cstddef> // offsetof
#include <cstring> // strerror()
#include <cerrno> // errno
#include <csignal> // sigignore(SIGPIPE)

#include <sys/file.h> // flock
#include <fcntl.h> // open
#include <unistd.h> // F_OK, access(), unlink(), close()
#include <sys/shm.h> // shared memory
#include <sys/socket.h> // listen(), accept(), bind()
#include <sys/un.h> // sockaddr_un

IpcHub::IpcHub() : _pe(20), _logger("IPC Server") {}

/* generate listener socket, create socket file and initialize Sets + Flags */
int32_t IpcHub::start_server()
{
    struct sockaddr_un server_addr;

    signal(SIGPIPE, SIG_IGN); // Do not kill yourself if send fails
    _listener = socket(AF_UNIX, SOCK_STREAM, 0); // Create listener socket

    size_t path_length = strlen(OC_SOCKET_PATH);

    if (sizeof(server_addr.sun_path) <= path_length)
    {
        _logger.error("Socket path is too long, must be less than %i, but is %i", sizeof(server_addr.sun_path), strlen(OC_SOCKET_PATH));
        return EXIT_FAILURE;
    }

    int lock_fd = open(OC_SOCKET_LOCK_PATH, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (lock_fd < 0)
    {
        _logger.error("Could not open lock file: (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0)
    {
        _logger.error("Can't acquire lock, another IPC Hub is probably still running. (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    if (access(OC_SOCKET_PATH, F_OK) == 0) // Old socket file exists, so we have to delete it
    {
        if (unlink(OC_SOCKET_PATH) < 0)
        {
            _logger.error("Could not delete old socket file: (%i) %s", errno, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, OC_SOCKET_PATH);

    socklen_t server_addr_len = (socklen_t)(offsetof(sockaddr_un, sun_path) + path_length + 1);

    if (bind(_listener, (struct sockaddr *) &server_addr, server_addr_len) < 0)
    {
        _logger.error("Error at bind: (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    // listen for new connections
    if (listen(_listener, 8) < 0)
    {
        _logger.error("Error at listen: (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    _pe.add_fd(_listener);

    _logger.log("Started IPC server. Listening at: \"%s\"", OC_SOCKET_PATH);

    return EXIT_SUCCESS;
}


/* start the shared memory */
int32_t IpcHub::create_shared_memory()
{
    srand((uint32_t) time(nullptr));
    key_t sm_key = rand(); // key which will generate the shared memory access identification

    _shmid = shmget(sm_key, sizeof(ocSharedMemory), IPC_CREAT | 0666);
    if (_shmid < 0)
    {
        _logger.error("Shared Memory Init failed: (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    _shared_memory = (ocSharedMemory*) shmat(_shmid, nullptr, 0);

    _canaries[0].init(&_shared_memory->_canary0, random_uint64());
    _canaries[1].init(&_shared_memory->_canary1, random_uint64());
    _canaries[2].init(&_shared_memory->_canary2, random_uint64());
    _canaries[3].init(&_shared_memory->_canary3, random_uint64());
    _canaries[4].init(&_shared_memory->_canary4, random_uint64());
    _canaries[5].init(&_shared_memory->_canary5, random_uint64());
    _canaries[6].init(&_shared_memory->_canary6, random_uint64());
    _canaries[7].init(&_shared_memory->_canary7, random_uint64());

    _shared_memory->online_members = (uint16_t) ocMemberId::Ipc_Hub;

    _logger.log("Created Shared Memory. Size: %ibytes ID: 0x%x", sizeof(ocSharedMemory), _shmid);
    return EXIT_SUCCESS;
}

void IpcHub::check_shared_memory()
{
    for (int i = 0; i < 8; ++i)
    {
        oc_assert(_canaries[i].check(), i);
    }
}

/* the main function to process all clients. will listen for incoming packets and forward them to the other clients */
void IpcHub::process_clients()
{
    // block until activity on any socket is detected.
    _pe.await();

    TIMED_BLOCK();
    // check if there was activity on the listen socket which accepts new connections
    if (_pe.was_triggered(_listener))
    {
        TIMED_BLOCK("new client");
        struct sockaddr addr;
        socklen_t s_len = sizeof(addr);

        int new_socket = accept(_listener, (struct sockaddr*) &addr, &s_len);

        if (new_socket < 0)
            _logger.error("Accepting new Client failed: (%i) %s", errno, strerror(errno));
        else
            _handle_auth(new_socket);
    }

    // loop over all connected clients and check if there was activity on their sockets
    for (auto it = _members_by_id.begin(); it != _members_by_id.end();)
    {
        ocMemberId member_id = it->first;
        IpcMember* member = it->second;
        int32_t status = 0;
        if (_pe.was_triggered(member->socket.get_fd()))
        {
            bool disconnect = false;
            uint32_t packets = 0;
            uint32_t bytes = 0;
            member->packets_sent++;
            while (!disconnect && 0 < (status = member->socket.read_packet(_packet, false))) // read non-blockingly
            {
                member->last_active_time = ocTime::now();
                packets += 1;
                bytes += (uint32_t)status;
                if (_packet.get_sender() != member_id)
                {
                    if (ocMemberId::None == _packet.get_sender())
                    {
                        _packet.set_sender(member_id);
                    }
                    else
                    {
                        _logger.warn("Member %s (%i) sent a packet with different sender id: %s (%i)",
                            to_string(member_id),
                            member_id,
                            to_string(_packet.get_sender()),
                            _packet.get_sender());
                    }
                }
                switch (_packet.get_message_id())
                {
                case ocMessageId::Subscribe_To_Messages:
                {
                    auto reader = _packet.read_from_start();
                    while (reader.can_read<ocMessageId>())
                    {
                        ocMessageId message_id = reader.read<ocMessageId>();
                        _subscribers_by_message_id[message_id].append(_packet.get_sender());
                    }
                } break;
                case ocMessageId::Deafen_Member:
                {
                    auto reader = _packet.read_from_start();
                    while (reader.can_read<ocMemberId, bool>())
                    {
                        ocMemberId id = reader.read<ocMemberId>();
                        bool value = reader.read<bool>();
                        if (_members_by_id.find(id) != _members_by_id.end())
                        {
                            _members_by_id[id]->deaf = value;
                        }
                    }
                } break;
                case ocMessageId::Mute_Member:
                {
                    auto reader = _packet.read_from_start();
                    while (reader.can_read<ocMemberId, bool>())
                    {
                        ocMemberId id = reader.read<ocMemberId>();
                        bool value = reader.read<bool>();
                        if (_members_by_id.find(id) != _members_by_id.end())
                        {
                            _members_by_id[id]->mute = value;
                        }
                    }
                } break;
                case ocMessageId::Request_Timing_Sites:
                {
                    _distribute(_packet);

                    _packet.set_sender(ocMemberId::Ipc_Hub);
                    _packet.set_message_id(ocMessageId::Timing_Sites);
                    if (write_timing_sites_to_buffer(_packet.get_payload()))
                    {
                        _distribute(_packet);
                    }
                } break;
                case ocMessageId::Disconnect_Me:
                {
                    it = _disconnect_client(member_id);
                    disconnect = true;
                } break;
                default:
                {
                    _distribute(_packet);
                } break;
                }
            }
            _add_stats(0, packets, 0, bytes);
        }
        if (status < 0)
        {
            _logger.error("Error while receiving from member %s (%i): (%i) %s", to_string(member_id), member_id, errno, strerror(errno));
            it = _disconnect_client(member_id);
        }
        else
        {
            ++it;
        }
    }

    check_shared_memory();

    if (40 < timing_event_count())
    {
        TIMED_BLOCK("Send timing data");
        _packet.set_sender(ocMemberId::Ipc_Hub);
        _packet.set_message_id(ocMessageId::Timing_Events);
        if (write_timing_events_to_buffer(_packet.get_payload()))
        {
            _distribute(_packet);
        }
    }
}

/* distribute a received packet to all receivers */
void IpcHub::_distribute(const ocPacket& packet)
{
    TIMED_BLOCK();

    uint32_t packets = 0;
    uint32_t bytes = 0;

    ocMemberId sender_id = packet.get_sender();
    if (_members_by_id.contains(sender_id) &&
        _members_by_id[sender_id]->mute) return;

    ocMessageId message_id = packet.get_message_id();
    for (ocMemberId receiver_id : _subscribers_by_message_id[message_id])
    {
        oc_assert(_members_by_id.contains(receiver_id));

        IpcMember *receiver = _members_by_id[receiver_id];
        if (!receiver->deaf)
        {
            int32_t result = receiver->socket.send_packet(packet, false);
            if (result == 0)
            {
                _logger.error(
                    "IPC Packet lost due to nonblocking. message_id: %s (%i) from %s (%i) to %s (%i)",
                    to_string(message_id), message_id,
                    to_string(sender_id), sender_id,
                    to_string(receiver_id), receiver_id);
            }
            else if (result < 0)
            {
                _logger.error("IPC Packet lost due to an error. message_id: %s (%i) from %s (%i) to %s (%i) error: (%i) %s",
                    to_string(message_id), message_id,
                    to_string(sender_id), sender_id,
                    to_string(receiver_id), receiver_id,
                    errno, strerror(errno));
                _disconnect_client(receiver_id);
                // TODO: when a member gets disconnected here, they will be replaced by the last member in the _subscribers_by_message_id[message_id] list that
                // we're currently iterating over. This has two problems: 1. we're advancing the iterator in this loop, so that member will be skipped.
                // 2. if the disconnected member was the second-to-last member in that list, we're going to crash because the loop wants to go once more, but
                // by skipping the last one, we're now one over.
            } else
            {
                bytes += (uint32_t)result;
                packets += 1;
                receiver->packets_received++;
            }
        }
    }
    _add_stats(packets, 0, bytes, 0);
}

void IpcHub::_add_stats(uint32_t sent_packets, uint32_t read_packets, uint32_t sent_bytes, uint32_t read_bytes)
{
    static bool recursion = false;

    _packets_sent += sent_packets;
    _packets_read += read_packets;
    _bytes_sent += sent_bytes;
    _bytes_read += read_bytes;

    if (recursion) return;

    auto now = ocTime::now();
    auto diff = now - _last_stat_time;
    if (ocTime::seconds(1) <= diff)
    {
        recursion = true; // make sure we don't recurse into this function
        auto diff_f = diff.get_float_seconds();
        ocPacket stats(ocMessageId::Ipc_Stats, ocMemberId::Ipc_Hub);
        stats.clear_and_edit()
            .write<uint32_t>((uint32_t)((float)_packets_sent / diff_f))
            .write<uint32_t>((uint32_t)((float)_packets_read / diff_f))
            .write<uint32_t>((uint32_t)((float)_bytes_sent / diff_f))
            .write<uint32_t>((uint32_t)((float)_bytes_read / diff_f));

        _packets_sent = 0;
        _packets_read = 0;
        _bytes_sent = 0;
        _bytes_read = 0;
        _distribute(stats);
        _last_stat_time = now;
        recursion = false;
    }
}

int IpcHub::_handle_auth(int new_socket)
{
    IpcMember *member = new IpcMember();
    member->socket.set_fd(new_socket);

    // read the message that the client initially had sent and read the client ID from it
    ocPacket tp;
    member->socket.read_packet(tp);
    ocMemberId member_id = tp.get_sender();
    ocMessageId message_id = tp.get_message_id();
    if (ocMemberId::None == member_id)
    {
        _logger.error("A process sent its auth packet without a valid id.");
        delete member;
        return EXIT_FAILURE;
    }
    if (ocMessageId::Auth_Request != message_id)
    {
        _logger.error(
            "Process (%i) %s send first packet with wrong message id: (%i) %s should be (%i) %s",
            member_id, to_string(member_id),
            message_id, to_string(message_id),
            ocMessageId::Auth_Request, to_string(ocMessageId::Auth_Request));
        delete member;
        return EXIT_FAILURE;
    }
    member->packets_sent++;

    // if a process with that member ID already exists, we kill the old one
    // and let the new one take its place.
    if (_members_by_id.contains(member_id))
    {
        _disconnect_client(member_id);
    }

    // answer with the Shared Memory ID so the client can attach to it
    // we also send a password so the client can be sure that it connected to the right socket.
    tp.set_header(ocMessageId::Auth_Response, ocMemberId::Ipc_Hub);
    tp.clear_and_edit()
        .write<uint32_t>(OC_AUTH_PASSWORD)
        .write<int>(_shmid);

    _logger.log("New connection from %s (%i) at socket %i", to_string(member_id), member_id, new_socket);
    if (member->socket.send_packet(tp) <= 0)
    {
        _logger.error("Failed to send the answer: (%i) %s", errno, strerror(errno));
        delete member;
        return EXIT_FAILURE;
    }
    member->packets_received++;

    // now that the client is properly authenticated, we can add it to the list
    _members_by_id[member_id] = member;

    // add the new socket to the watchlist
    _pe.add_fd(new_socket);

    _shared_memory->online_members |= (uint16_t) member_id;
    _notify_members_changed(member_id, true);

    member->deaf = false;
    member->mute = false;

    return EXIT_SUCCESS;
}

std::map<ocMemberId, IpcMember*>::iterator IpcHub::_disconnect_client(ocMemberId member_id)
{
    auto it = _members_by_id.find(member_id);
    oc_assert(_members_by_id.end() != it);
    IpcMember *member = it->second;

    _pe.delete_fd(member->socket.get_fd());

    it = _members_by_id.erase(it);
    delete member;

    // walk through all the message_ids and remove the process from the subscriber list
    for (auto &entry : _subscribers_by_message_id)
    {
        ocArray<ocMemberId> &arr = entry.second;
        size_t index = arr.first_index_of(member_id);
        if (index < arr.get_length()) arr.remove_at(index);
    }

    _shared_memory->online_members &= (uint16_t) ~(int)member_id;
    _notify_members_changed(member_id, false);

    _logger.log("Disconnected member %s (%i)", to_string(member_id), member_id);

    return it;
}

void IpcHub::_notify_members_changed(ocMemberId member_id, bool came_online)
{
    ocPacket client_info_packet(ocMessageId::Member_List, ocMemberId::Ipc_Hub);
    client_info_packet.clear_and_edit()
        .write<ocMemberId>(member_id)
        .write<bool>(came_online);
    _distribute(client_info_packet);
}
