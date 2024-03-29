#include <iostream>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <thread>
#include <poll.h>
#include <sstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "utils.hpp"
#include "game.hpp"
#include "net_protocol.hpp"

extern FILE *dbg;

using namespace std;

constexpr int screenwidth = 640;
constexpr int screenheigh = 640;

bool check_for_valid_moves = true;
struct game last_game;

bool quit = false;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;

SDL_Surface *icon = nullptr;

SDL_Texture *white_pawn_texture = nullptr;
SDL_Texture *white_rook_texture = nullptr;
SDL_Texture *white_knight_texture = nullptr;
SDL_Texture *white_bishop_texture = nullptr;
SDL_Texture *white_queen_texture = nullptr;
SDL_Texture *white_king_texture = nullptr;

SDL_Texture *black_pawn_texture = nullptr;
SDL_Texture *black_rook_texture = nullptr;
SDL_Texture *black_knight_texture = nullptr;
SDL_Texture *black_bishop_texture = nullptr;
SDL_Texture *black_queen_texture = nullptr;
SDL_Texture *black_king_texture = nullptr;

int square_width, square_heigh;

SDL_Rect out_of_view_rect = { -square_width, -square_heigh, square_width, square_heigh };

enum color player_color;

size_t dragged_piece = -1;
struct animated_sprite
{
    size_t pos = -1;
    SDL_Rect cur, src, dst;
    struct timespec begin, end;
} anim_sprite;

Sint32 mouse_x = 0, mouse_y = 0;

enum class screen_type
{
    menu,
    game,
    players_list,
};

enum screen_type scr_type = screen_type::menu;

void clean_up()
{
    if(icon)
        SDL_FreeSurface(icon);
    if(white_pawn_texture)
        SDL_DestroyTexture(white_pawn_texture);
    if(white_rook_texture)
        SDL_DestroyTexture(white_rook_texture);
    if(white_knight_texture)
        SDL_DestroyTexture(white_knight_texture);
    if(white_bishop_texture)
        SDL_DestroyTexture(white_bishop_texture);
    if(white_queen_texture)
        SDL_DestroyTexture(white_queen_texture);
    if(white_king_texture)
        SDL_DestroyTexture(white_king_texture);

    if(black_pawn_texture)
        SDL_DestroyTexture(black_pawn_texture);
    if(black_rook_texture)
        SDL_DestroyTexture(black_rook_texture);
    if(black_knight_texture)
        SDL_DestroyTexture(black_knight_texture);
    if(black_bishop_texture)
        SDL_DestroyTexture(black_bishop_texture);
    if(black_queen_texture)
        SDL_DestroyTexture(black_queen_texture);
    if(black_king_texture)
        SDL_DestroyTexture(black_king_texture);

    if(ren)
        SDL_DestroyRenderer(ren);

    if(display)
        SDL_DestroyWindow(display);
    SDL_Quit();
    IMG_Quit();
    TTF_Quit();
}

void exit_failure()
{
    clean_up();
    exit(EXIT_FAILURE);
}

struct menu_item
{
    string label;
    SDL_Rect rect;
    SDL_Texture *texture;
    SDL_Texture *hl_texture;

    menu_item(string label);
private:
    SDL_Texture *create_texture(string label, SDL_Color color);
};

menu_item::menu_item(string l) :
    label(l)
{
    texture = create_texture(label, {255,255,255,0});
    hl_texture = create_texture(label, {255,0,0,0});

    if(SDL_QueryTexture(texture, nullptr, nullptr,
                        &rect.w, &rect.h) == -1)
    {
        fprintf(stderr, "SDL_QueryTexture(): %s.\n", SDL_GetError());
        exit_failure();
    }

    rect.x = screenwidth/2 - rect.w/2;
    rect.y = screenheigh/2 - rect.h/2;
}

SDL_Texture *menu_item::create_texture(string label, SDL_Color color)
{
    // TODO Make this parametrable. Or we should be able to get the
    // list of installed font. There might be a specific debian
    // package to install as a dependency. 2- The point size must be
    // adequate: how find to we find a correct value here?
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 40);
    if(!font)
    {
        fprintf(stderr, "TTF_OpenFont(): %s.\n", TTF_GetError());
        exit_failure();
    }

    SDL_Surface *text_surface = TTF_RenderText_Blended(font, label.c_str(), color);
    if(!text_surface)
    {
        fprintf(stderr, "TTF_RenderText_Solid(): %s.\n", TTF_GetError());
        exit_failure();
    }

    TTF_CloseFont(font);
    font = nullptr;

    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(ren, text_surface);
    if(!text_texture)
    {
        fprintf(stderr, "SDL_CreateTextureFromSurface(): %s.\n", SDL_GetError());
        exit_failure();
    }

    SDL_FreeSurface(text_surface);
    return text_texture;
}

vector<struct menu_item> menu_items;

vector<string> players_list;

bool operator != (SDL_Rect l, SDL_Rect r)
{
    return l.x != r.x || l.y != r.y || l.w != r.w || l.h != r.h;
}

bool operator > (struct timespec l, struct timespec r)
{
    return l.tv_sec > r.tv_sec || ((l.tv_sec == r.tv_sec) && (l.tv_nsec > r.tv_nsec));
}

SDL_Rect square2rect(struct square square)
{
    assert(square.row >= 0 && square.row <= 7);
    assert(square.col >= 0 && square.col <= 7);
    if(player_color == color::white)
    {
        square.row = 8 - square.row - 1;
    }

    SDL_Rect rect;
    rect.x = square.col * square_width;
    rect.y = square.row * square_heigh;
    rect.w = square_width;
    rect.h = square_heigh;
    return rect;
}

struct square detect_square(Sint32 x, Sint32 y)
{
    for(int i = 0; i < 8; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            struct square square = {i,j};
            SDL_Rect rect = square2rect(square);
            if(is_hitting_rect(rect, x, y))
            {
                // print_square(square);
                return square;
            }
        }
    }

    assert(false);
    struct square result = {-1,-1};
    // print_square(result);
    return result;
}

struct sprite
{
    SDL_Texture *tex = nullptr;
    SDL_Rect rect = {0,0,0,0};
    bool is_dragged = false;
    size_t piece_pos = -1;
};

void print_timespec(struct timespec t)
{
    fprintf(dbg, "tv_sec=%lu, tv_nsec=%lu.\n", t.tv_sec, t.tv_nsec);
}

void print_mouse_motion(SDL_Event e)
{
    fprintf(dbg, "e.motion={type=%u,timestamp=%u,winid=%u,which=%u,state=%u,x=%d,y=%d,xrel=%d,yrel=%d}\n",
           e.motion.type,
           e.motion.timestamp,
           e.motion.windowID,
           e.motion.which,
           e.motion.state,
           e.motion.x,
           e.motion.y,
           e.motion.xrel,
           e.motion.yrel);
}

void print_mouse_button_event(SDL_Event e)
{
    fprintf(dbg, "e.button={type=%u,timestamp=%u,winid=%u,which=%u,button=%d,state=%u,clicks=%d,x=%d,y=%d}\n",
           e.button.type,
           e.button.timestamp,
           e.button.windowID,
           e.button.which,
           e.button.button,
           e.button.state,
           e.button.clicks,
           e.button.x,
           e.button.y);
}

SDL_Texture *init_texture(string filename)
{
    SDL_Surface *surface = IMG_Load(filename.c_str());
    if(!surface) {
        cerr << "IMG_Load() error : " << IMG_GetError() << endl;
        exit_failure();
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);
    if(!texture){
        cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
        exit_failure();
    }
    SDL_FreeSurface(surface);
    return texture;
}

void init_textures()
{
    white_pawn_texture = init_texture("./Chess_plt60.png");
    white_rook_texture = init_texture("./Chess_rlt60.png");
    white_knight_texture = init_texture("./Chess_nlt60.png");
    white_bishop_texture = init_texture("./Chess_blt60.png");
    white_queen_texture =  init_texture("./Chess_qlt60.png");
    white_king_texture = init_texture("./Chess_klt60.png");

    black_pawn_texture = init_texture("./Chess_pdt60.png");
    black_rook_texture = init_texture("./Chess_rdt60.png");
    black_knight_texture = init_texture("./Chess_ndt60.png");
    black_bishop_texture = init_texture("./Chess_bdt60.png");
    black_queen_texture =  init_texture("./Chess_qdt60.png");
    black_king_texture = init_texture("./Chess_kdt60.png");
}

SDL_Texture *deduct_texture(struct piece piece)
{
    if(piece.color == color::white)
    {
        switch(piece.type)
        {
        case piece_type::none: assert(false); break;
        case piece_type::pawn: return white_pawn_texture;
        case piece_type::rook: return white_rook_texture;
        case piece_type::knight: return white_knight_texture;
        case piece_type::bishop: return white_bishop_texture;
        case piece_type::queen: return white_queen_texture;
        case piece_type::king: return white_king_texture;
        }
    }
    else
    {
        switch(piece.type)
        {
        case piece_type::none: assert(false); break;
        case piece_type::pawn: return black_pawn_texture;
        case piece_type::rook: return black_rook_texture;
        case piece_type::knight: return black_knight_texture;
        case piece_type::bishop: return black_bishop_texture;
        case piece_type::queen: return black_queen_texture;
        case piece_type::king: return black_king_texture;
        }
    }
    assert(false);
    return nullptr;
}

struct sprite init_sprite(struct piece piece)
{
    struct sprite sprite;
    sprite.tex = deduct_texture(piece);
    sprite.rect = piece.is_captured ? out_of_view_rect : square2rect(piece.square);
    // print_rect(sprite.rect);
    return sprite;
}

void send_move(struct move move, int fd)
{
    assert(fd != -1);
    struct move_msg move_msg;
    move_msg.msg_type = msg_type::move_msg;
    move_msg.src_row = move.src.row;
    move_msg.src_col = move.src.col;
    move_msg.dst_row = move.dst.row;
    move_msg.dst_col = move.dst.col;
    ssize_t n = write(fd, &move_msg, sizeof(move_msg));
    if(n == -1)
    {
        perror("write()");
        exit_failure();
    }
}

void process_sdl_mousebuttondown(SDL_Event& e, struct game& game, int fd)
{
    // print_mouse_button_event(e);
    assert(fd != -1);
    if(scr_type == screen_type::menu)
    {
        // fprintf(dbg, "process_sdl_mousebuttondown(): menu.\n");
        for(auto menu_item : menu_items)
        {
            if(is_hitting_rect(menu_item.rect, e.button.x, e.button.y))
            {
                if(menu_item.label == "play online")
                {
                    struct play_online_msg msg;
                    msg.msg_type = msg_type::play_online;
                    ssize_t n = write(fd, &msg, sizeof(msg));
                    if(n == -1)
                    {
                        perror("write()");
                        exit_failure();
                    }
                }
            }
        }
        return;
    }
    else if(scr_type == screen_type::players_list)
    {
        fprintf(dbg, "process_sdl_mousebuttondown(): player list.\n");
        for(auto menu_item : menu_items)
        {
            if(is_hitting_rect(menu_item.rect, e.button.x, e.button.y))
            {
                fprintf(dbg, "send request for game to %s.", menu_item.label.c_str());
                struct request_for_game msg;
                msg.msg_type = msg_type::request_for_game;
                strcpy(msg.opponent_username, menu_item.label.c_str());
                ssize_t n = write(fd, &msg, sizeof(msg));
                if(n == -1)
                {
                    perror("write()");
                    exit_failure();
                }
            }
        }
    }

    if(game.cur_player == opponent(player_color))
        return;

    bool found = false;
    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        SDL_Rect rect = square2rect(game.pieces[i].square);
        if(is_hitting_rect(rect, e.button.x, e.button.y))
        {
            if(game.pieces[i].color != player_color)
                continue;
            if(game.pieces[i].is_captured)
                continue;
            found = true;
            dragged_piece = i;
            mouse_x = e.button.x;
            mouse_y = e.button.y;
        }
    }
    if(found)
    {
        // fprintf(dbg, "process_sdl_mousebuttondown: dragged_piece pos = %lu.\n", dragged_piece);
        print_piece(game.pieces[dragged_piece]);
    }
}

void process_sdl_mousebuttonup(SDL_Event& e, struct game& game, int fd)
{
    if(scr_type == screen_type::menu)
        return;

    if(dragged_piece != (size_t)-1)
    {
        struct piece& piece = game.pieces[dragged_piece];
        struct square src = piece.square;
        struct square dst = detect_square(e.button.x, e.button.y);
        struct move candidate_move = {src, dst, piece_type::none};

        // TODO Refactor the following. Must be moved into game.cpp file?

        if(check_for_valid_moves)
        {
            vector<struct move> valid_moves = next_valid_moves(game);
            fprintf(dbg, "CANDIDATE MOVE IS :\n");
            print_move(candidate_move);
            fprintf(dbg, "VALID MOVES BEGIN\n");
            bool found = false;
            // TODO When implementing a real promotion with choice
            // from the user, then fix that.
            for(auto move : valid_moves)
            {
                print_move(move);
                if(move.src != candidate_move.src)
                    continue;
                if(move.dst != candidate_move.dst)
                    continue;

                // TODO FIX THIS.
                candidate_move = move;

                found = true;
            }
            fprintf(dbg, "VALID MOVES END\n");

            // auto found = find(valid_moves.begin(), valid_moves.end(), candidate_move);

            if(found)
                fprintf(dbg, "CANDIDATE MOVE DOES NOT MATCH ANY VALID MOVES.\n");

            if(found)
            {
                fprintf(dbg, "APPLY MOVE.\n");
                print_move(candidate_move);
                game = apply_move(game, candidate_move);
                update_king_statuses(game);
                fprintf(dbg, "APPLY MOVE - AFTER.\n");
                send_move(candidate_move, fd);

                if(next_valid_moves(game).size() == 0)
                {
                    if(game.cur_player == color::white && game.is_white_king_checked)
                        fprintf(dbg, "WHITE IS CHECKMATED!!\n");
                    else if(game.cur_player == color::black && game.is_black_king_checked)
                        fprintf(dbg, "BLACK IS CHECKMATED!!\n");
                    else
                        fprintf(dbg, "STALEMATE!!\n");
                }
            }
        }
        else
        {
            last_game = game;
            game = apply_move(game, candidate_move);
            send_move(candidate_move, fd);
        }
        dragged_piece = -1;
        mouse_x = e.button.x;
        mouse_y = e.button.y;
    }

}

void process_input_events(SDL_Event& e, struct game& game, int fd)
{
    // fprintf(dbg, "Input Events!\n");
    switch(e.type)
    {
    case SDL_QUIT:
    {
        fprintf(dbg, "quit\n");
        quit = true;
        break;
    }
    case SDL_KEYDOWN:
    {
        fprintf(dbg, "key down %d\n", e.key.keysym.sym);
        if(e.key.keysym.sym == SDLK_ESCAPE)
        {
            scr_type = screen_type::menu;
            fprintf(dbg, "scr_type = %d.\n", scr_type);
        }
        break;
    }
    case SDL_MOUSEMOTION:
    {
        mouse_x = e.motion.x;
        mouse_y = e.motion.y;
        break;
    }
    case SDL_MOUSEBUTTONDOWN:
    {
        process_sdl_mousebuttondown(e, game, fd);
        break;
    }
    case SDL_MOUSEBUTTONUP:
    {
        process_sdl_mousebuttonup(e, game, fd);
        break;
    }
    default:
    {
        fprintf(dbg, "e.type=%d\n", e.type);
        break;
    }
    }
}

void paint_chess_board()
{
    SDL_Rect dst;
    dst.x = dst.y = 0;
    dst.w = square_width;
    dst.h = square_heigh;

    for(int col = 0; col < 8; col++)
    {
        dst.x = 0;
        for(int row = 0; row < 8; row++)
        {
            Uint8 r,g,b;
            if((col % 2 == 0 && row % 2 == 0) || (col % 2 == 1 && row % 2 == 1))
            {
                // White square
                r = 200;
                g = 200;
                b = 216;
            }
            else
            {
                // Dark square
                r = 100;
                g = 100;
                b = 140;
            }

            if(scr_type != screen_type::game)
            {
                r *= (100/255.0);
                g *= (100/255.0);
                b *= (100/255.0);
            }

            SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE);

            // fprintf(dbg, "dst.x=%d, dst.y=%d, dst.w=%d, dst.h=%d.\n",
            //        dst.x, dst.y, dst.w, dst.h);
            SDL_RenderFillRect(ren, &dst);
            dst.x += dst.w;
        }
        dst.y += dst.h;
    }
}

void paint_sprites(const struct game& game)
{
    struct timespec curtime;
    clock_gettime(CLOCK_MONOTONIC, &curtime);

    if(curtime > anim_sprite.end)
    {
        // print_timespec(curtime);
        // print_timespec(anim_sprite.end);
        // fprintf(dbg, "End animation.\n");
        anim_sprite.pos = -1;
    }

    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        struct piece piece = game.pieces[i];

        if(piece.is_captured)
            continue;

        if(i == dragged_piece)
            continue;

        if(i == anim_sprite.pos)
            continue;

        SDL_Texture *texture = deduct_texture(piece);
        Uint8 r,g,b;
        if(scr_type != screen_type::game)
            r = g = b = 100;
        else
            r = g = b = 255;

        if(SDL_SetTextureColorMod(texture, r, g, b) == -1)
        {
            fprintf(stderr, "SDL_SetTextureColorMod: %s.\n", SDL_GetError());
            exit_failure();
        }

        SDL_Rect rect = square2rect(piece.square);
        SDL_RenderCopy(ren, texture, nullptr, &rect);
    }

    if(anim_sprite.pos != (size_t)-1)
    {
        struct piece piece = game.pieces[anim_sprite.pos];
        assert(!piece.is_captured);
        // fprintf(dbg, "Paint the animated sprite.\n");
        // print_rect(anim_sprite.cur);

        int dx = (anim_sprite.dst.x > anim_sprite.src.x)? anim_sprite.dst.x-anim_sprite.src.x : anim_sprite.src.x-anim_sprite.dst.x;
        int dy = (anim_sprite.dst.y > anim_sprite.src.y)? anim_sprite.dst.y-anim_sprite.src.y : anim_sprite.src.y-anim_sprite.dst.y;
        int x = to_uint64(curtime - anim_sprite.begin) * dx / to_uint64(anim_sprite.end - anim_sprite.begin);
        int y = to_uint64(curtime - anim_sprite.begin) * dy / to_uint64(anim_sprite.end - anim_sprite.begin);
        x = ((anim_sprite.dst.x > anim_sprite.src.x)?x:-x);
        y = ((anim_sprite.dst.y > anim_sprite.src.y)?y:-y);
        anim_sprite.cur.x = anim_sprite.src.x+x;
        anim_sprite.cur.y = anim_sprite.src.y+y;

        SDL_Texture *texture = deduct_texture(piece);
        SDL_RenderCopy(ren, texture, nullptr, &anim_sprite.cur);
    }

    if(dragged_piece != (size_t)-1)
    {
        struct piece piece = game.pieces[dragged_piece];
        assert(!piece.is_captured);

        SDL_Texture *texture = deduct_texture(piece);
        SDL_Rect rect;
        rect.x = mouse_x - square_width / 2;
        rect.y = mouse_y - square_heigh / 2;
        rect.w = square_width;
        rect.h = square_heigh;
        SDL_RenderCopy(ren, texture, nullptr, &rect);
    }
}

void initWindowIcon()
{
    icon = IMG_Load("./Chess_ndt60.png");
    if(!icon) {
        cerr << "IMG_Load() error : " << IMG_GetError() << endl;
        exit_failure();
    }
    SDL_SetWindowIcon(display, icon);
}

void paint_menu()
{
    switch(scr_type)
    {
    case screen_type::menu:
    case screen_type::players_list:
        // fprintf(dbg, "Paint menu.\n");
        for(auto menu_item : menu_items)
        {
            SDL_Texture *text_texture = nullptr;
            if(is_hitting_rect(menu_item.rect, mouse_x, mouse_y))
                text_texture = menu_item.hl_texture;
            else
                text_texture = menu_item.texture;

            assert(text_texture);

            if(SDL_RenderCopy(ren, text_texture, NULL, &menu_item.rect) == -1)
            {
                fprintf(stderr, "SDL_RenderCopy(): %s.\n", SDL_GetError());
                exit_failure();
            }
        }
        break;
    case screen_type::game:
        break;
    default:
        fprintf(dbg, "Not implemented yet.");
        break;
    }
}

void paint_screen(const struct game& game)
{
    assert(display);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    paint_chess_board();
    paint_sprites(game);
    paint_menu();

    SDL_RenderPresent(ren);
}

void init_font()
{
    if(TTF_Init() == -1)
    {
        fprintf(stderr, "TTF_Init(): %s.\n", TTF_GetError());
        exit_failure();
    }
}

void init_menu_item()
{
    struct menu_item play_online_item("play online");
    menu_items.push_back(play_online_item);
}

void init_sdl()
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cerr << "SDL_Init error: " << SDL_GetError() << "." << endl;
        exit_failure();
    }

    display = SDL_CreateWindow("Chess",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               screenwidth, screenheigh, SDL_WINDOW_HIDDEN);
    if(!display) {
        cerr << "SDL_CreateWindow() error : " << SDL_GetError() << "." << endl;
        exit_failure();
    }

    constexpr Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    ren = SDL_CreateRenderer(display, -1/*index*/, renderer_flags);
    if(!ren) {
        cerr << "SDL_CreateRenderer() error :" << SDL_GetError() << "." << endl;
        exit_failure();
    }

    SDL_Rect viewport;
    SDL_RenderGetViewport(ren, &viewport);
    square_width = viewport.w / 8;
    square_heigh = viewport.h / 8;

    initWindowIcon();
    SDL_ShowWindow(display);

    init_textures();

    SDL_EventState(SDL_WINDOWEVENT, SDL_DISABLE);
}

int init_network(string ip, string port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket()");
        exit_failure();
    }

    int optval = 1;
    int res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                         &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(TCP_NODELAY)");
        exit_failure();
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port.c_str()));
    res = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if(res != 1)
    {
        perror("inet_pton()");
        exit_failure();
    }

    // TODO Handle SIGFPIPE.
    res = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(res == -1)
    {
        perror("connect()");
        exit_failure();
    }

    string username = "yves";
    username += to_string(getpid());

    struct login login;
    login.msg_type = msg_type::login;
    strncpy(login.username, username.c_str(), sizeof(login.username));
    res = send(fd, &login, sizeof(login), 0);
    if(res == -1)
    {
        perror("send()");
        exit_failure();
    }

    // TODO Move that recv() call into process_server_fd().
    struct login_ack login_ack;
    int n = recv(fd, &login_ack, sizeof(login_ack), 0);
    if(n == -1)
    {
        perror("recv()");
        exit_failure();
    }

    return fd;
}

void print_poll_events(short events)
{
    if(events & POLLIN) fprintf(dbg, "POLLIN|");
    if(events & POLLPRI) fprintf(dbg, "POLLPRI|");
    if(events & POLLOUT) fprintf(dbg, "POLLOUT|");
    if(events & POLLRDHUP) fprintf(dbg, "POLLRDHUP|");
    if(events & POLLERR) fprintf(dbg, "POLLERR|");
    if(events & POLLHUP) fprintf(dbg, "POLLHUP|");
    if(events & POLLNVAL) fprintf(dbg, "POLLNVAL|");
    if(events & POLLRDNORM) fprintf(dbg, "POLLRDNORM|");
    if(events & POLLRDBAND) fprintf(dbg, "POLLRDBAND|");
    if(events & POLLWRNORM) fprintf(dbg, "POLLWRNORM|");
    if(events & POLLWRBAND) fprintf(dbg, "POLLWRBAND|");
}

void print_pollfd(struct pollfd pollfd)
{
    fprintf(dbg, "pollfd={fd=%d,events=", pollfd.fd);
    print_poll_events(pollfd.events);
    fprintf(dbg, ",revents=");
    print_poll_events(pollfd.revents);
    fprintf(dbg, "}\n");
}

void process_sdl_evt_fd(struct pollfd pollfd, int fd, struct game& game)
{
    // fprintf(dbg, "controller thread: read pipe read end.\n");
    SDL_Event e;
    ssize_t n = read(pollfd.fd, &e, sizeof(e));
    if(n == -1)
    {
        perror("read()");
        exit_failure();
    }
    else if(n == 0)
    {
        fprintf(dbg, "controller thread: nothing to read from the sdl evt thread.\n");
        close(pollfd.fd);
        quit = true;
        return;
    }
    else if(n != sizeof(e))
    {
        fprintf(dbg, "controller thread: received %lu, instead of %lu.\n",
               n, sizeof(e));
    }

    process_input_events(e, game, fd);
}

void process_server_fd(struct pollfd pollfd, struct game& game)
{
    // fprintf(dbg, "controller thread: read socket.\n");
    char buf[1024];
    int n = recv(pollfd.fd, buf, sizeof(buf), 0);
    if(n < 0)
    {
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("recv()");
            exit_failure();
        }
    }
    else
    {
        if(n == 0)
        {
            fprintf(dbg, "controller thread: socket closed.\n");
            close(pollfd.fd);
            quit = true;
        }
        else
        {
            enum msg_type msgtype = (enum msg_type)buf[0];
            fprintf(dbg, "msg type=%d, len=%d.\n", msgtype, n);
            switch(msgtype)
            {
            case msg_type::move_msg:
            {
                struct move_msg msg = *(struct move_msg*)buf;
                assert(n == sizeof(msg));
                struct move move;
                move.src.row = msg.src_row;
                move.src.col = msg.src_col;
                move.dst.row = msg.dst_row;
                move.dst.col = msg.dst_col;
                move.promotion = piece_type::none;

                anim_sprite.pos = -1;
                for(size_t i = 0; i < game.pieces.size(); i++)
                {
                    if(game.pieces[i].square == move.src)
                    {
                        if(game.pieces[i].is_captured)
                            continue;
                        anim_sprite.pos = i;
                    }
                }
                assert(anim_sprite.pos != (size_t)-1);

                anim_sprite.cur = square2rect(move.src);
                anim_sprite.src = square2rect(move.src);
                anim_sprite.dst = square2rect(move.dst);

                struct timespec begin;
                clock_gettime(CLOCK_MONOTONIC, &begin);
                anim_sprite.begin = begin;
                struct timespec duration = {0,100000000};
                anim_sprite.end = begin+duration;

                game = apply_move(game, move);
                update_king_statuses(game);
                if(next_valid_moves(game).size() == 0)
                {
                    if(game.cur_player == color::white && game.is_white_king_checked)
                        fprintf(dbg, "WHITE IS CHECKMATED!!\n");
                    else if(game.cur_player == color::black && game.is_black_king_checked)
                        fprintf(dbg, "BLACK IS CHECKMATED!!\n");
                    else
                        fprintf(dbg, "STALEMATE!!\n");
                }
                break;
            }
            case msg_type::reject_move_msg:
            {
                fprintf(dbg, "rejected msg.\n");
                assert(n == sizeof(struct reject_move_msg));
                game = last_game;
                break;
            }
            case msg_type::new_game_msg:
            {
                struct new_game_msg msg = *(struct new_game_msg*)buf;
                assert(n == sizeof(msg));
                fprintf(dbg, "new_game_msg={player_color=%d}\n", msg.player_color);
                fflush(stdout);
                struct game new_game;
                game = new_game;
                game.pieces = initial_board;
                player_color = msg.player_color;
                scr_type = screen_type::game;
                break;
            }
            case msg_type::game_evt_msg:
            {
                fprintf(dbg, "Opponent resigned.\n");
                break;
            }
            case msg_type::players:
            {
                struct players_list_msg msg = *(struct players_list_msg*)buf;
                assert(n == sizeof(msg));
                // fprintf(dbg, "players = [%s].\n", msg.players_list);
                // fflush(stdout);

                istringstream iss(msg.players_list);
                string username;
                while(getline(iss, username, ','))
                    players_list.push_back(username);

                menu_items.clear();
                for(size_t i = 0; i < players_list.size(); i++)
                {
                    // TODO Remove the players_list.
                    string username = players_list[i];
                    struct menu_item menu_item(username);
                    // rect.x = screenwidth/2 - rect.w/2;
                    menu_item.rect.y = (screenheigh/2 - menu_item.rect.h/2)+(i*menu_item.rect.h);
                    menu_items.push_back(menu_item);
                }

                scr_type = screen_type::players_list;
                break;
            }
            default:
            {
                assert(false);
                break;
            }
            }
        }
    }
}

int compute_timeout()
{
    if(anim_sprite.pos == (size_t)-1)
        return -1;
    return 1;
}

void controller_thread(string ip, string port, int sdl_evt_fd)
{
    init_sdl();
    init_font();
    init_menu_item();

    struct game game;
    game.pieces = initial_board;
    paint_screen(game);

    int fd = init_network(ip, port);

    while(!quit)
    {
        struct pollfd fds[2] = {{sdl_evt_fd, POLLIN, 0},{fd, POLLIN, 0}};
        int timeout = compute_timeout();
        int nfds = poll(&fds[0], arraysize(fds), timeout);

        struct timespec beg;
        clock_gettime(CLOCK_MONOTONIC, &beg);

        if(nfds == -1)
        {
            perror("poll()");
            exit_failure();
        }
        if(nfds == 0)
        {
            // Timeout.
        }

        // Refactor this for loop into process_all_fd().
        for(size_t i = 0; i < arraysize(fds); i++)
        {
            struct pollfd pollfd = fds[i];
            if(pollfd.revents == 0)
            {
                continue;
            }
            if(pollfd.revents & (POLLIN|POLLHUP))
            {
                if(pollfd.fd == sdl_evt_fd)
                    process_sdl_evt_fd(pollfd, fd, game);
                else if(pollfd.fd == fd)
                    process_server_fd(pollfd, game);
                else
                    assert(false);
            }
            else
            {
                fprintf(dbg, "controller thread: Un-processed event: ");
                print_pollfd(pollfd);
            }
        }

        paint_screen(game);
        fflush(dbg);

        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        // fprintf(dbg, "Total time = %" PRIu64 ".\n", to_uint64(end-beg));
    }
    // TODO We must try to re-connect to the server.
    fprintf(dbg, "controller thread: bye!\n");
    clean_up();
}

void push_evt_into_controller_thread(SDL_Event& e, int fd)
{
    ssize_t n = write(fd, &e, sizeof(e));
    if(n == -1)
    {
        perror("write()");
        exit_failure();
    }
    else if(n != sizeof(e))
    {
        fprintf(dbg, "SDL Thread: error, write() wrote %lu bytes instead of %lu\n",
               n, sizeof(e));
        assert(false);
        exit_failure();
    }
}

void sdl_evt_thread(int event_pipe)
{
    SDL_Event e;
    while(SDL_WaitEvent(&e))
    {
        push_evt_into_controller_thread(e, event_pipe);
        if(e.type == SDL_QUIT)
            break;
    }
    fprintf(dbg, "sdl evt thread: bye!\n");
}

int main()
{
    printf("PID=%d.\n", getpid());

    int pipefd[2] = {-1,-1};
    int res = pipe(pipefd);
    if(res == -1)
    {
        perror("pipe()");
        exit_failure();
    }

    thread t(sdl_evt_thread, pipefd[1]);

    controller_thread("195.154.72.36", "55555", pipefd[0]);
    fprintf(dbg, "bye!\n");

    close(pipefd[1]);
    t.join();
}
