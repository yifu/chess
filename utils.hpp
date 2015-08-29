#pragma once

#include <SDL2/SDL.h>

bool operator == (SDL_Rect l, SDL_Rect r);
bool is_hitting_rect(SDL_Rect rect, Sint32 x, Sint32 y);
void print_rect(SDL_Rect r);
