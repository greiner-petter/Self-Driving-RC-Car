#include "ocMember.h"
#include "ocPacket.h"

#include <cstdlib> // exit(), EXIT_FAILURE, SUCCESS
#include <cstdint> // _t ints
#include <cerrno> // errno

#include <sys/socket.h> // Sockets
#include <sys/un.h> // Unix Socket Structures
#include <sys/shm.h> // Shared Memory
#include <unistd.h> // sleep

ocMember::ocMember(ocMemberId identifier, std::string_view name) :
    _socket(),
    _logger(name.data())
{
    _id = identifier;
}

/* register the process at the server with an id */

void ocMember::attach()
{
    int32_t sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        _logger.error("socket(): (%i) %s", errno, strerror(errno));
        exit(-1);
    }

    struct sockaddr_un remote; // socket target struct
    remote.sun_family = AF_UNIX; // Interprocess Communication Socket
    strcpy(remote.sun_path, OC_SOCKET_PATH);

    // size of the socket address
    socklen_t len = (socklen_t)(strlen(remote.sun_path) + sizeof(remote.sun_family));

    while (connect(sock, (struct sockaddr*) &remote, len) < 0)
    {
        _logger.warn("connect() failed, retrying in 1s: (%i) %s", errno, strerror(errno));
        sleep(1);
    }

    _socket.set_fd(sock);

    if (EXIT_FAILURE == _auth()) {
        exit(-1);
    }
}

/* private function to authenticate and get the shared memory id */

int ocMember::_auth()
{
    /* First we send the authentication packet */
    ocPacket auth_packet(ocMessageId::Auth_Request, _id);
    if (_socket.send_packet(auth_packet) <= 0)
    {
        _logger.error("Could not send auth packet: (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    /* then we shall receive the answer with the password and shared memory ID */
    _socket.read_packet(auth_packet, true);
    if (auth_packet.get_message_id() != ocMessageId::Auth_Response)
    {
        _logger.error("Received wrong response message id: %i", auth_packet.get_message_id());
        return EXIT_FAILURE;
    }

    auto reader = auth_packet.read_from_start();
    if (!reader.can_read<uint32_t, int>())
    {
        _logger.error("Received too small auth packet. That shouldn't happen.");
        return EXIT_FAILURE;
    }

    /* check the password to make sure that we connected to the correct socket and process */
    uint32_t password = reader.read<uint32_t>();
    if (password != OC_AUTH_PASSWORD)
    {
        _logger.error("Connected to the wrong socket. Oops. Bye bye.");
        _logger.error("length: %u pwd: 0x%x", auth_packet.get_payload()->get_length(), password);
        return EXIT_FAILURE;
    }

    /* extract the access-id for the shared memory and attach to it */
    int sharedmemory_id = reader.read<int>();

    void *shmaddr = shmat(sharedmemory_id, nullptr, 0);
    if (((void *)-1) == shmaddr)
    {
        _logger.error("Error while attaching the shared memory: (%i) %s", errno, strerror(errno));
        return EXIT_FAILURE;
    }

    _shared_memory = (ocSharedMemory*) shmaddr;

    _logger.log("Connection successful, Shared Memory ID: 0x%x", sharedmemory_id);
    return EXIT_SUCCESS;
}
