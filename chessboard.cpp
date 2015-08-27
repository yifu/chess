#include <iostream>

#include <SDL2/SDL.h>

using namespace std;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;

void paint_chess_board()
{
    SDL_Rect dst, viewport;

    SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    SDL_RenderGetViewport(ren, &viewport);

    dst.x = dst.y = 0;
    dst.w = viewport.w / 8;
    dst.h = viewport.h / 8;
    for(int c = 0; c < 8; c++)
    {
	dst.x = 0;
	for(int r = 0; r < 8; r++)
	{
	    if((c % 2 == 0 && r % 2 == 0) || (c % 2 == 1 && r % 2 == 1))
		SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE); // White square
	    else
		SDL_SetRenderDrawColor(ren, 255, 0, 0, SDL_ALPHA_OPAQUE); // Dark square

	    // printf("dst.x=%d, dst.y=%d, dst.w=%d, dst.h=%d.\n",
	    //        dst.x, dst.y, dst.w, dst.h);
	    SDL_RenderFillRect(ren, &dst);
	    dst.x += dst.w;
	}
	dst.y += dst.h;
    }
    SDL_RenderPresent(ren);
    SDL_UpdateWindowSurface(display);
}

int main()
{
    int res = 0;
    constexpr Uint32 renderer_flags = 0;
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 640;
    bool quit = false;

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

    while(!quit)
    {
	SDL_Event e;
	if(SDL_PollEvent(&e))
	{
	    if(e.type == SDL_QUIT)
	    {
		printf("quit\n");
	    	quit = true;
	    }
	    else if(e.type == SDL_KEYDOWN)
	    {
		printf("key down %d\n", e.key.keysym.sym);
		if(e.key.keysym.sym == SDLK_ESCAPE)
		    quit = true;
	    }
	}
	paint_chess_board();
    }
    printf("bye!\n");

 clean:
    if(ren)
	SDL_DestroyRenderer(ren);
    if(display)
	SDL_DestroyWindow(display);
    SDL_Quit();
    return res;
}

// g++ -ggdb -std=c++11 chessboard.cpp $(sdl2-config --cflags --libs)
