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

struct game
{
    enum color cur_player = color::white;
    std::vector<struct move> moves;
    std::vector<struct piece> pieces;
};

extern std::vector<struct piece> initial_board;

bool operator == (struct square l, struct square r);
bool operator == (struct move l, struct move r);

void print_square(struct square square);
void print_move(struct move move);
enum color opponent(enum color c);
std::vector<struct move> next_moves(struct game game);
