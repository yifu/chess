#include <iostream>

#include <SDL2/SDL.h>

using namespace std;

int main()
{
    int res = 0;
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    SDL_Window *display = nullptr;
    SDL_Renderer *ren = nullptr;
    SDL_Surface *bmp = nullptr;
    SDL_Texture *tex = nullptr;

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
	cerr << "SDL_Init error: " << SDL_GetError() << "." << endl;
	res = 1;
	goto clean;
    }

    cout << "Base path = " << SDL_GetBasePath() << "." << endl;

    display = SDL_CreateWindow("Hello world!", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
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

    bmp = SDL_LoadBMP("./hello.bmp");
    if(!bmp) {
	cerr << "SDL_LoadBMP() error : " << SDL_GetError() << "." << endl;
	res = 1;
	goto clean;
    }

    tex = SDL_CreateTextureFromSurface(ren, bmp);
    if(!tex) {
	cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
	res = 1;
	goto clean;
    }

    SDL_FreeSurface(bmp);
    for(int i = 0; i < 3; i++) {
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren, tex, nullptr, nullptr);
	SDL_RenderPresent(ren);
	SDL_Delay(1000);
    }

 clean:
    if(bmp)
	SDL_FreeSurface(bmp);
    if(ren)
	SDL_DestroyRenderer(ren);
    if(display)
	SDL_DestroyWindow(display);
    SDL_Quit();
    return res;
}

// g++ -std=c++11 main.cpp $(sdl2-config --cflags --libs)
