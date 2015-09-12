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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "utils.hpp"
#include "game.hpp"
#include "net_protocol.hpp"

using namespace std;

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
Sint32 mouse_x = 0, mouse_y = 0;

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

void exit_failure()
{
    clean_up();
    exit(EXIT_FAILURE);
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

void process_input_events(SDL_Event& e, struct game& game, int fd)
{
    // printf("Input Events!\n");
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
                mouse_x = e.button.x;
                mouse_y = e.button.y;
                SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
            }
        }
        // printf("found = %d.\n", found);
        if(found)
        {
            // printf("dragged_piece pos = %lu.\n", dragged_piece);
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

            if(check_for_valid_moves)
            {
                vector<struct move> valid_moves = next_valid_moves(game);
                auto found = find(valid_moves.begin(), valid_moves.end(), candidate_move);
                if(found != valid_moves.end())
                {
                    game = apply_move(game, candidate_move);
                    send_move(candidate_move, fd);

                    if(next_valid_moves(game).size() == 0)
                    {
                        if(is_king_checked(game))
                            printf("CHECKMATE!!\n");
                        else
                            printf("STALEMATE!!\n");
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
            SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
        }
        break;
    }
    default:
    {
        printf("e.type=%d\n", e.type);
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

    SDL_EventState(SDL_WINDOWEVENT, SDL_DISABLE);
    SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
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

    struct login login;
    login.msg_type = msg_type::login;
    strncpy(login.username, "yves", sizeof(login.username));
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
    if(events & POLLIN) printf("POLLIN|");
    if(events & POLLPRI) printf("POLLPRI|");
    if(events & POLLOUT) printf("POLLOUT|");
    if(events & POLLRDHUP) printf("POLLRDHUP|");
    if(events & POLLERR) printf("POLLERR|");
    if(events & POLLHUP) printf("POLLHUP|");
    if(events & POLLNVAL) printf("POLLNVAL|");
    if(events & POLLRDNORM) printf("POLLRDNORM|");
    if(events & POLLRDBAND) printf("POLLRDBAND|");
    if(events & POLLWRNORM) printf("POLLWRNORM|");
    if(events & POLLWRBAND) printf("POLLWRBAND|");
}

void print_pollfd(struct pollfd pollfd)
{
    printf("pollfd={fd=%d,events=", pollfd.fd);
    print_poll_events(pollfd.events);
    printf(",revents=");
    print_poll_events(pollfd.revents);
    printf("}\n");
}

void process_sdl_evt_fd(struct pollfd pollfd, int fd, struct game& game)
{
    // printf("controller thread: read pipe read end.\n");
    SDL_Event e;
    ssize_t n = read(pollfd.fd, &e, sizeof(e));
    if(n == -1)
    {
        perror("read()");
        exit_failure();
    }
    else if(n == 0)
    {
        printf("controller thread: nothing to read from the sdl evt thread.\n");
        close(pollfd.fd);
        quit = true;
        return;
    }
    else if(n != sizeof(e))
    {
        printf("controller thread: received %lu, instead of %lu.\n",
               n, sizeof(e));
    }

    process_input_events(e, game, fd);
}

void process_server_fd(struct pollfd pollfd, struct game& game)
{
    // printf("controller thread: read socket.\n");
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
            printf("controller thread: socket closed.\n");
            close(pollfd.fd);
            quit = true;
        }
        else
        {
            enum msg_type type = (enum msg_type)buf[0];
            printf("msg type=%d, len=%d.\n", type, n);
            if(type == msg_type::move_msg)
            {
                struct move_msg msg = *(struct move_msg*)buf;
                assert(n == sizeof(msg));
                struct move move;
                move.src.row = msg.src_row;
                move.src.col = msg.src_col;
                move.dst.row = msg.dst_row;
                move.dst.col = msg.dst_col;
                game = apply_move(game, move);
                if(next_valid_moves(game).size() == 0)
                {
                    if(is_king_checked(game))
                        printf("CHECKMATE!!\n");
                    else
                        printf("STALEMATE!!\n");
                }
            }
            else if(type == msg_type::reject_move_msg)
            {
                printf("rejected msg.\n");
                assert(n == sizeof(struct reject_move_msg));
                game = last_game;
            }
            else if(type == msg_type::new_game_msg)
            {
                struct new_game_msg msg = *(struct new_game_msg*)buf;
                assert(n == sizeof(msg));
                printf("new_game_msg={player_color=%d}\n", msg.player_color);
                fflush(stdout);
                struct game new_game;
                game = new_game;
                game.pieces = initial_board;
                player_color = msg.player_color;
            }
            else if(type == msg_type::game_evt_msg)
            {
                printf("Opponent resigned.\n");
            }
            else
            {
                assert(false);
            }
        }
    }
}

void controller_thread(string ip, string port, int sdl_evt_fd)
{
    init_sdl();

    struct game game;
    game.pieces = initial_board;
    paint_screen(game);

    int fd = init_network(ip, port);

    while(!quit)
    {
        struct pollfd fds[2] = {{sdl_evt_fd, POLLIN, 0},{fd, POLLIN, 0}};
        int nfds = poll(&fds[0], arraysize(fds), -1/*timeout*/);
        if(nfds == -1)
        {
            perror("poll()");
            exit_failure();
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
                printf("controller thread: Un-processed event: ");
                print_pollfd(pollfd);
            }
        }


        paint_screen(game);
    }
    // TODO We must try to re-connect to the server.
    printf("controller thread: bye!\n");
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
        printf("SDL Thread: error, write() wrote %lu bytes instead of %lu\n",
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
    printf("sdl evt thread: bye!\n");
}

int main()
{
    int pipefd[2] = {-1,-1};
    int res = pipe(pipefd);
    if(res == -1)
    {
        perror("pipe()");
        exit_failure();
    }

    thread t(sdl_evt_thread, pipefd[1]);

    controller_thread("127.0.0.1", "55555", pipefd[0]);
    printf("bye!\n");

    close(pipefd[1]);
    t.join();
}
