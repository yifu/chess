#include <stdio.h>
#include <assert.h>
#include <algorithm>

#include "game.hpp"

using namespace std;

#include <sys/types.h>
#include <unistd.h>

string filename = string("debug-") + to_string(getpid()) + ".txt";
FILE *dbg = fopen(filename.c_str(), "w");

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

struct square mk_square(int row, int col)
{
    struct square s {row, col};
    return s;
}

bool operator != (struct square l, struct square r)
{
    return l.row != r.row || l.col != r.col;
}

bool operator == (struct square l, struct square r)
{
    return l.row == r.row && l.col == r.col;
}

bool operator == (struct move l, struct move r)
{
    return l.src == r.src && l.dst == r.dst && l.promotion == r.promotion;
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
    case type::none: return "none";
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
    fprintf(dbg, "square={row=%d,col=%d}\n", square.row, square.col);
}

void print_move(struct move move)
{
    fprintf(dbg, "move={src={%d,%d},dst={%d,%d},promotion=%s}\n",
            move.src.row, move.src.col,
            move.dst.row, move.src.col,
            type2string(move.promotion));
}

void print_game(struct game game)
{
    fprintf(dbg, "game={cur_player=%s, moves={",
           color2string(game.cur_player));
    for(struct move move : game.moves)
    {
        print_move(move);
        fprintf(dbg, ",");
    }

    fprintf(dbg, "}, pieces={");
    for(struct piece piece : game.pieces)
    {
        print_piece(piece);
        fprintf(dbg, ",");
    }
    fprintf(dbg, "}}\n");
}

void print_piece(struct piece piece)
{
    fprintf(dbg, "piece={color=%s, type=%s, square={%d,%d}, is_captured=%d}\n",
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

vector<struct move> generate_pawn_en_passant_move(struct game game, size_t pos)
{
    assert(pos < game.pieces.size());

    vector<struct move> moves;

    if(game.moves.empty())
        return moves;

    struct move last_move = game.moves.back();
    struct piece piece = get_piece(game, last_move.dst);
    struct piece pawn = game.pieces[pos];

    if(piece.type != type::pawn)
        return moves;

    assert(piece.color == opponent(game.cur_player));

    if(game.cur_player == color::white &&
       last_move.src.row == 6 &&
       last_move.dst.row == 4 &&
       last_move.src.col == last_move.dst.col &&
       last_move.dst.row == pawn.square.row &&
       ((pawn.square.col == last_move.dst.col-1) ||
        (pawn.square.col == last_move.dst.col+1)))
    {
        struct square dst {pawn.square.row+1, last_move.dst.col};
        struct move move {pawn.square, dst, type::none};
        moves.push_back(move);
    }
    else if(game.cur_player == color::black &&
            last_move.src.row == 1 &&
            last_move.dst.row == 3 &&
            last_move.src.col == last_move.dst.col &&
            last_move.dst.row == pawn.square.row &&
            ((pawn.square.col == last_move.dst.col-1) ||
             (pawn.square.col == last_move.dst.col+1)))
    {
        struct square dst {pawn.square.row-1, last_move.dst.col};
        struct move move {pawn.square, dst, type::none};
        moves.push_back(move);
    }

    return moves;
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

        // fprintf(dbg, "src square = "); print_square(src);
        // fprintf(dbg, "dst square = "); print_square(dst);

        struct move move = {src, dst, type::none};

        if(game.cur_player == color::white && dst.row == 7)
            move.promotion = type::queen;
        else if(game.cur_player == color::black && dst.row == 0)
            move.promotion = type::queen;

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

    // fprintf(dbg, "src square = "); print_square(src);
    // fprintf(dbg, "dst square = "); print_square(dst);

    struct move move = {src, dst, type::none};
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

    // fprintf(dbg, "src square = "); print_square(src);
    // fprintf(dbg, "dst square = "); print_square(dst);

    struct move move = {src, dst, type::none};

    if(game.cur_player == color::white && dst.row == 7)
        move.promotion = type::queen;
    else if(game.cur_player == color::black && dst.row == 0)
        move.promotion = type::queen;

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
    vector<struct move> en_passant_moves = generate_pawn_en_passant_move(game, pos);

    moves.insert(moves.begin(), usual_moves.begin(), usual_moves.end());
    moves.insert(moves.begin(), starting_moves.begin(), starting_moves.end());
    moves.insert(moves.begin(), capturing_moves.begin(), capturing_moves.end());
    moves.insert(moves.begin(), en_passant_moves.begin(), en_passant_moves.end());

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
                moves.push_back({src, dst, type::none});
                dst += direction;
            }
            else
            {
                assert(!is_square_clear(game, dst));
                size_t pos = find_piece_pos(game, dst);
                assert(pos < game.pieces.size());
                if(game.pieces[pos].color != game.cur_player)
                    moves.push_back({src, dst, type::none});
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

bool piece_moved(struct game game, size_t pos)
{
    struct square init_square = initial_board[pos].square;

    for(auto move : game.moves)
        if(move.src == init_square)
            return true;

    return false;
}

vector<struct move> generate_king_moves(struct game game, size_t pos)
{
    vector<struct move> moves;
    vector<struct square> directions = {
        {0,1},{1,0},{0,-1},{-1,0},{1,1},{-1,1},{-1,-1},{1,-1}
    };

    struct piece king = game.pieces[pos];
    struct square src = king.square;

    for(struct square direction : directions)
    {
        struct square dst = src;

        dst += direction;
        if(is_valid_square(dst))
        {
            if(is_square_clear(game, dst))
            {
                moves.push_back({src, dst, type::none});
            }
            else
            {
                assert(!is_square_clear(game, dst));
                size_t pos = find_piece_pos(game, dst);
                assert(pos < game.pieces.size());
                if(game.pieces[pos].color != game.cur_player)
                    moves.push_back({src, dst, type::none});
            }
        }
    }

    if(game.cur_player == color::white && game.is_white_king_checked)
        return moves;
    else if(game.cur_player == color::black && game.is_black_king_checked)
        return moves;

    if(piece_moved(game, pos))
        return moves;

    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        struct piece piece = game.pieces[i];

        if(piece.is_captured)
            continue;

        if(piece.type != type::rook)
            continue;

        if(piece.color != game.cur_player)
            continue;

        if(piece_moved(game, i))
            continue;

        struct piece rook = piece;

        struct square direction;
        if(rook.square.col > src.col)
            direction = mk_square(0,1);
        else
            direction = mk_square(0,-1);

        struct square dst = src;
        dst += direction;
        bool cleared_path = true;
        while(dst != rook.square)
        {
            if(not is_square_clear(game, dst))
                cleared_path = false;
            dst += direction;
        }
        if(not cleared_path)
            continue;

        dst = src;
        for(size_t i = 0; i < 2; i++)
        {
            dst += direction;
            struct game next_game = apply_move(game, {src, dst, type::none});
            assert(next_game.cur_player == opponent(game.cur_player));

            if(game.cur_player == color::white && game.is_white_king_checked)
                cleared_path = false;
            else if(game.cur_player == color::black && game.is_black_king_checked)
                cleared_path = false;
        }

        if(cleared_path)
            moves.push_back({src, dst, type::none});
    }

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
                moves.push_back({src, dst, type::none});
            }
            else
            {
                assert(!is_square_clear(game, dst));
                size_t pos = find_piece_pos(game, dst);
                assert(pos < game.pieces.size());
                if(game.pieces[pos].color != game.cur_player)
                    moves.push_back({src, dst, type::none});
            }
        }
    }
    return moves;
}

bool is_castling_move(struct game game, struct move move)
{
    // fprintf(dbg, "inside is_castling_move().\n");
    size_t src_pos = find_piece_pos(game, move.src);
    struct piece src_piece = game.pieces[src_pos];

    if(src_piece.type != type::king)
    {
        // fprintf(dbg, "Not a king = %s.\n", type2string(src_piece.type));
        return false;
    }

    if(src_piece.color == color::white)
    {
        if(src_piece.square != mk_square(0,4))
        {
            // fprintf(dbg, "Not a king?\n");
            // print_square(src_piece.square);
            return false;
        }

        if(move.dst != mk_square(0,2) &&
           move.dst != mk_square(0,6))
        {
            // fprintf(dbg, "Wrong dst square.\n");
            // print_square(move.dst);
            return false;
        }
    }

    if(src_piece.color == color::black)
    {
        if(src_piece.square != mk_square(7,4))
        {
            // fprintf(dbg, "Wrong src square.\n");
            // print_square(src_piece.square);
            return false;
        }

        if(move.dst != mk_square(7,2) &&
           move.dst != mk_square(7,6))
        {
            // fprintf(dbg, "Wrong dst square for black.\n");
            // print_square(move.dst);
            return false;
        }
    }

    return true;
}

bool is_en_passant_move(struct game game, struct move move)
{
    size_t src_pos = find_piece_pos(game, move.src);
    size_t dst_pos = find_piece_pos(game, move.dst);
    struct piece src_piece = game.pieces[src_pos];

    if(src_piece.type != type::pawn)
        return false;

    if(move.src.col == move.dst.col)
        return false;

    if(dst_pos != (size_t)-1)
        return false;

    return true;
}

struct game apply_move(struct game game, struct move move)
{
    size_t src_pos = find_piece_pos(game, move.src);
    size_t dst_pos = find_piece_pos(game, move.dst);
    size_t rook_pos = (size_t)-1;

    assert(src_pos < game.pieces.size());
//    assert(dst_pos < game.pieces.size());

    struct piece src_piece = game.pieces[src_pos];
    bool is_castling_mv = is_castling_move(game, move);
    if(is_castling_mv)
    {
        // fprintf(dbg, "is castling move.\n");
        assert(src_piece.type == type::king);
        assert(dst_pos == (size_t)-1);

        if(src_piece.square.row == 0)
        {
            // fprintf(dbg, "Castling for white.\n");
            assert(src_piece.color == color::white);
            if(move.dst.col == 2) // Big castle.
            {
                // fprintf(dbg, "big castle.\n");
                rook_pos = find_piece_pos(game, {0,0});
                assert(rook_pos != (size_t)-1);
                struct piece rook = game.pieces[rook_pos];
                assert(rook.color == color::white);
                assert(find_piece_pos(game, {0,3}) == (size_t)-1);
                game.pieces[rook_pos].square = {0,3};
            }
            else if(move.dst.col == 6) // little castle.
            {
                // fprintf(dbg, "little castle.\n");
                rook_pos = find_piece_pos(game, {0,7});
                assert(rook_pos != (size_t)-1);
                struct piece rook = game.pieces[rook_pos];
                assert(rook.color == color::white);
                assert(find_piece_pos(game, {0,5}) == (size_t)-1);
                game.pieces[rook_pos].square = {0,5};
            }
        }
        else if(src_piece.square.row == 7)
        {
            // fprintf(dbg, "Castling for black.\n");
            assert(src_piece.color == color::black);
            if(move.dst.col == 2) // Big castle.
            {
                rook_pos = find_piece_pos(game, {7,0});
                assert(rook_pos != (size_t)-1);
                struct piece rook = game.pieces[rook_pos];
                assert(rook.color == color::black);
                assert(find_piece_pos(game, {7,3}) == (size_t)-1);
                game.pieces[rook_pos].square = {7,3};
            }
            else if(move.dst.col == 6) // litlle castle.
            {
                rook_pos = find_piece_pos(game, {7,7});
                assert(rook_pos != (size_t)-1);
                struct piece rook = game.pieces[rook_pos];
                assert(rook.color == color::black);
                assert(find_piece_pos(game, {7,5}) == (size_t)-1);
                game.pieces[rook_pos].square = {7,5};
            }
        }
    }

    if(is_en_passant_move(game, move))
    {
        struct square captured_square = move.dst;
        if(game.cur_player == color::white)
            captured_square.row--;
        else if(game.cur_player == color::black)
            captured_square.row++;
        else
            assert(false);

        size_t pos = find_piece_pos(game, captured_square);
        assert(pos != (size_t)-1);
        struct piece& captured_piece = game.pieces[pos];
        captured_piece.is_captured = true;
    }

    game.moves.push_back(move);
    game.cur_player = opponent(game.cur_player);

    src_piece.square = move.dst;
    if(move.promotion != type::none)
    {
        assert(src_piece.type == type::pawn);
        src_piece.type = move.promotion;
    }
    game.pieces[src_pos] = src_piece;

    if(dst_pos != (size_t)-1)
    {
        struct piece dst_piece = game.pieces[dst_pos];
        dst_piece.is_captured = true;
        game.pieces[dst_pos] = dst_piece;
    }

    return game;
}

void update_king_statuses(struct game& game)
{
    game.is_white_king_checked = is_white_king_checked(game);
    game.is_black_king_checked = is_black_king_checked(game);

    assert(not(game.is_white_king_checked && game.is_black_king_checked));

    if(opponent(game.cur_player) == color::white)
        assert(game.is_white_king_checked == false);
    else if(opponent(game.cur_player) == color::black)
        assert(game.is_black_king_checked == false);
    else
        assert(false);
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
        case type::none:
            assert(false);
            break;
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

    for(auto move : cur_player_moves)
        assert(move.dst != move.src);

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
    for(struct game next_game : next_games(game))
    {
        if(is_king_captured(next_game))
            return true;
    }
    return false;
}

bool is_white_king_checked(struct game game)
{
    game.cur_player = color::black;
    return is_king_checked(game);
}

bool is_black_king_checked(struct game game)
{
    game.cur_player = color::white;
    return is_king_checked(game);
}
