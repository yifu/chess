#pragma once

#include <vector>
#include <stdint.h>

enum class color
{
    white,
    black,
};

enum class type
{
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,
};

struct square
{
    uint8_t row, col;
};

struct move
{
    struct square src, dst;
};

struct piece
{
    enum color color;
    enum type type;
    struct square square;
};

extern std::vector<struct piece> initial_board;

struct game
{
    enum color cur_player = color::white;
    std::vector<struct move> moves;
    std::vector<struct piece> pieces;

};

void print_square(struct square square);
void print_move(struct move move);
struct game init_game();
std::vector<struct game> next_games(struct game game);
