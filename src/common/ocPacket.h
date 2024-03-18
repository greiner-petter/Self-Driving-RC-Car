#pragma once

#include "ocBuffer.h"
#include "ocBufferReader.h"
#include "ocBufferWriter.h"
#include "ocTypes.h" // ocMemberId, ocMessageId

#include <cstdint> // _t types

class ocPacket final
{
private:
    ocMessageId _message_id;
    ocMemberId  _sender;
    ocBuffer    _payload;

public:
    explicit ocPacket(
        ocMessageId message_id = ocMessageId::None,
        ocMemberId  sender = ocMemberId::None
        ): _payload(1 << 24)
    {
        _message_id = message_id;
        _sender = sender;
    }

    ocPacket(const ocPacket&) = delete;

    ocMessageId get_message_id() const
    {
        return _message_id;
    }
    ocMemberId get_sender() const
    {
        return _sender;
    }
    ocBuffer *get_payload()
    {
        return &_payload;
    }
    const ocBuffer *get_payload() const
    {
        return &_payload;
    }

    void set_message_id(ocMessageId message_id)
    {
        _message_id = message_id;
    }
    void set_sender(ocMemberId sender)
    {
        _sender = sender;
    }
    void set_header(ocMessageId message_id, ocMemberId sender)
    {
        _message_id = message_id;
        _sender = sender;
    }

    uint32_t get_length() const
    {
        return (uint32_t)_payload.get_length();
    }

    void clear()
    {
        _payload.clear();
    }

    ocBufferWriter clear_and_edit()
    {
        return _payload.clear_and_edit();
    }
    ocBufferWriter edit_from_end()
    {
        return _payload.edit_from_end();
    }

    ocBufferReader read_from_start() const
    {
        return _payload.read_from_start();
    }
};
