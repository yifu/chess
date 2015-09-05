#pragma once

#include <stdint.h>
#include "game.hpp"

enum class msg_type : uint8_t
{
    login = 0x01,
    login_ack = 0x02,
    move_msg = 0x03,
    reject_move_msg = 0x04,
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
    enum color player_color;
};

struct move_msg
{
    enum msg_type msg_type;
    uint8_t src_row, src_col, dst_row, dst_col;
};

struct reject_move_msg
{
    enum msg_type msg_type;
};

#pragma pack(pop)
