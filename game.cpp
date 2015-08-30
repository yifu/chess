#include <stdio.h>
#include <assert.h>

#include "game.hpp"

using namespace std;

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

bool operator == (struct square l, struct square r)
{
    return l.row == r.row && l.col == r.col;
}

bool operator == (struct move l, struct move r)
{
    return l.src == r.src && l.dst == r.dst;
}

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

enum color opponent(enum color c)
{
    if(c == color::white) return color::black;
    else if(c == color::black) return color::white;
    else assert(false);
}

bool is_square_clear(struct game game, struct square square)
{
    assert(square.row < 8);
    assert(square.col < 8);

    for(struct piece piece : game.pieces)
    {
        if(piece.square == square)
            return false;
    }
    return true;
}

struct piece get_piece(struct game game, struct square square)
{
    assert(square.row < 8);
    assert(square.col < 8);

    for(struct piece piece : game.pieces)
    {
        if(piece.square == square)
            return piece;
    }
    assert(false);
}

vector<struct move> generate_pawn_capturing_move(struct game game, size_t pos)
{
    assert(pos != -1);
    assert(pos < game.pieces.size());

    vector<struct move> moves;

    for(int shift : {-1, 1})
    {
        struct piece pawn = game.pieces[pos];
        struct square src = pawn.square;
        struct square dst = src;

        if(pawn.color == color::white)
        {
            dst.row++;
            dst.col += shift;
        }
        else if(pawn.color == color::black)
        {
            dst.row--;
            dst.col += shift;
        }
        else
            assert(false);

        if(dst.row < 0 || dst.row > 7)
            continue;

        if(dst.col < 0 || dst.col > 7)
            continue;

        if(is_square_clear(game, dst))
            continue;

        struct piece captured_piece = get_piece(game, dst);
        if(captured_piece.color == game.cur_player)
            continue;

        // printf("src square = "); print_square(src);
        // printf("dst square = "); print_square(dst);

        struct move move = {src, dst};
        moves.push_back(move);
    }
    return moves;
}

vector<struct move> generate_pawn_starting_move(struct game game, size_t pos)
{
    assert(pos != -1);
    assert(pos < game.pieces.size());

    vector<struct move> moves;

    struct piece pawn = game.pieces[pos];
    struct square src = pawn.square;
    struct square dst = src;

    if(pawn.color == color::white)
    {
        if(pawn.square.row != 1)
            return moves;

        dst.row++;
        if(!is_square_clear(game, dst))
            return moves;

        dst.row++;
        if(!is_square_clear(game, dst))
            return moves;
    }
    else if(pawn.color == color::black)
    {
        if(pawn.square.row != 6)
            return moves;

        dst.row--;
        if(!is_square_clear(game, dst))
            return moves;

        dst.row--;
        if(!is_square_clear(game, dst))
            return moves;
    }
    else
    {
        assert(false);
    }

    // printf("src square = "); print_square(src);
    // printf("dst square = "); print_square(dst);

    struct move move = {src, dst};
    moves.push_back(move);

    return moves;
}

vector<struct move> generate_usual_pawn_move(struct game game, size_t pos)
{
    assert(pos != -1);
    assert(pos < game.pieces.size());

    vector<struct move> moves;

    struct piece pawn = game.pieces[pos];
    struct square src = pawn.square;
    struct square dst = src;

    if(pawn.color == color::white)
        dst.row++;
    else if(pawn.color == color::black)
        dst.row--;
    else
        assert(false);

    if(dst.row < 0 || dst.row > 7)
        return moves;

    if(!is_square_clear(game, dst))
        return moves;

    // printf("src square = "); print_square(src);
    // printf("dst square = "); print_square(dst);

    struct move move = {src, dst};
    moves.push_back(move);
    return moves;
}

vector<struct move> generate_pawn_moves(struct game game, size_t pos)
{
    assert(pos != -1);
    assert(pos < game.pieces.size());

    vector<struct move> moves;
    vector<struct move> usual_moves = generate_usual_pawn_move(game, pos);
    vector<struct move> starting_moves = generate_pawn_starting_move(game, pos);
    vector<struct move> capturing_moves = generate_pawn_capturing_move(game, pos);
    // TODO Implement promoting pawn move.

    moves.insert(moves.begin(), usual_moves.begin(), usual_moves.end());
    moves.insert(moves.begin(), starting_moves.begin(), starting_moves.end());
    moves.insert(moves.begin(), capturing_moves.begin(), capturing_moves.end());

    return moves;
}
vector<struct move> next_moves(struct game game)
{
    vector<struct move> next_moves;
    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        struct piece piece = game.pieces[i];
        if(piece.color != game.cur_player)
            continue;

        vector<struct move> moves;
        switch(piece.type)
        {
        case type::pawn:
        {
            moves = generate_pawn_moves(game, i);
            break;
        }
        case type::knight:
        case type::bishop:
        case type::rook:
        case type::queen:
        case type::king:
            break;

        default:
            assert(false);
            break;
        }

        next_moves.insert(next_moves.end(), moves.begin(), moves.end());
    }
    return next_moves;
}
