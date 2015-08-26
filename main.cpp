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
    SDL_Texture *background = nullptr;
    SDL_Texture *image = nullptr;
    SDL_Rect dst;

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

    bmp = SDL_LoadBMP("./background.bmp");
    if(!bmp) {
	cerr << "SDL_LoadBMP() error : " << SDL_GetError() << endl;
	res = 1;
	goto clean;
    }

    background = SDL_CreateTextureFromSurface(ren, bmp);
    if(!background) {
	cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
	res = 1;
	goto clean;
    }

    SDL_FreeSurface(bmp);
    bmp = nullptr;

    bmp = SDL_LoadBMP("./image.bmp");
    if(!bmp) {
	cerr << "SDL_LoadBMP() error : " << SDL_GetError() << endl;
	res = 1;
	goto clean;
    }

    image = SDL_CreateTextureFromSurface(ren, bmp);
    if(!image) {
	cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
	res = 1;
	goto clean;
    }

    SDL_FreeSurface(bmp);
    bmp = nullptr;

    res = SDL_QueryTexture(background, nullptr, nullptr, &dst.w, &dst.h);
    if(res != 0) {
	cerr << "SDL_QueryTexture() error : " << SDL_GetError() << "." << endl;
	goto clean;
    }

    for(int i = 0; i < 3; i++) {
	SDL_RenderClear(ren);

	dst.x = dst.y = 0;
	SDL_RenderCopy(ren, background, nullptr, &dst);
	dst.x = dst.w;
	dst.y = 0;
	SDL_RenderCopy(ren, background, nullptr, &dst);
	dst.x = 0;
	dst.y = dst.h;
	SDL_RenderCopy(ren, background, nullptr, &dst);
	dst.x = dst.w;
	dst.y = dst.h;
	SDL_RenderCopy(ren, background, nullptr, &dst);

	SDL_RenderPresent(ren);

	SDL_Delay(1000);
    }

 clean:
    if(image)
	SDL_DestroyTexture(image);
    if(background)
	SDL_DestroyTexture(background);
    if(bmp)
	SDL_FreeSurface(bmp);
    if(ren)
	SDL_DestroyRenderer(ren);
    if(display)
	SDL_DestroyWindow(display);
    SDL_Quit();
    return res;
}

// g++ -ggdb -std=c++11 main.cpp $(sdl2-config --cflags --libs)
