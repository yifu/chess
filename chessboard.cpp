#include <iostream>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <thread>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "utils.hpp"
#include "game.hpp"
#include "net_protocol.hpp"

using namespace std;

bool quit = false;
int exit_result = EXIT_SUCCESS;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;

SDL_Surface *icon = nullptr;

constexpr Sint32 NETWORK_CODE = 1;
Uint32 custom_event_type;

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

int fd = -1;
enum color player_color;

size_t dragged_piece = -1;
SDL_MouseMotionEvent mouse;

bool operator != (SDL_Rect l, SDL_Rect r)
{
    return l.x != r.x || l.y != r.y || l.w != r.w || l.h != r.h;
}

struct square detect_square(Sint32 x, Sint32 y)
{
    struct square result;
    result.row = y / square_heigh;
    result.col = x / square_width;
    if(player_color == color::white)
    {
        result.row = 8 - result.row - 1;
        result.col = 8 - result.col - 1;
    }

    assert(result.row >= 0 && result.row <= 7);
    assert(result.col >= 0 && result.col <= 7);
    print_square(result);
    return result;
}

SDL_Rect square2rect(struct square square)
{
    assert(square.row >= 0 && square.row <= 7);
    assert(square.col >= 0 && square.col <= 7);
    if(player_color == color::white)
    {
        square.row = 8 - square.row - 1;
        square.col = 8 - square.col - 1;
    }

    SDL_Rect rect;
    rect.x = square.col * square_width;
    rect.y = square.row * square_heigh;
    rect.w = square_width;
    rect.h = square_heigh;
    return rect;
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
    printf("tv_sec=%lu, tv_nsec=%lu.\n", t.tv_sec, t.tv_nsec);
}

void print_mouse_motion(SDL_Event e)
{
    printf("e.motion={type=%u,timestamp=%u,winid=%u,which=%u,state=%u,x=%d,y=%d,xrel=%d,yrel=%d}\n",
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
    printf("e.button={type=%u,timestamp=%u,winid=%u,which=%u,button=%d,state=%u,clicks=%d,x=%d,y=%d}\n",
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

uint64_t substract_time(struct timespec l, struct timespec r)
{
    assert(l.tv_sec > r.tv_sec ||
           (l.tv_sec == r.tv_sec && l.tv_nsec > r.tv_nsec));
    assert(l.tv_nsec < 1000000000);
    assert(r.tv_nsec < 1000000000);

    uint64_t result = 0;
    if(r.tv_sec == r.tv_sec)
    {
        result = l.tv_nsec - r.tv_nsec;
    }
    else
    {
        assert(l.tv_sec > r.tv_sec);
        uint64_t sec = l.tv_sec - r.tv_sec;
        uint64_t nsec = 1000000000 * sec;
        nsec += l.tv_nsec;
        nsec += (1000000000 - r.tv_nsec);
        result = nsec;
    }
    return result;
}

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
}

void exit_success()
{
    clean_up();
    quit = true;
    exit_result = EXIT_SUCCESS;
}

void exit_failure()
{
    clean_up();
    quit = true;
    exit_result = EXIT_FAILURE;
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
        case type::pawn: return white_pawn_texture;
        case type::rook: return white_rook_texture;
        case type::knight: return white_knight_texture;
        case type::bishop: return white_bishop_texture;
        case type::queen: return white_queen_texture;
        case type::king: return white_king_texture;
        }
    }
    else
    {
        switch(piece.type)
        {
        case type::pawn: return black_pawn_texture;
        case type::rook: return black_rook_texture;
        case type::knight: return black_knight_texture;
        case type::bishop: return black_bishop_texture;
        case type::queen: return black_queen_texture;
        case type::king: return black_king_texture;
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

void send_move(struct move move)
{
    assert(fd != -1);
    struct move_msg move_msg;
    move_msg.msg_type = msg_type::move_msg;
    move_msg.src_row = move.src.row;
    move_msg.src_col = move.src.col;
    move_msg.dst_row = move.dst.row;
    move_msg.dst_col = move.dst.col;
    int n = send(fd, &move_msg, sizeof(move_msg), 0);
    if(n == -1)
    {
        perror("send()");
        exit_failure();
    }
}

void process_input_events(struct game& game)
{
    SDL_Event e;

    if(SDL_WaitEvent(&e))
    {
        switch(e.type)
        {
        case SDL_QUIT:
        {
            printf("quit\n");
            quit = true;
            break;
        }
        case SDL_KEYDOWN:
        {
            printf("key down %d\n", e.key.keysym.sym);
            if(e.key.keysym.sym == SDLK_ESCAPE)
                quit = true;
            break;
        }
        case SDL_MOUSEMOTION:
        {
            mouse = e.motion;
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            print_mouse_button_event(e);
            bool found = false;
            for(size_t i = 0; i < game.pieces.size(); i++)
            {
                SDL_Rect rect = square2rect(game.pieces[i].square);
                if(is_hitting_rect(rect, e.button.x, e.button.y))
                {
                    if(game.pieces[i].color != player_color)
                        continue;
                    if(game.cur_player == opponent(player_color))
                        continue;
                    if(game.pieces[i].is_captured)
                        continue;
                    found = true;
                    dragged_piece = i;
                }
            }
            printf("found = %d.\n", found);
            if(found)
            {
                printf("dragged_piece pos = %lu.\n", dragged_piece);
                print_piece(game.pieces[dragged_piece]);
            }
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            if(dragged_piece != (size_t)-1)
            {
                struct piece& piece = game.pieces[dragged_piece];
                struct square src = piece.square;
                struct square dst = detect_square(e.button.x, e.button.y);
                struct move candidate_move = {src, dst};

                // TODO Refactor the following. Must be moved into game.cpp file?

                vector<struct move> valid_moves = next_valid_moves(game);
                auto found = find(valid_moves.begin(), valid_moves.end(), candidate_move);
                if(found != valid_moves.end())
                {
                    // TODO We may overwrite with the candidate_game above.
                    game = apply_move(game, candidate_move);
                    send_move(candidate_move);

                    if(next_valid_moves(game).size() == 0)
                    {
                        if(is_king_checked(game))
                            printf("CHECKMATE!!\n");
                        else
                            printf("STALEMATE!!\n");
                    }
                }
                dragged_piece = -1;
            }
            break;
        }
        default:
        {
            // printf("e.type=%d\n", e.type);
            if(e.type == custom_event_type)
            {
                if(e.user.code != NETWORK_CODE)
                {
                    assert(false);
                    break;
                }
                char *buf = (char*)e.user.data1;
                enum msg_type type = (enum msg_type)buf[0];
                printf("msg type=%d.\n", type);
                if(type != msg_type::move_msg)
                {
                    printf("error!\n");
                    free(e.user.data1);
                    break;
                }
                struct move_msg msg = (*(struct move_msg*)buf);
                struct move move;
                move.src.row = msg.src_row;
                move.src.col = msg.src_col;
                move.dst.row = msg.dst_row;
                move.dst.col = msg.dst_col;
                game = apply_move(game, move);
                free(e.user.data1);
            }
            break;
        }
    }
}
}

void paint_chess_board()
{
    SDL_Rect dst;
    // TODO BIG REFACTORING. We must use the list from the game, and
    // re-generate the sprites from this list, at each turn of the
    // game loop.

    dst.x = dst.y = 0;
    dst.w = square_width;
    dst.h = square_heigh;

    for(int c = 0; c < 8; c++)
    {
        dst.x = 0;
        for(int r = 0; r < 8; r++)
        {
            if((c % 2 == 0 && r % 2 == 0) || (c % 2 == 1 && r % 2 == 1))
                SDL_SetRenderDrawColor(ren, 200, 200, 216, SDL_ALPHA_OPAQUE); // White square
            else
                SDL_SetRenderDrawColor(ren, 100, 100, 140, SDL_ALPHA_OPAQUE); // Dark square

            // printf("dst.x=%d, dst.y=%d, dst.w=%d, dst.h=%d.\n",
            //        dst.x, dst.y, dst.w, dst.h);
            SDL_RenderFillRect(ren, &dst);
            dst.x += dst.w;
        }
        dst.y += dst.h;
    }
}

void paint_sprites(const struct game& game)
{
    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        struct piece piece = game.pieces[i];

        if(piece.is_captured)
            continue;

        if(i == dragged_piece)
            continue;

        SDL_Texture *texture = deduct_texture(piece);
        SDL_Rect rect = square2rect(piece.square);
        SDL_RenderCopy(ren, texture, nullptr, &rect);
    }

    if(dragged_piece != (size_t)-1)
    {
        struct piece piece = game.pieces[dragged_piece];

        assert(!piece.is_captured);

        SDL_Texture *texture = deduct_texture(piece);
        SDL_Rect rect;
        rect.x = mouse.x - square_width / 2;
        rect.y = mouse.y - square_heigh / 2;
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

void paint_screen(const struct game& game)
{
    assert(display);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    paint_chess_board();
    paint_sprites(game);

    SDL_RenderPresent(ren);
    SDL_UpdateWindowSurface(display);
}

void init_sdl()
{
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 640;

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
    custom_event_type = SDL_RegisterEvents(1);
    if(custom_event_type == (Uint32)-1)
    {
        printf("SDL_RegisterEvents() error : %s.\n", SDL_GetError());
        exit_failure();
    }
}

void init_network()
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
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
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(55555);
    addr.sin_addr.s_addr = INADDR_ANY;

    // TODO Handle SIGFPIPE.
    res = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(res == -1)
    {
        perror("connect()");
        exit_failure();
    }

    struct login login;
    login.msg_type = msg_type::login;
    strncpy(login.username, "yves", sizeof(login.username));
    res = send(fd, &login, sizeof(login), 0);
    if(res == -1)
    {
        perror("send()");
        exit_failure();
    }

    struct login_ack login_ack;
    int n = recv(fd, &login_ack, sizeof(login_ack), 0);
    if(n == -1)
    {
        perror("recv()");
        exit_failure();
    }
    printf("login_ack={player_color=%d}\n", login_ack.player_color);
    fflush(stdout);
    player_color = login_ack.player_color;
}

void network_thread()
{
    init_network();
    while(!quit)
    {
        char buf[1024];
        int n = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
        if(n < 0)
        {
            if(errno != EAGAIN && errno != EWOULDBLOCK)
            {
                printf("recv");
                perror("recv()");
                exit_failure();
            }
        }
        else
        {
            char *data = (char*)malloc(n);
            if(data == nullptr)
            {
                perror("malloc()");
                exit_failure();
            }
            else
            {
                memcpy(data, buf, n);
                SDL_Event event;
                memset(&event, 0, sizeof(event));
                event.type = custom_event_type;
                event.user.code = NETWORK_CODE;
                event.user.data1 = data;
                SDL_PushEvent(&event);
            }
        }
    }
}

int main()
{
    init_sdl();
    thread t(network_thread);

    struct game game;
    game.pieces = initial_board;

    while(!quit)
    {
        process_input_events(game);

        paint_screen(game);
    }
    printf("bye!\n");

    t.join();
    clean_up();
}
