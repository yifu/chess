#pragma once

#include <stdint.h>

enum class msg_type : uint8_t
{
    login = 0x01,
};



#pragma pack(push,1)

struct login
{
    enum msg_type msg_type;
    char username[10];
};

#pragma pack(pop)
