#pragma once

#include <array>
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
    int row, col;
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
    bool is_captured;
};

struct game
{
    enum color cur_player = color::white;
    std::vector<struct move> moves;
    std::array<struct piece, 32> pieces;
};

extern std::array<struct piece, 32> initial_board;

bool operator == (struct square l, struct square r);
bool operator == (struct move l, struct move r);

void print_square(struct square square);
void print_move(struct move move);
void print_piece(struct piece piece);
void print_game(struct game game);
enum color opponent(enum color c);
bool is_square_clear(struct game game, struct square square);
struct game apply_move(struct game game, struct move move);
bool is_king_captured(struct game game);
std::vector<struct move> next_moves(struct game game);
std::vector<struct game> next_games(struct game game);
std::vector<struct move> next_valid_moves(struct game game);
size_t find_piece_pos(struct game game, struct square square);
