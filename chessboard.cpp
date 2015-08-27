#include <iostream>

#include <SDL2/SDL.h>

using namespace std;

int main()
{
    int res = 0;
    constexpr Uint32 renderer_flags = 0;
    SDL_Window *display = nullptr;
    SDL_Renderer *ren = nullptr;
    SDL_Rect dst;
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 480;

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

    for(int i = 0; i < 3; i++) {
	SDL_RenderClear(ren);

	dst.x = dst.y = 0;
	dst.w = dst.h = 30;
	// SDL_RenderCopy(ren, square, nullptr, &dst);

	SDL_RenderPresent(ren);

	SDL_Delay(1000);
    }

 clean:
    if(ren)
	SDL_DestroyRenderer(ren);
    if(display)
	SDL_DestroyWindow(display);
    SDL_Quit();
    return res;
}

// g++ -ggdb -std=c++11 chessboard.cpp $(sdl2-config --cflags --libs)
