#include "utils.hpp"

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
