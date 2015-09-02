#pragma once

#include <stdint.h>

enum class msg_type : uint8_t
{
    login = 0x01,
    login_ack = 0x02,
};

enum class player_color : uint8_t
{
    white = 0x01,
    black = 0x02,
};

#pragma pack(push,1)

struct login
{
    enum msg_type msg_type;
    char username[10];
    // TODO Protocol version + app version.
};

struct login_ack
{
    enum msg_type msg_type;
    enum player_color player_color;
};

#pragma pack(pop)
