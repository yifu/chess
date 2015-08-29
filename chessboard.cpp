#include <iostream>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "utils.hpp"

using namespace std;

bool quit = false;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;

vector<SDL_Texture*> textures;
vector<SDL_Surface*> surfaces;

int square_width, square_heigh;

struct square
{
    uint8_t row, col;
};

struct square detect_square(Sint32 x, Sint32 y)
{
    // TODO If player is black, then the coordinates are inversed.

    struct square result;
    result.row = y / square_heigh;
    result.col = x / square_width;
    assert(result.row <= 7);
    assert(result.col <= 7);
    return result;
}

void print_square(struct square square)
{
    printf("square={row=%d,col=%d}\n", square.row, square.col);
}

SDL_Rect square2rect(struct square square)
{
    assert(square.row <= 7);
    assert(square.col <= 7);

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
    struct square orig_square = {0,0};
};

vector<struct sprite> sprites;

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

void assertInvariants()
{
    int dragged_sprite_cnt = 0;
    for(auto sprite : sprites)
    {
	if(sprite.is_dragged)
	{
	    dragged_sprite_cnt++;
	    continue;
	}
	assert(sprite.rect == square2rect(sprite.orig_square));
    }
    assert(dragged_sprite_cnt == 0 || dragged_sprite_cnt == 1);
}

void process_input_events()
{
    SDL_Event e;
    assertInvariants();

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
            // print_mouse_motion(e);
	    bool found = false; // TODO int count?
	    for(size_t i = 0; i < sprites.size(); i++)
	    {
		if(sprites[i].is_dragged)
		{
		    assert(!found);
		    found = true;
		    // TODO Implement a function: updateDraggedXY() or updateDraggedSpritePos()...
		    sprites[i].rect.x = e.motion.x - square_width / 2;
		    sprites[i].rect.y = e.motion.y - square_heigh / 2;
		}
	    }


            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            // print_mouse_button_event(e);
	    for(size_t i = 0; i < sprites.size(); i++)
	    {
		if(is_hitting_rect(sprites[i].rect, e.button.x, e.button.y))
		{
		    sprites[i].rect.x = e.motion.x - square_width / 2;
		    sprites[i].rect.y = e.motion.y - square_heigh / 2;
		    sprites[i].is_dragged = true;
		}
	    }
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
	    bool found = false;
	    for(size_t i = 0; i < sprites.size(); i++)
	    {
		if(sprites[i].is_dragged)
		{
		    assert(!found);
		    found = true;
		    sprites[i].is_dragged = false;
		    sprites[i].orig_square = detect_square(e.button.x, e.button.y);
		    // print_square(s);
		    sprites[i].rect = square2rect(sprites[i].orig_square);
		    // print_rect(sprites[i].rect);
		}
	    }
	    // print_mouse_button_event(e);
            break;
        }
        default:
        {
            printf("e.type=%d\n", e.type);
            break;
        }
        }
    }
    assertInvariants();
}

void paint_chess_board()
{
    SDL_Rect dst;

    assertInvariants();

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
    assertInvariants();
}

void paint_sprites()
{
    assertInvariants();
    for(auto sprite : sprites)
    {
	if(sprite.is_dragged)
	    continue;
	SDL_RenderCopy(ren, sprite.tex, nullptr, &sprite.rect);
    }

    for(auto sprite : sprites)
    {
	if(!sprite.is_dragged)
	    continue;
	SDL_RenderCopy(ren, sprite.tex, nullptr, &sprite.rect);
    }

    assertInvariants();
}

void clean_up()
{
    for(auto texture: textures)
    {
	assert(texture);
	SDL_DestroyTexture(texture);
    }

    for(auto surface : surfaces)
    {
	assert(surface);
	SDL_FreeSurface(surface);
    }

    if(ren)
        SDL_DestroyRenderer(ren);

    if(display)
        SDL_DestroyWindow(display);
    SDL_Quit();
}

void exit_success()
{
    clean_up();
    exit(EXIT_SUCCESS);
}

void exit_failure()
{
    clean_up();
    exit(EXIT_FAILURE);
}

void initSprite(struct square square, string img_filename)
{
    SDL_Surface *surface = IMG_Load(img_filename.c_str());
    if(!surface) {
	cerr << "IMG_Load() error : " << IMG_GetError() << endl;
	exit_failure();
    }
    surfaces.push_back(surface);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);
    if(!texture) {
	cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
	exit_failure();
    }
    textures.push_back(texture);

    struct sprite sprite;
    sprite.tex = texture;
    sprite.orig_square = square;
    sprite.rect = square2rect(sprite.orig_square);
    sprites.push_back(sprite);
}

void initWindowIcon()
{
    SDL_Surface *icon = IMG_Load("./Chess_ndt60.png");
    if(!icon) {
	cerr << "IMG_Load() error : " << IMG_GetError() << endl;
	exit_failure();
    }
    surfaces.push_back(icon);
    icon = SDL_ConvertSurfaceFormat(icon, SDL_PIXELFORMAT_ARGB8888, 0);
    assert(icon->format->format == SDL_PIXELFORMAT_ARGB8888);
    printf("Set Window icon.\n");
    SDL_SetWindowIcon(display, icon);
}

void initSprites()
{
    assert(ren);

    struct square square;

    for(uint8_t i = 0; i < 8; i++)
    {
	square = {6, i};
	initSprite(square, "./Chess_plt60.png");
	square = {1, i};
	initSprite(square, "./Chess_pdt60.png");
    }

    square = {7, 0};
    initSprite(square, "./Chess_rlt60.png");
    square = {7, 7};
    initSprite(square, "./Chess_rlt60.png");
    square = {0, 0};
    initSprite(square, "./Chess_rdt60.png");
    square = {0, 7};
    initSprite(square, "./Chess_rdt60.png");

    square = {7, 1};
    initSprite(square, "./Chess_nlt60.png");
    square = {7, 6};
    initSprite(square, "./Chess_nlt60.png");
    square = {0, 1};
    initSprite(square, "./Chess_ndt60.png");
    square = {0, 6};
    initSprite(square, "./Chess_ndt60.png");

    square = {7, 2};
    initSprite(square, "./Chess_blt60.png");
    square = {7, 5};
    initSprite(square, "./Chess_blt60.png");
    square = {0, 2};
    initSprite(square, "./Chess_bdt60.png");
    square = {0, 5};
    initSprite(square, "./Chess_bdt60.png");

    square = {7, 3};
    initSprite(square, "./Chess_qlt60.png");
    square = {0, 3};
    initSprite(square, "./Chess_qdt60.png");

    square = {7, 4};
    initSprite(square, "./Chess_klt60.png");
    square = {0, 4};
    initSprite(square, "./Chess_kdt60.png");

    assertInvariants();
}

int main()
{
    constexpr Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 640;
    SDL_Rect viewport;

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

    ren = SDL_CreateRenderer(display, -1/*index*/, renderer_flags);
    if(!ren) {
        cerr << "SDL_CreateRenderer() error :" << SDL_GetError() << "." << endl;
	exit_failure();
    }

    SDL_RenderGetViewport(ren, &viewport);
    square_width = viewport.w / 8;
    square_heigh = viewport.h / 8;

    initWindowIcon();
    SDL_ShowWindow(display);

    initSprites();

    while(!quit)
    {
        process_input_events();

	SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

        paint_chess_board();
	paint_sprites();

	SDL_RenderPresent(ren);
	assert(display);
	SDL_UpdateWindowSurface(display);
    }
    printf("bye!\n");

    exit_success();
}

// g++ -Wall -Wextra -ggdb -std=c++11 chessboard.cpp $(sdl2-config --cflags --libs) -lSDL2_image
