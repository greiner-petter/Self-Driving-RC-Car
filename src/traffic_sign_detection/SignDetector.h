#pragma once
#include "../common/ocMember.h"

#include <darknet.h>

class SignDetector
{
public:
    static void Init(ocIpcSocket* socket, ocLogger* logger);
};
