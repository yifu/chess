#include <iostream>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

using namespace std;

bool quit = false;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;
SDL_Surface *img = nullptr;
SDL_Texture *tex = nullptr;

int square_width, square_heigh;

bool operator == (SDL_Rect l, SDL_Rect r)
{
    return l.x == r.x && l.y == r.y &&
	l.w == r.w && l.h == r.h;
}

bool is_hitting_rect(SDL_Rect rect, Sint32 x, Sint32 y)
{
    return x >= rect.x &&
        x <= rect.x + rect.w &&
        y >= rect.y &&
        y <= rect.y + rect.h;
}

void print_rect(SDL_Rect r)
{
    printf("r.x=%d, r.y=%d, r.w=%d, r.h=%d.\n",
           r.x, r.y, r.w, r.h);
}

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

struct piece
{
    SDL_Texture *tex = nullptr;
    SDL_Rect rect = {0,0,0,0};
    bool is_dragged = false;
    struct square orig_square = {0,0};
};

vector<struct piece> pieces;

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
    int dragged_piece_cnt = 0;
    for(auto piece : pieces)
    {
	if(piece.is_dragged)
	{
	    dragged_piece_cnt++;
	    continue;
	}
	assert(piece.rect == square2rect(piece.orig_square));
    }
    assert(dragged_piece_cnt == 0 || dragged_piece_cnt == 1);
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
	    for(size_t i = 0; i < pieces.size(); i++)
	    {
		if(pieces[i].is_dragged)
		{
		    assert(!found);
		    found = true;
		    // TODO Implement a function: updateDraggedXY() or updateDraggedPiecePos()...
		    pieces[i].rect.x = e.motion.x - square_width / 2;
		    pieces[i].rect.y = e.motion.y - square_heigh / 2;
		}
	    }


            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            // print_mouse_button_event(e);
	    for(size_t i = 0; i < pieces.size(); i++)
	    {
		if(is_hitting_rect(pieces[i].rect, e.button.x, e.button.y))
		{
		    pieces[i].rect.x = e.motion.x - square_width / 2;
		    pieces[i].rect.y = e.motion.y - square_heigh / 2;
		    pieces[i].is_dragged = true;
		}
	    }
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
	    bool found = false;
	    for(size_t i = 0; i < pieces.size(); i++)
	    {
		if(pieces[i].is_dragged)
		{
		    assert(!found);
		    found = true;
		    pieces[i].is_dragged = false;
		    pieces[i].orig_square = detect_square(e.button.x, e.button.y);
		    // print_square(s);
		    pieces[i].rect = square2rect(pieces[i].orig_square);
		    // print_rect(pieces[i].rect);
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

void paint_pieces()
{
    assertInvariants();
    for(auto p : pieces)
    {
	SDL_RenderCopy(ren, p.tex, nullptr, &p.rect);
    }
    assertInvariants();
}

int main()
{
    int res = 0;
    constexpr Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 640;
    SDL_Rect viewport;
    piece p;

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

    tex = SDL_CreateTextureFromSurface(ren, img);
    if(!tex) {
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

    for(int i = 0; i < 8; i++)
    {
	p.tex = tex;
	p.orig_square.row = 6;
	p.orig_square.col = i;
	p.rect = square2rect(p.orig_square);
	SDL_Rect rect = { i * square_width, square_heigh * 6,
			  square_width, square_heigh };
	assert(p.rect == rect);
	pieces.push_back(p);
    }

    while(!quit)
    {
        process_input_events();

	SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(ren);

        paint_chess_board();
	paint_pieces();

	SDL_RenderPresent(ren);
	SDL_UpdateWindowSurface(display);
    }
    printf("bye!\n");

 clean:
    if(tex)
        SDL_DestroyTexture(tex);
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
