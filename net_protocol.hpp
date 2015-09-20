#pragma once

#include <stdint.h>
#include "game.hpp"

enum class msg_type : uint8_t
{
    login = 0x01,
    login_ack = 0x02,
    move_msg = 0x03,
    reject_move_msg = 0x04,
    new_game_msg = 0x05,
    game_evt_msg = 0x06,
    play_online = 0x07,
    players = 0x08,
};

enum class game_evt_type : uint8_t
{
    opponent_resigned = 0x01,
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

struct new_game_msg
{
    enum msg_type msg_type;
    enum color player_color;
};

struct game_evt_msg
{
    enum msg_type msg_type;
    enum game_evt_type game_evt_type;
};

struct play_online_msg
{
    enum msg_type msg_type;
};

struct players_list_msg
{
    enum msg_type msg_type;
    char players_list[100];
};

#pragma pack(pop)
