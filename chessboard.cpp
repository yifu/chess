#include <iostream>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

using namespace std;

bool quit = false;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;
SDL_Surface *img = nullptr;
SDL_Texture *pawn = nullptr;

SDL_Rect pawn_rect;
bool is_pawn_dragged = false;

int square_width, square_heigh;

struct timespec last_time;

struct square
{
    uint8_t row, col;
};

square detect_square(Sint32 x, Sint32 y)
{
    // TODO If player is black, then the coordinates are inversed.

    square result;
    result.row = y / square_heigh;
    result.col = x / square_width;
    assert(result.row <= 7);
    assert(result.col <= 7);
    return result;
}

void print_square(square s)
{
    printf("square={row=%d,col=%d}\n", s.row, s.col);
}

SDL_Rect square2rect(square s)
{
    assert(s.row <= 7);
    assert(s.col <= 7);

    SDL_Rect rect;
    rect.x = s.col * square_width;
    rect.y = s.row * square_heigh;
    rect.w = square_width;
    rect.h = square_heigh;
    return rect;
}


void print_rect(SDL_Rect r)
{
    printf("r.x=%d, r.y=%d, r.w=%d, r.h=%d.\n",
           r.x, r.y, r.w, r.h);
}

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

bool on_pawn_clicked(Sint32 x, Sint32 y)
{
    return x >= pawn_rect.x &&
        x <= pawn_rect.x + pawn_rect.w &&
        y >= pawn_rect.y &&
        y <= pawn_rect.y + pawn_rect.h;
}

void drag_pawn(Sint32 x, Sint32 y)
{
    pawn_rect.x = x - square_width / 2;
    pawn_rect.y = y - square_heigh / 2;
    is_pawn_dragged = true;
}

void process_input_events()
{
    SDL_Event e;
    if(SDL_PollEvent(&e))
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
            print_mouse_motion(e);
            if(is_pawn_dragged)
                drag_pawn(e.motion.x, e.motion.y);
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            print_mouse_button_event(e);
            if(on_pawn_clicked(e.button.x, e.button.y))
                drag_pawn(e.button.x, e.button.y);
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            is_pawn_dragged = false;
            square s = detect_square(e.button.x, e.button.y);
            print_square(s);
            SDL_Rect rect = square2rect(s);
            pawn_rect = rect;
            print_rect(pawn_rect);
            print_mouse_button_event(e);
            break;
        }
        default:
        {
            printf("e.type=%d\n", e.type);
            break;
        }
        }
    }
}

void paint_chess_board()
{
    SDL_Rect dst;

    SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

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

    // struct timespec new_time;
    // res = clock_gettime(CLOCK_MONOTONIC_RAW, &new_time);
    // if(res == -1)
    // {
    //     perror("clock_gettime():");
    //     res = 1;
    //     exit(EXIT_FAILURE);
    // }

    // uint64_t diff = substract_time(new_time, last_time);
    // if(diff > 16*1000000)
    // {
    //     pawn_rect.x += 4;
    //     last_time = new_time;
    // }

    SDL_RenderCopy(ren, pawn, nullptr, &pawn_rect);

    SDL_RenderPresent(ren);
    SDL_UpdateWindowSurface(display);
}

int main()
{
    int res = 0;
    constexpr Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 640;
    SDL_Rect viewport;

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cerr << "SDL_Init error: " << SDL_GetError() << "." << endl;
        res = 1;
        goto clean;
    }

    display = SDL_CreateWindow("Hello world!",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               screenwidth, screenheigh, SDL_WINDOW_SHOWN);
    if(!display) {
        cerr << "SDL_CreateWindow() error : " << SDL_GetError() << "." << endl;
        res = 1;
        goto clean;
    }

    ren = SDL_CreateRenderer(display, -1/*index*/, renderer_flags);
    if(!ren) {
        cerr << "SDL_CreateRenderer() error :" << SDL_GetError() << "." << endl;
        res = 1;
        goto clean;
    }

    img = IMG_Load("./Chess_plt60.png");
    if(!img) {
        cerr << "IMG_Load() error : " << IMG_GetError() << endl;
        res = 1;
        goto clean;
    }

    pawn = SDL_CreateTextureFromSurface(ren, img);
    if(!pawn) {
        cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
        res = 1;
        goto clean;
    }

    if(img)
        SDL_FreeSurface(img);
    img = nullptr;

    SDL_RenderGetViewport(ren, &viewport);
    square_width = viewport.w / 8;
    square_heigh = viewport.h / 8;

    pawn_rect.x = pawn_rect.y = 0;
    pawn_rect.w = square_width;
    pawn_rect.h = square_heigh;

    res = clock_gettime(CLOCK_MONOTONIC_RAW, &last_time);
    if(res == -1)
    {
        perror("clock_gettime():");
        res = 1;
        exit(EXIT_FAILURE);
    }

    while(!quit)
    {
        process_input_events();
        paint_chess_board();
    }
    printf("bye!\n");

 clean:
    if(pawn)
        SDL_DestroyTexture(pawn);
    if(img)
        SDL_FreeSurface(img);
    if(ren)
        SDL_DestroyRenderer(ren);
    if(display)
        SDL_DestroyWindow(display);
    SDL_Quit();
    return res;
}

// g++ -Wall -Wextra -ggdb -std=c++11 chessboard.cpp $(sdl2-config --cflags --libs) -lSDL2_image
