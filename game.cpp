#include "game.hpp"
#include "stdio.h"

using namespace std;

void print_square(struct square square)
{
    printf("square={row=%d,col=%d}\n", square.row, square.col);
}

void print_move(struct move move)
{
    printf("move={src={%d,%d},dst={%d,%d}}\n",
	   move.src.row, move.src.col,
	   move.dst.row, move.src.col);
}

vector<struct piece> initial_board = {
    {color::white, type::pawn, {1, 0}},
    {color::white, type::pawn, {1, 1}},
    {color::white, type::pawn, {1, 2}},
    {color::white, type::pawn, {1, 3}},
    {color::white, type::pawn, {1, 4}},
    {color::white, type::pawn, {1, 5}},
    {color::white, type::pawn, {1, 6}},
    {color::white, type::pawn, {1, 7}},
    {color::white, type::rook, {0, 0}},
    {color::white, type::rook, {0, 7}},
    {color::white, type::knight, {0, 1}},
    {color::white, type::knight, {0, 6}},
    {color::white, type::bishop, {0, 2}},
    {color::white, type::bishop, {0, 5}},
    {color::white, type::queen, {0, 3}},
    {color::white, type::king, {0, 4}},

    {color::black, type::pawn, {6, 0}},
    {color::black, type::pawn, {6, 1}},
    {color::black, type::pawn, {6, 2}},
    {color::black, type::pawn, {6, 3}},
    {color::black, type::pawn, {6, 4}},
    {color::black, type::pawn, {6, 5}},
    {color::black, type::pawn, {6, 6}},
    {color::black, type::pawn, {6, 7}},
    {color::black, type::rook, {7, 0}},
    {color::black, type::rook, {7, 7}},
    {color::black, type::knight, {7, 1}},
    {color::black, type::knight, {7, 6}},
    {color::black, type::bishop, {7, 2}},
    {color::black, type::bishop, {7, 5}},
    {color::black, type::queen, {7, 3}},
    {color::black, type::king, {7, 4}},
};

struct game init_game()
{
    struct game game;
    return game;
}

vector<struct game> next_games(struct game game)
{
    vector<struct game> result;
    return result;
}
