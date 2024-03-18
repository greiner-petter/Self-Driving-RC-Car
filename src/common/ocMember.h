#pragma once

#include "ocIpcSocket.h" // ocIpcSocket
#include "ocLogger.h" // ocLogger
#include "ocTypes.h" // ocSharedMemory, ocMemberId

#include <string_view>

class ocMember final
{
public:
    void attach();

    ocSharedMemory *get_shared_memory() {return _shared_memory;}
    ocIpcSocket *get_socket() {return &_socket;}
    ocLogger *get_logger() {return &_logger;}

    ocMember(ocMemberId identifier, std::string_view name);

private:
    ocSharedMemory *_shared_memory;
    ocMemberId      _id;
    ocIpcSocket     _socket;
    ocLogger        _logger;

    int _auth();
};
