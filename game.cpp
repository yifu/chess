#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include "game.hpp"

using namespace std;

array<struct piece, 32> initial_board = {{
    {color::white, type::pawn, {1, 0}, false},
    {color::white, type::pawn, {1, 1}, false},
    {color::white, type::pawn, {1, 2}, false},
    {color::white, type::pawn, {1, 3}, false},
    {color::white, type::pawn, {1, 4}, false},
    {color::white, type::pawn, {1, 5}, false},
    {color::white, type::pawn, {1, 6}, false},
    {color::white, type::pawn, {1, 7}, false},
    {color::white, type::rook, {0, 0}, false},
    {color::white, type::rook, {0, 7}, false},
    {color::white, type::knight, {0, 1}, false},
    {color::white, type::knight, {0, 6}, false},
    {color::white, type::bishop, {0, 2}, false},
    {color::white, type::bishop, {0, 5}, false},
    {color::white, type::queen, {0, 3}, false},
    {color::white, type::king, {0, 4}, false},

    {color::black, type::pawn, {6, 0}, false},
    {color::black, type::pawn, {6, 1}, false},
    {color::black, type::pawn, {6, 2}, false},
    {color::black, type::pawn, {6, 3}, false},
    {color::black, type::pawn, {6, 4}, false},
    {color::black, type::pawn, {6, 5}, false},
    {color::black, type::pawn, {6, 6}, false},
    {color::black, type::pawn, {6, 7}, false},
    {color::black, type::rook, {7, 0}, false},
    {color::black, type::rook, {7, 7}, false},
    {color::black, type::knight, {7, 1}, false},
    {color::black, type::knight, {7, 6}, false},
    {color::black, type::bishop, {7, 2}, false},
    {color::black, type::bishop, {7, 5}, false},
    {color::black, type::queen, {7, 3}, false},
    {color::black, type::king, {7, 4}, false},
}};

bool operator == (struct square l, struct square r)
{
    return l.row == r.row && l.col == r.col;
}

bool operator == (struct move l, struct move r)
{
    return l.src == r.src && l.dst == r.dst;
}

const char *color2string(enum color c)
{
    switch(c)
    {
    case color::white: return "white";
    case color::black: return "black";
    default: assert(false); return "unknown";
    }
    return "-unknown-";
}

const char *type2string(enum type t)
{
    switch(t)
    {
    case type::pawn: return "pawn";
    case type::rook: return "rook";
    case type::knight: return "knight";
    case type::bishop: return "bishop";
    case type::queen: return "queen";
    case type::king: return "king";
    default: assert(false); return "unknown";
    }
    return "-unknown-";
}

// TODO Overload all that?
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

void print_game(struct game game)
{
    printf("game={cur_player=%s, moves={",
           color2string(game.cur_player));
    for(struct move move : game.moves)
    {
        print_move(move);
        printf(",");
    }

    printf("}, pieces={");
    for(struct piece piece : game.pieces)
    {
        print_piece(piece);
        printf(",");
    }
    printf("}}\n");
}

void print_piece(struct piece piece)
{
    printf("piece={color=%s, type=%s, square={%d,%d}, is_captured=%d}\n",
           color2string(piece.color),
           type2string(piece.type),
           piece.square.row,
           piece.square.col,
           piece.is_captured);
}

enum color opponent(enum color c)
{
    if(c == color::white) return color::black;
    else if(c == color::black) return color::white;
    else assert(false);
}

bool is_valid_row(int row)
{
    return row >= 0 && row <= 7;
}

bool is_valid_col(int col)
{
    return col >= 0 && col <= 7;
}

bool is_valid_square(struct square square)
{
    return is_valid_row(square.row) && is_valid_col(square.col);
}

bool is_square_clear(struct game game, struct square square)
{
    assert(is_valid_square(square));

    for(struct piece piece : game.pieces)
    {
        if(piece.is_captured)
            continue;
        if(piece.square == square)
            return false;
    }
    return true;
}

// Remove get_piece() call by find_piece_pos()
struct piece get_piece(struct game game, struct square square)
{
    assert(is_valid_square(square));

    for(struct piece piece : game.pieces)
    {
        if(piece.is_captured)
            continue;
        if(piece.square == square)
            return piece;
    }
    assert(false);
}

size_t find_piece_pos(struct game game, struct square square)
{
    assert(is_valid_square(square));

    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        struct piece piece = game.pieces[i];
        if(piece.is_captured)
            continue;
        if(piece.square == square)
            return i;
    }
    return -1;
}

vector<struct move> generate_pawn_capturing_move(struct game game, size_t pos)
{
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

        if(!is_valid_square(dst))
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

    if(!is_valid_row(dst.row))
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

struct square& operator += (struct square& l, struct square r)
{
    l.row += r.row;
    l.col += r.col;
    return l;
}

vector<struct move> generate_sliding_moves(struct game game, size_t pos,
                                           vector<struct square> directions)
{
    assert(pos < game.pieces.size());

    vector<struct move> moves;
    for(square direction : directions)
    {
        struct piece piece = game.pieces[pos];
        struct square src = piece.square;
        struct square dst = src;

        dst += direction;
        while(is_valid_square(dst))
        {
            if(is_square_clear(game, dst))
            {
                moves.push_back({src,dst});
                dst += direction;
            }
            else
            {
                assert(!is_square_clear(game, dst));
                size_t pos = find_piece_pos(game, dst);
                assert(pos < game.pieces.size());
                if(game.pieces[pos].color != game.cur_player)
                    moves.push_back({src,dst});
                break;
            }
        }
    }
    return moves;
}

vector<struct move> generate_rook_moves(struct game game, size_t pos)
{
    return generate_sliding_moves(game, pos, {{0,1},{1,0},{0,-1},{-1,0}});
}

vector<struct move> generate_bishop_moves(struct game game, size_t pos)
{
    return generate_sliding_moves(game, pos, {{1,1},{-1,1},{-1,-1},{1,-1}});
}

vector<struct move> generate_queen_moves(struct game game, size_t pos)
{
    vector<struct square> directions = {
        {0,1},{1,0},{0,-1},{-1,0},{1,1},{-1,1},{-1,-1},{1,-1}
    };
    return generate_sliding_moves(game, pos, directions);
}

vector<struct move> generate_king_moves(struct game game, size_t pos)
{
    vector<struct move> moves;
    vector<struct square> directions = {
        {0,1},{1,0},{0,-1},{-1,0},{1,1},{-1,1},{-1,-1},{1,-1}
    };

    for(struct square direction : directions)
    {
        struct piece king = game.pieces[pos];
        struct square src = king.square;
        struct square dst = src;

        dst += direction;
        if(is_valid_square(dst))
        {
            if(is_square_clear(game, dst))
            {
                moves.push_back({src,dst});
            }
            else
            {
                assert(!is_square_clear(game, dst));
                size_t pos = find_piece_pos(game, dst);
                assert(pos < game.pieces.size());
                if(game.pieces[pos].color != game.cur_player)
                    moves.push_back({src,dst});
            }
        }
    }

    // TODO Implement Castling.

    return moves;
}

vector<struct move> generate_knight_moves(struct game game, size_t pos)
{
    vector<struct move> moves;
    vector<struct square> directions = {
        {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}
    };

    for(struct square direction : directions)
    {
        struct piece knight = game.pieces[pos];
        struct square src = knight.square;
        struct square dst = src;

        dst += direction;
        if(is_valid_square(dst))
        {
            if(is_square_clear(game, dst))
            {
                moves.push_back({src,dst});
            }
            else
            {
                assert(!is_square_clear(game, dst));
                size_t pos = find_piece_pos(game, dst);
                assert(pos < game.pieces.size());
                if(game.pieces[pos].color != game.cur_player)
                    moves.push_back({src,dst});
            }
        }
    }
    return moves;
}

struct game apply_move(struct game game, struct move move)
{
    size_t src_pos, dst_pos;

    game.moves.push_back(move);
    game.cur_player = opponent(game.cur_player);

    dst_pos = find_piece_pos(game, move.dst);
    if(dst_pos == (size_t)-1)
        goto init_src;

    assert(dst_pos < game.pieces.size());
    game.pieces[dst_pos].is_captured = true;

// TODO Refactor that.
// dst must be modified before src.
init_src:
    src_pos = find_piece_pos(game, move.src);
    assert(src_pos < game.pieces.size());
    game.pieces[src_pos].square = move.dst;

    return game;
}

bool is_king_captured(struct game game)
{
    for(struct piece piece : game.pieces)
    {
        if(piece.type != type::king)
            continue;
        if(piece.color !=  game.cur_player)
            continue;
        return piece.is_captured;
    }
    assert(false);
    return false;
}

vector<struct move> next_moves(struct game game)
{
    vector<struct move> cur_player_moves;
    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        struct piece piece = game.pieces[i];
        if(piece.color != game.cur_player)
            continue;
        // TODO Find a solution. Too much issue with captured pieces.
        // for_each_uncaptured_pieces() ?
        if(piece.is_captured)
            continue;

        vector<struct move> moves;
        switch(piece.type)
        {
        case type::pawn:
            moves = generate_pawn_moves(game, i);
            break;
        case type::rook:
            moves = generate_rook_moves(game, i);
            break;
        case type::knight:
            moves = generate_knight_moves(game, i);
            break;
        case type::bishop:
            moves = generate_bishop_moves(game, i);
            break;
        case type::queen:
            moves = generate_queen_moves(game, i);
            break;
        case type::king:
            moves = generate_king_moves(game, i);
            break;
        default:
            assert(false);
            break;
        }

        cur_player_moves.insert(cur_player_moves.end(), moves.begin(), moves.end());
    }
    return cur_player_moves;
}

vector<struct game> next_games(struct game game)
{
    vector<struct move> moves = next_moves(game);
    vector<struct game> games;
    games.reserve(moves.size());

    for(struct move move : moves)
        games.push_back(apply_move(game, move));

    return games;
}

vector<struct move> next_valid_moves(struct game game)
{
    vector<struct move> moves;

    for(struct move move : next_moves(game))
    {
        struct game i = apply_move(game, move);
        assert(i.cur_player != game.cur_player);

        bool is_valid = true;
        for(struct game j : next_games(i))
        {
            assert(j.cur_player == game.cur_player);
            if(is_king_captured(j))
            {
                is_valid = false;
                break;
            }
        }

        if(is_valid)
            moves.push_back(move);
   }

    return moves;
}

bool is_king_checked(struct game game)
{
    game.cur_player = opponent(game.cur_player);
    for(struct game next_game : next_games(game))
    {
        if(is_king_captured(next_game))
            return true;
    }
    return false;
}
